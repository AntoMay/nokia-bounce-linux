/*
 * RTPL (Nokia Smart Messaging OTA Ringing Tone) decoder for the ORIGINAL
 * Bounce `.ott` resources.
 *
 * See rtpl_decoder.h for the full provenance of the grammar, the field widths,
 * and the implementation boundary. The short version:
 *
 *   - This module decodes bytes into events and stops there.
 *   - It opens no audio device of any kind, generates no PCM, and synthesises
 *     no waveform. It contains no waveform, duty-cycle, amplitude, speaker or
 *     oscillator assumption, because Smart Messaging Rev 2.0.0 defines none.
 *   - It performs no dynamic allocation. Every field is read through a
 *     bounds-checked MSB-first bit reader.
 *   - Every malformed input is rejected with a specific status. Nothing is
 *     guessed, defaulted, or silently repaired.
 *
 * Word-level summary of the grammar actually implemented (bit widths are the
 * ones proven by STEP 14B-19's byte-exact re-encode, not re-derived here):
 *
 *   <command-length>              8 bits   = number of command parts
 *   <command-part>                 7 bits   each, octet-aligned between parts
 *   payload begins immediately after the LAST command part (no trailing align)
 *   <song-type>                    3 bits   001 = basic song
 *   <text-length>                  4 bits   then that many 8-bit characters
 *   <song-sequence-length>         8 bits   >= 1
 *   per pattern:
 *     <pattern-header-id>          3 bits   000
 *     <pattern-id>                 2 bits   00=A 01=B 10=C 11=D
 *     <loop-value>                 4 bits   0 = no repeat, 15 = infinite
 *     <pattern-specifier>          8 bits   0 = reuse defined pattern (rejected)
 *                                           1..255 = instruction count
 *     per instruction:
 *       <note-instruction-id>      3 bits   001
 *         <note-value>             4 bits   0=pause 1=C .. 12=H, 13..15 reserved
 *         <note-duration>          3 bits   000..101, 110/111 reserved
 *         <note-duration-specifier> 2 bits
 *       <scale-instruction-id>     3 bits   010, then <note-scale> 2 bits
 *       <style-instruction-id>     3 bits   011, then <style-value> 2 bits
 *       <tempo-instruction-id>     3 bits   100, then <beats-per-minute> 5 bits
 *       <volume-instruction-id>    3 bits   101, then <volume> 4 bits
 *   <command-end>                  8 bits   00000000
 *   trailing padding               0..7 bits, all zero
 */

#include "rtpl_decoder.h"

#include <string.h>

/* ------------------------------------------------------------------------ */
/* Constants transcribed from Smart Messaging Specification Rev 2.0.0 §3.8   */
/* ------------------------------------------------------------------------ */

enum {
    /* Table 3.8-1. A Bounce `.ott` is <ringing-tone-programming> then
     * <sound>; nothing else may lead. Gammu's recogniser tests the leading
     * bytes for 0x02 0x4A, which is exactly this pair seen from byte level:
     * 0x02 is the command length and 0x4A is <ringing-tone-programming> (0x25)
     * stored left-shifted by one within its octet.
     *
     * The specification also permits an optional third <unicode> command part.
     * This decoder does NOT implement that form: a file declaring more than two
     * command parts is rejected outright rather than partially decoded, which
     * is a loud failure instead of a silent misreading. None of the three
     * original Bounce assets contains it. */
    RTPL_COMMAND_LENGTH_OCTETS = 2,
    RTPL_MAX_COMMAND_PARTS = 2,
    RTPL_COMMAND_PART_RINGING_TONE_PROGRAMMING = 0x25,
    RTPL_COMMAND_PART_SOUND = 0x1d
};

/* Table 3.8-6. Codes 13..15 are RESERVED and must be rejected. */
enum {
    RTPL_NOTE_VALUE_PAUSE = 0,
    RTPL_NOTE_VALUE_FIRST = 1,  /* C  */
    RTPL_NOTE_VALUE_LAST = 12,  /* H  */
    RTPL_NOTE_VALUE_MAX_RESERVED = 15
};

/* Table 3.8-8 <note-duration-specifier> multiplicative factors. */
static const double RTPL_DURATION_SPECIFIER_FACTOR[4] = {
    1.0,            /* 00 no special duration  */
    1.5,            /* 01 dotted note          */
    1.75,           /* 10 double dotted note   */
    2.0 / 3.0       /* 11 2/3 length          */
};

/*
 * Table 3.8-11, <beats-per-minute>, 32 rows indexed by the 5-bit field.
 * Index 14 is 125 BPM, the tempo carried by all three original assets.
 */
static const unsigned int RTPL_BPM_TABLE[32] = {
    25, 28, 31, 35, 40, 45, 50, 56,
    63, 70, 80, 90, 100, 112, 125, 140,
    160, 180, 200, 225, 250, 285, 320, 355,
    400, 450, 500, 565, 635, 715, 800, 900
};

/* Table 3.8-9, "Note A is ...": the scale anchor in Hz, indexed by scale code. */
static const double RTPL_SCALE_ANCHOR_HZ[4] = {
    440.0, 880.0, 1760.0, 3520.0
};

/*
 * The specification's own scale model, f = scale_A * 2^((note - 10) / 12), with
 * note 10 = A. So the semitone offset relative to A is (note - 10), and the
 * table is indexed by (note - 1), giving offsets -9 .. +2.
 *
 * These are the exact double-precision values of 2^(k/12) for k = -9..+2, listed
 * rather than computed with pow() so the decoder needs no libm and produces
 * bit-identical results on every platform.
 */
static const double RTPL_SEMITONE_RATIO[12] = {
    0.5946035575013605,   /* C   -9 */
    0.6299605249474366,   /* Cis -8 */
    0.6674199270850172,   /* D   -7 */
    0.7071067811865476,   /* Dis -6 */
    0.7491535384383408,   /* E   -5 */
    0.7937005259840998,   /* F   -4 */
    0.8408964152537145,   /* Fis -3 */
    0.8908987181403393,   /* G   -2 */
    0.9438743126816935,   /* Gis -1 */
    1.0,                  /* A    0 */
    1.0594630943592953,   /* Ais +1 */
    1.1224620483093730    /* H   +2 */
};

/* The specification's default scale (Table 3.8-9 marks Scale-2 "default"). */
enum { RTPL_DEFAULT_SCALE = 1 };
/* The specification's default tempo (Table 3.8-11 marks 63 BPM "default"). */
enum { RTPL_DEFAULT_TEMPO_INDEX = 8 };
/* The specification's default style (Table 3.8-10 marks Natural "default"). */
enum { RTPL_DEFAULT_STYLE = 0 };
/* The specification's default volume (Table 3.8-12 marks level-7 "default"). */
enum { RTPL_DEFAULT_VOLUME_LEVEL = 7 };

/* Instruction IDs, Table 3.8-5. */
enum {
    RTPL_INSTRUCTION_PATTERN_HEADER = 0,
    RTPL_INSTRUCTION_NOTE = 1,
    RTPL_INSTRUCTION_SCALE = 2,
    RTPL_INSTRUCTION_STYLE = 3,
    RTPL_INSTRUCTION_TEMPO = 4,
    RTPL_INSTRUCTION_VOLUME = 5
};

/* Largest payload accepted by bounce_rtpl_decode_file. The original assets are
 * 16..18 bytes; this bound only exists so the file path can use a fixed stack
 * buffer and therefore never allocates. */
enum { RTPL_MAX_FILE_BYTES = 1024 };

/* ------------------------------------------------------------------------ */
/* Bounds-checked MSB-first bit reader                                       */
/* ------------------------------------------------------------------------ */

typedef struct RtplBitReader {
    const unsigned char *data;
    size_t length_bits;
    size_t position;
} RtplBitReader;

static void rtpl_bits_init(RtplBitReader *reader,
                           const unsigned char *data,
                           size_t length)
{
    reader->data = data;
    reader->length_bits = length * 8u;
    reader->position = 0u;
}

/*
 * Read `count` bits, most significant first, into *value_out.
 *
 * Returns false without touching *value_out if the read would pass the end of
 * the buffer, so a truncated or over-long declared field is a rejection rather
 * than an out-of-bounds read.
 */
static bool rtpl_bits_read(RtplBitReader *reader,
                           unsigned int count,
                           unsigned int *value_out)
{
    unsigned int value = 0u;
    unsigned int index;

    if (reader->data == NULL || value_out == NULL || count > 24u) {
        return false;
    }
    if (reader->position > reader->length_bits) {
        return false;
    }
    if ((size_t)count > reader->length_bits - reader->position) {
        return false;
    }
    for (index = 0u; index < count; ++index) {
        size_t bit = reader->position + (size_t)index;
        size_t byte_index = bit / 8u;
        unsigned int shift = 7u - (unsigned int)(bit % 8u);
        unsigned int one = (unsigned int)((reader->data[byte_index] >> shift) & 1u);

        value = (value << 1) | one;
    }
    reader->position += (size_t)count;
    *value_out = value;
    return true;
}

/* Skip to the next octet boundary. Called BETWEEN command parts, never after
 * the last one, because the payload starts immediately after the last part. */
static void rtpl_bits_align(RtplBitReader *reader)
{
    size_t remainder = reader->position % 8u;

    if (remainder != 0u) {
        reader->position += 8u - remainder;
    }
}

static size_t rtpl_bits_remaining(const RtplBitReader *reader)
{
    if (reader->position >= reader->length_bits) {
        return 0u;
    }
    return reader->length_bits - reader->position;
}

/* Every trailing bit after <command-end> must be zero padding of less than one
 * octet. This also rejects a buffer that carries a whole extra octet of data,
 * which is how the "trailing garbage" negative test is detected. */
static bool rtpl_bits_padding_is_clean(const RtplBitReader *reader)
{
    size_t remaining = rtpl_bits_remaining(reader);
    size_t index;

    if (remaining >= 8u) {
        return false;
    }
    for (index = 0u; index < remaining; ++index) {
        size_t bit = reader->position + index;
        size_t byte_index = bit / 8u;
        unsigned int shift = 7u - (unsigned int)(bit % 8u);

        if (((reader->data[byte_index] >> shift) & 1u) != 0u) {
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------------ */
/* Public accessors                                                          */
/* ------------------------------------------------------------------------ */

double bounce_rtpl_nominal_frequency_hz(unsigned int scale, unsigned int note_value)
{
    if (scale > 3u || note_value < RTPL_NOTE_VALUE_FIRST
        || note_value > RTPL_NOTE_VALUE_LAST) {
        return -1.0;
    }
    /*
     * A pause (note value 0) has no pitch. The format models silence as a note
     * value, so this function reports it as a negative result rather than
     * inventing a frequency for it.
     */
    if (note_value == RTPL_NOTE_VALUE_PAUSE) {
        return -1.0;
    }
    return RTPL_SCALE_ANCHOR_HZ[scale] * RTPL_SEMITONE_RATIO[note_value - 1u];
}

const char *bounce_rtpl_note_name(unsigned int note_value)
{
    /* Table 3.8-6, including the specification's own aliases. Note that the
     * specification writes "Ais i.e. B" and "H" for the twelfth degree, so H is
     * the format's own name for what is usually spelled B. */
    static const char *const names[RTPL_NOTE_VALUE_MAX_RESERVED + 1] = {
        "pause", "C", "Cis", "D", "Dis", "E", "F", "Fis",
        "G", "Gis", "A", "Ais", "H", NULL, NULL, NULL
    };

    if (note_value > RTPL_NOTE_VALUE_MAX_RESERVED) {
        return NULL;
    }
    return names[note_value];
}

const char *bounce_rtpl_scale_label(unsigned int scale)
{
    /* The ENCODED names from Table 3.8-9. These are deliberately not converted
     * to scientific octaves: the encoded scale state is retained verbatim. */
    static const char *const labels[4] = {
        "Scale-1", "Scale-2", "Scale-3", "Scale-4"
    };

    if (scale > 3u) {
        return NULL;
    }
    return labels[scale];
}

const char *bounce_rtpl_style_name(unsigned int style)
{
    static const char *const names[4] = {
        "Natural", "Continuous", "Staccato", NULL /* reserved */
    };

    if (style > 3u) {
        return NULL;
    }
    return names[style];
}

const char *bounce_rtpl_status_text(BounceRtplStatus status)
{
    switch (status) {
    case BOUNCE_RTPL_OK:                          return "ok";
    case BOUNCE_RTPL_ERR_NULL_ARGUMENT:           return "null argument";
    case BOUNCE_RTPL_ERR_EMPTY_INPUT:             return "empty input";
    case BOUNCE_RTPL_ERR_TRUNCATED:               return "truncated input";
    case BOUNCE_RTPL_ERR_BAD_MAGIC:               return "bad .ott magic";
    case BOUNCE_RTPL_ERR_BAD_COMMAND_LENGTH:      return "bad command length";
    case BOUNCE_RTPL_ERR_BAD_COMMAND_PART:        return "bad command part";
    case BOUNCE_RTPL_ERR_UNSUPPORTED_SONG_TYPE:   return "unsupported song type";
    case BOUNCE_RTPL_ERR_BAD_PATTERN_SEQUENCE:    return "bad pattern sequence length";
    case BOUNCE_RTPL_ERR_ALREADY_DEFINED_PATTERN: return "already-defined pattern reuse is unsupported";
    case BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID:      return "bad instruction id";
    case BOUNCE_RTPL_ERR_RESERVED_NOTE_VALUE:     return "reserved note value";
    case BOUNCE_RTPL_ERR_RESERVED_DURATION:       return "reserved note duration";
    case BOUNCE_RTPL_ERR_RESERVED_STYLE:          return "reserved style value";
    case BOUNCE_RTPL_ERR_TOO_MANY_NOTES:          return "note count exceeds decoder bound";
    case BOUNCE_RTPL_ERR_MISSING_COMMAND_END:     return "missing or non-zero command-end";
    case BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING:    return "bad trailing padding or extra data";
    case BOUNCE_RTPL_ERR_IO:                       return "io error";
    default:                                      return "unknown status";
    }
}

/* ------------------------------------------------------------------------ */
/* Derived note values                                                        */
/* ------------------------------------------------------------------------ */

/*
 * Derive a note's sounding time in milliseconds.
 *
 * Table 3.8-11 binds a quarter note to 60/BPM seconds, and Table 3.8-7 gives
 * each duration code as a power-of-two division of the full note, so a
 * full note is four quarter notes.
 *
 * This is NOTE SOUND TIME ONLY. It excludes any rest between notes, because the
 * format defines no rest magnitude (see BOUNCE_RTPL_STYLE_REST_UNDEFINED).
 */
static unsigned int rtpl_note_duration_ms(unsigned int tempo_bpm,
                                           unsigned int duration_code,
                                           unsigned int specifier_code)
{
    double milliseconds;

    if (tempo_bpm == 0u) {
        return 0u;
    }
    milliseconds = (60000.0 / (double)tempo_bpm) * 4.0
                 / (double)(1u << duration_code);
    milliseconds *= RTPL_DURATION_SPECIFIER_FACTOR[specifier_code & 3u];
    return (unsigned int)(milliseconds + 0.5);
}

static void rtpl_song_finish(BounceRtplSong *song)
{
    unsigned int index;

    song->tempo_bpm = RTPL_BPM_TABLE[song->tempo_index & 31u];
    for (index = 0u; index < song->note_count; ++index) {
        BounceRtplNote *note = &song->notes[index];

        note->order = index;
        note->is_pause = (note->note_value == RTPL_NOTE_VALUE_PAUSE);
        note->duration_ms = rtpl_note_duration_ms(
            song->tempo_bpm,
            note->duration_code,
            note->duration_specifier_code);
        note->nominal_frequency_hz = note->is_pause
            ? -1.0
            : bounce_rtpl_nominal_frequency_hz(note->scale, note->note_value);
        song->total_note_duration_ms += note->duration_ms;
    }
    song->style_rest_duration_ms = BOUNCE_RTPL_STYLE_REST_UNDEFINED;
}

/* ------------------------------------------------------------------------ */
/* Decoder                                                                    */
/* ------------------------------------------------------------------------ */

BounceRtplStatus bounce_rtpl_decode(const unsigned char *data,
                                    size_t length,
                                    BounceRtplSong *song_out)
{
    RtplBitReader reader;
    BounceRtplSong song;
    unsigned int command_length;
    unsigned int command_part[RTPL_MAX_COMMAND_PARTS];
    unsigned int part_index;
    unsigned int field;
    unsigned int song_type_code;
    unsigned int sequence_length;
    unsigned int pattern_index;
    unsigned int current_scale = RTPL_DEFAULT_SCALE;
    BounceRtplStatus status = BOUNCE_RTPL_OK;

    if (song_out == NULL) {
        return BOUNCE_RTPL_ERR_NULL_ARGUMENT;
    }
    /* Zero BOTH the caller's song and the local working copy, so a caller can
     * never observe a partially decoded song and no uninitialized field can
     * reach the accumulators below. */
    memset(song_out, 0, sizeof(*song_out));
    memset(&song, 0, sizeof(song));
    if (data == NULL) {
        return BOUNCE_RTPL_ERR_NULL_ARGUMENT;
    }
    if (length == 0u) {
        return BOUNCE_RTPL_ERR_EMPTY_INPUT;
    }
    if (length < 2u) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }

    rtpl_bits_init(&reader, data, length);

    /* <command-length>: 8 bits. Zero parts, or a part count a `.ott` cannot
     * legally have, is rejected here. */
    if (!rtpl_bits_read(&reader, 8u, &command_length)) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }
    if (command_length == 0u || command_length > RTPL_MAX_COMMAND_PARTS) {
        return BOUNCE_RTPL_ERR_BAD_COMMAND_LENGTH;
    }

    /* <command-part>: 7 bits each, octet-aligned between parts. */
    for (part_index = 0u; part_index < command_length; ++part_index) {
        if (part_index > 0u) {
            rtpl_bits_align(&reader);
        }
        if (!rtpl_bits_read(&reader, 7u, &field)) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        command_part[part_index] = field;
    }
    /* The payload starts immediately after the LAST part: no align here. */

    /* A Bounce `.ott` leads with <ringing-tone-programming> then <sound>. This
     * is the byte-level "0x02 0x4A" magic Gammu tests for, expressed through
     * the grammar. */
    if (command_length < 2u
        || command_part[0] != RTPL_COMMAND_PART_RINGING_TONE_PROGRAMMING) {
        return BOUNCE_RTPL_ERR_BAD_COMMAND_PART;
    }
    if (command_part[1] != RTPL_COMMAND_PART_SOUND) {
        return BOUNCE_RTPL_ERR_BAD_COMMAND_PART;
    }

    /* <song-type>: 3 bits. Only basic song is decoded. */
    if (!rtpl_bits_read(&reader, 3u, &song_type_code)) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }
    if (song_type_code != 1u) {
        return BOUNCE_RTPL_ERR_UNSUPPORTED_SONG_TYPE;
    }
    song.song_type = BOUNCE_RTPL_SONG_TYPE_BASIC;

    /* <text-length>: 4 bits, then that many 8-bit characters when Unicode is
     * disabled. All three original assets are 0 (unnamed), but the length is
     * honoured and bounds-checked rather than assumed. */
    if (!rtpl_bits_read(&reader, 4u, &field)) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }
    song.title_length = field;
    if (field > 0u) {
        if (rtpl_bits_remaining(&reader) < (size_t)field * 8u) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        reader.position += (size_t)field * 8u;
    }

    /* <song-sequence-length>: 8 bits, at least one pattern. */
    if (!rtpl_bits_read(&reader, 8u, &sequence_length)) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }
    if (sequence_length == 0u) {
        return BOUNCE_RTPL_ERR_BAD_PATTERN_SEQUENCE;
    }
    song.pattern_count = sequence_length;

    for (pattern_index = 0u; pattern_index < sequence_length; ++pattern_index) {
        unsigned int instruction_count;
        unsigned int instruction_index;
        unsigned int pattern_id;
        unsigned int loop_value;
        unsigned int specifier;

        if (!rtpl_bits_read(&reader, 3u, &field)) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        if (field != RTPL_INSTRUCTION_PATTERN_HEADER) {
            return BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID;
        }
        if (!rtpl_bits_read(&reader, 2u, &pattern_id)) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        if (!rtpl_bits_read(&reader, 4u, &loop_value)) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        if (!rtpl_bits_read(&reader, 8u, &specifier)) {
            return BOUNCE_RTPL_ERR_TRUNCATED;
        }
        /* 00000000 means "use an already defined pattern again", which carries
         * no new instructions. The specification calls a zero length illegal
         * for a NEW pattern, so zero is rejected rather than treated as an
         * empty pattern. */
        if (specifier == 0u) {
            return BOUNCE_RTPL_ERR_ALREADY_DEFINED_PATTERN;
        }
        if (pattern_index == 0u) {
            song.first_pattern_id = pattern_id;
            song.first_pattern_loop_value = loop_value;
        }
        if (loop_value != 0u) {
            /* The format asked for repetitions this decoder does not expand.
             * No original Bounce asset uses a non-zero loop value, so this only
             * ever flags a file the originals never contained. */
            song.unexpanded_repeats = true;
        }
        instruction_count = specifier;

        for (instruction_index = 0u;
             instruction_index < instruction_count;
             ++instruction_index) {
            unsigned int instruction_id;

            if (!rtpl_bits_read(&reader, 3u, &instruction_id)) {
                return BOUNCE_RTPL_ERR_TRUNCATED;
            }
            switch (instruction_id) {
            case RTPL_INSTRUCTION_NOTE: {
                BounceRtplNote *note;
                unsigned int note_value;
                unsigned int duration_code;
                unsigned int specifier_code;

                if (!rtpl_bits_read(&reader, 4u, &note_value)
                    || !rtpl_bits_read(&reader, 3u, &duration_code)
                    || !rtpl_bits_read(&reader, 2u, &specifier_code)) {
                    return BOUNCE_RTPL_ERR_TRUNCATED;
                }
                if (note_value > RTPL_NOTE_VALUE_LAST) {
                    return BOUNCE_RTPL_ERR_RESERVED_NOTE_VALUE;
                }
                if (duration_code > BOUNCE_RTPL_DURATION_THIRTY_SECOND) {
                    return BOUNCE_RTPL_ERR_RESERVED_DURATION;
                }
                if (song.note_count >= BOUNCE_RTPL_MAX_NOTES) {
                    return BOUNCE_RTPL_ERR_TOO_MANY_NOTES;
                }
                note = &song.notes[song.note_count];
                note->note_value = note_value;
                note->scale = current_scale;
                note->duration_code = duration_code;
                note->duration_specifier_code = specifier_code;
                song.note_count += 1u;
                break;
            }
            case RTPL_INSTRUCTION_SCALE:
                if (!rtpl_bits_read(&reader, 2u, &field)) {
                    return BOUNCE_RTPL_ERR_TRUNCATED;
                }
                /* A 2-bit field has no invalid value; every code is a defined
                 * scale, which is why no range check is needed or wanted. */
                current_scale = field;
                break;
            case RTPL_INSTRUCTION_STYLE:
                if (!rtpl_bits_read(&reader, 2u, &field)) {
                    return BOUNCE_RTPL_ERR_TRUNCATED;
                }
                if (field > (unsigned int)BOUNCE_RTPL_STYLE_STACCATO) {
                    return BOUNCE_RTPL_ERR_RESERVED_STYLE;
                }
                song.style = (BounceRtplStyle)field;
                song.style_present = true;
                break;
            case RTPL_INSTRUCTION_TEMPO:
                if (!rtpl_bits_read(&reader, 5u, &field)) {
                    return BOUNCE_RTPL_ERR_TRUNCATED;
                }
                song.tempo_index = field;
                break;
            case RTPL_INSTRUCTION_VOLUME:
                if (!rtpl_bits_read(&reader, 4u, &field)) {
                    return BOUNCE_RTPL_ERR_TRUNCATED;
                }
                song.volume_level = field;
                song.volume_present = true;
                break;
            default:
                return BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID;
            }
        }
    }

    /* <command-end>: 8 zero bits. A non-zero value means the stream was not
     * terminated legally. */
    if (!rtpl_bits_read(&reader, 8u, &field)) {
        return BOUNCE_RTPL_ERR_TRUNCATED;
    }
    if (field != 0u) {
        return BOUNCE_RTPL_ERR_MISSING_COMMAND_END;
    }
    if (!rtpl_bits_padding_is_clean(&reader)) {
        return BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING;
    }

    /* Absent style, tempo and volume instructions mean the specification
     * defaults, which all three original assets rely on for tempo and volume. */
    if (!song.style_present) {
        song.style = (BounceRtplStyle)RTPL_DEFAULT_STYLE;
    }
    if (!song.volume_present) {
        song.volume_level = RTPL_DEFAULT_VOLUME_LEVEL;
    }

    rtpl_song_finish(&song);
    *song_out = song;
    return status;
}

BounceRtplStatus bounce_rtpl_decode_file(const char *filesystem_path,
                                         BounceRtplSong *song_out)
{
    unsigned char buffer[RTPL_MAX_FILE_BYTES];
    FILE *file;
    size_t read_count;
    BounceRtplStatus status;

    if (song_out != NULL) {
        memset(song_out, 0, sizeof(*song_out));
    }
    if (filesystem_path == NULL || song_out == NULL) {
        return BOUNCE_RTPL_ERR_NULL_ARGUMENT;
    }
    file = fopen(filesystem_path, "rb");
    if (file == NULL) {
        return BOUNCE_RTPL_ERR_IO;
    }
    read_count = fread(buffer, 1u, sizeof(buffer), file);
    if (ferror(file) != 0) {
        (void)fclose(file);
        return BOUNCE_RTPL_ERR_IO;
    }
    /* A file at or beyond the buffer size is refused rather than truncated:
     * decoding a clipped prefix would be a silent lie. */
    if (read_count >= sizeof(buffer)) {
        (void)fclose(file);
        return BOUNCE_RTPL_ERR_IO;
    }
    (void)fclose(file);

    status = bounce_rtpl_decode(buffer, read_count, song_out);
    memset(buffer, 0, sizeof(buffer));
    return status;
}

void bounce_rtpl_print_song(const BounceRtplSong *song, FILE *output_file)
{
    unsigned int index;

    if (song == NULL || output_file == NULL) {
        return;
    }
    (void)fprintf(output_file,
        "  song-type=%s patterns=%u first-pattern-id=%u loop=%u%s\n",
        song->song_type == BOUNCE_RTPL_SONG_TYPE_BASIC ? "basic" : "unknown",
        song->pattern_count,
        song->first_pattern_id,
        song->first_pattern_loop_value,
        song->unexpanded_repeats ? " (repeats NOT expanded)" : "");
    (void)fprintf(output_file,
        "  tempo-index=%u tempo=%u BPM  style=%s%s  volume=%s%u\n",
        song->tempo_index,
        song->tempo_bpm,
        bounce_rtpl_style_name((unsigned int)song->style) != NULL
            ? bounce_rtpl_style_name((unsigned int)song->style) : "reserved",
        song->style_present ? "" : " (default, no instruction)",
        song->volume_present ? "level " : "default level ",
        song->volume_level);
    (void)fprintf(output_file,
        "  title-length=%u notes=%u total-note-duration=%u ms"
        " style-rest=%u (UNDEFINED BY FORMAT)\n",
        song->title_length,
        song->note_count,
        song->total_note_duration_ms,
        song->style_rest_duration_ms);
    for (index = 0u; index < song->note_count; ++index) {
        const BounceRtplNote *note = &song->notes[index];
        const char *name = bounce_rtpl_note_name(note->note_value);

        (void)fprintf(output_file,
            "  note[%u] value=%u name=%s scale=%u (%s) nominal=%0.2f Hz"
            " duration-code=%u specifier=%u duration=%u ms\n",
            index,
            note->note_value,
            name != NULL ? name : "?",
            note->scale,
            bounce_rtpl_scale_label(note->scale) != NULL
                ? bounce_rtpl_scale_label(note->scale) : "?",
            note->nominal_frequency_hz,
            note->duration_code,
            note->duration_specifier_code,
            note->duration_ms);
    }
}

/* ------------------------------------------------------------------------ */
/* Self-test                                                                  */
/* ------------------------------------------------------------------------ */

/*
 * The 18 original bytes of sounds/up.ott, transcribed here so the negative
 * tests are self-contained, deterministic and independent of the filesystem.
 * The same bytes are re-read from disk by the positive test, which is what
 * proves this transcription is still the original data.
 */
static const unsigned char RTPL_UP_OTT_BYTES[18] = {
    0x02, 0x4a, 0x3a, 0x40, 0x04, 0x00, 0x13, 0x1c, 0xc5,
    0x11, 0x82, 0x4a, 0xc0, 0xc4, 0x14, 0x46, 0x00, 0x00
};

/*
 * Bit offsets inside the original up.ott, established by STEP 14B-19 and
 * re-derived against the byte transcription above while writing this decoder.
 * Used only to build malformed variants.
 */
enum {
    BIT_COMMAND_LENGTH = 0,
    BIT_COMMAND_PART_0 = 8,
    BIT_COMMAND_PART_1 = 16,
    BIT_SONG_TYPE = 23,
    BIT_SEQUENCE_LENGTH = 30,
    BIT_PATTERN_SPECIFIER = 47,
    /* instruction 1 = tempo */
    BIT_INSTRUCTION_1_ID = 55,
    /* instruction 2 = style */
    BIT_INSTRUCTION_2_ID = 63,
    BIT_INSTRUCTION_2_VALUE = 66,
    /* instruction 3 = scale */
    BIT_INSTRUCTION_3_ID = 68,
    /* instruction 4 = first note */
    BIT_INSTRUCTION_4_ID = 73,
    BIT_INSTRUCTION_4_NOTE = 76,
    BIT_INSTRUCTION_4_DURATION = 80,
    /* instruction 5 = scale */
    BIT_INSTRUCTION_5_ID = 85,
    /* <command-end> and the trailing padding */
    BIT_COMMAND_END = 131,
    BIT_TRAILING_PADDING = 139
};

/* Test-only helper: overwrite a bit range. Not part of the decoder. */
static void rtpl_test_set_bits(unsigned char *buffer,
                               unsigned int bit_offset,
                               unsigned int count,
                               unsigned int value)
{
    unsigned int index;

    for (index = 0u; index < count; ++index) {
        unsigned int shift = 7u - ((bit_offset + index) % 8u);
        size_t byte_index = (bit_offset + index) / 8u;
        unsigned int mask = 1u << shift;
        unsigned int bit = (value >> (count - 1u - index)) & 1u;

        if (bit != 0u) {
            buffer[byte_index] = (unsigned char)(buffer[byte_index] | mask);
        } else {
            buffer[byte_index] = (unsigned char)(buffer[byte_index] & ~mask);
        }
    }
}

typedef struct RtplExpectedNote {
    unsigned int note_value;
    const char *name;
    /*
     * The ENCODED Table 3.8-9 code, not the human label number. The two differ
     * by one: "Scale-3" is the label in STEP 14B-19/20, and its encoded value
     * is 2 (binary 10). Asserting both is what keeps the two vocabularies from
     * being silently merged.
     */
    unsigned int scale;
    const char *scale_label;
    double nominal_hz;
    unsigned int duration_ms;
} RtplExpectedNote;

/*
 * The authoritative expected results from STEP 14B-19 (notes, scales,
 * durations) and STEP 14B-20 (nominal frequencies). These are transcribed
 * expectations, NOT values recomputed from the implementation, and they are
 * never adjusted to make a failing test pass.
 */
typedef struct RtplExpectedSong {
    const char *relative_path;
    size_t byte_length;
    unsigned int tempo_bpm;
    unsigned int note_count;
    unsigned int total_note_duration_ms;
    const RtplExpectedNote *notes;
} RtplExpectedSong;

static const RtplExpectedNote RTPL_EXPECT_UP_NOTES[4] = {
    {  1u, "C", 2u, "Scale-3", 1046.50, 120u },
    {  5u, "E", 1u, "Scale-2",  659.26, 120u },
    {  8u, "G", 1u, "Scale-2",  783.99, 120u },
    {  1u, "C", 2u, "Scale-3", 1046.50, 120u }
};

static const RtplExpectedNote RTPL_EXPECT_PICKUP_NOTES[3] = {
    {  1u, "C", 2u, "Scale-3", 1046.50, 240u },
    {  8u, "G", 1u, "Scale-2",  783.99, 120u },
    {  1u, "C", 2u, "Scale-3", 1046.50, 240u }
};

static const RtplExpectedNote RTPL_EXPECT_POP_NOTES[3] = {
    {  8u, "G", 2u, "Scale-3", 1567.98, 60u },
    { 12u, "H", 2u, "Scale-3", 1975.53, 60u },
    {  1u, "C", 1u, "Scale-2",  523.25, 120u }
};

static const RtplExpectedSong RTPL_EXPECTED_SONGS[3] = {
    {
        "sounds/up.ott", 18u, 125u, 4u, 480u, RTPL_EXPECT_UP_NOTES
    },
    {
        "sounds/pickup.ott", 16u, 125u, 3u, 600u, RTPL_EXPECT_PICKUP_NOTES
    },
    {
        "sounds/pop.ott", 16u, 125u, 3u, 240u, RTPL_EXPECT_POP_NOTES
    }
};

static bool rtpl_frequency_matches(double actual, double expected)
{
    double difference = actual - expected;

    if (difference < 0.0) {
        difference = -difference;
    }
    return difference <= (double)BOUNCE_RTPL_FREQUENCY_TOLERANCE_HZ;
}

static int rtpl_check_song(const RtplExpectedSong *expected,
                           const BounceRtplSong *song,
                           FILE *output_file)
{
    int failures = 0;
    unsigned int index;

    if (song->song_type != BOUNCE_RTPL_SONG_TYPE_BASIC) {
        ++failures;
    }
    if (song->pattern_count != 1u) {
        ++failures;
    }
    if (song->first_pattern_id != 0u || song->first_pattern_loop_value != 0u) {
        ++failures;
    }
    if (song->unexpanded_repeats) {
        ++failures;
    }
    if (song->title_length != 0u) {
        ++failures;
    }
    if (song->tempo_bpm != expected->tempo_bpm || song->tempo_index != 14u) {
        ++failures;
    }
    /* All three original assets carry an explicit Natural style instruction. */
    if (!song->style_present || song->style != BOUNCE_RTPL_STYLE_NATURAL) {
        ++failures;
    }
    if (song->volume_present) {
        ++failures;
    }
    if (song->style_rest_duration_ms != BOUNCE_RTPL_STYLE_REST_UNDEFINED) {
        ++failures;
    }
    if (song->note_count != expected->note_count) {
        ++failures;
    }
    if (song->total_note_duration_ms != expected->total_note_duration_ms) {
        ++failures;
    }
    if (song->note_count > expected->note_count) {
        return failures;
    }
    for (index = 0u; index < expected->note_count; ++index) {
        const BounceRtplNote *note = &song->notes[index];
        const char *actual_name = bounce_rtpl_note_name(note->note_value);
        const char *actual_scale = bounce_rtpl_scale_label(note->scale);

        if (note->order != index
            || note->note_value != expected->notes[index].note_value
            || note->scale != expected->notes[index].scale
            || actual_scale == NULL
            || strcmp(actual_scale, expected->notes[index].scale_label) != 0
            || note->duration_ms != expected->notes[index].duration_ms
            || note->is_pause
            || actual_name == NULL
            || strcmp(actual_name, expected->notes[index].name) != 0
            || !rtpl_frequency_matches(note->nominal_frequency_hz,
                                       expected->notes[index].nominal_hz)) {
            ++failures;
        }
    }
    if (output_file != NULL) {
        (void)fprintf(output_file, "  expected: notes=%u tempo=%u style=Natural"
            " total-note-duration=%u ms\n",
            expected->note_count, expected->tempo_bpm,
            expected->total_note_duration_ms);
        bounce_rtpl_print_song(song, output_file);
    }
    return failures;
}

static int rtpl_expect_failure(const char *label,
                               const unsigned char *data,
                               size_t length,
                               BounceRtplStatus expected_status,
                               FILE *output_file)
{
    BounceRtplSong song;
    BounceRtplStatus status = bounce_rtpl_decode(data, length, &song);

    if (status != expected_status) {
        if (output_file != NULL) {
            (void)fprintf(output_file,
                "  FAIL %-34s expected \"%s\", got \"%s\"\n",
                label,
                bounce_rtpl_status_text(expected_status),
                bounce_rtpl_status_text(status));
        }
        return 1;
    }
    if (output_file != NULL) {
        (void)fprintf(output_file, "  ok   %-34s rejected: %s\n",
                      label, bounce_rtpl_status_text(status));
    }
    return 0;
}

static int rtpl_run_negative_tests(FILE *output_file)
{
    unsigned char buffer[sizeof(RTPL_UP_OTT_BYTES) + 2u];
    int failures = 0;
    BounceRtplSong song;

    if (output_file != NULL) {
        (void)fprintf(output_file, "negative tests (malformed input must be"
            " rejected, never guessed):\n");
    }

    /* NULL and empty arguments. */
    if (bounce_rtpl_decode(NULL, 18u, &song)
        != BOUNCE_RTPL_ERR_NULL_ARGUMENT) {
        ++failures;
    }
    if (bounce_rtpl_decode(RTPL_UP_OTT_BYTES, 18u, NULL)
        != BOUNCE_RTPL_ERR_NULL_ARGUMENT) {
        ++failures;
    }
    if (bounce_rtpl_decode(RTPL_UP_OTT_BYTES, 0u, &song)
        != BOUNCE_RTPL_ERR_EMPTY_INPUT) {
        ++failures;
    }
    if (bounce_rtpl_decode(RTPL_UP_OTT_BYTES, 1u, &song)
        != BOUNCE_RTPL_ERR_TRUNCATED) {
        ++failures;
    }
    if (bounce_rtpl_decode_file(NULL, &song)
        != BOUNCE_RTPL_ERR_NULL_ARGUMENT) {
        ++failures;
    }
    if (bounce_rtpl_decode_file("no/such/file.ott", &song)
        != BOUNCE_RTPL_ERR_IO) {
        ++failures;
    }
    if (failures == 0 && output_file != NULL) {
        (void)fprintf(output_file, "  ok   %-34s rejected: null/empty/io\n",
                      "null and empty arguments");
    } else if (failures != 0 && output_file != NULL) {
        (void)fprintf(output_file, "  FAIL %-34s\n",
                      "null and empty arguments");
    }

    /* Truncation: the command-end, then a much shorter prefix. */
    failures += rtpl_expect_failure("truncated: 17 of 18 bytes",
        RTPL_UP_OTT_BYTES, 17u, BOUNCE_RTPL_ERR_TRUNCATED, output_file);
    failures += rtpl_expect_failure("truncated: 10 of 18 bytes",
        RTPL_UP_OTT_BYTES, 10u, BOUNCE_RTPL_ERR_TRUNCATED, output_file);
    failures += rtpl_expect_failure("truncated: 6 of 18 bytes",
        RTPL_UP_OTT_BYTES, 6u, BOUNCE_RTPL_ERR_TRUNCATED, output_file);

    /* Command length. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_LENGTH, 8u, 0u);
    failures += rtpl_expect_failure("command-length 0",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_COMMAND_LENGTH, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_LENGTH, 8u, 3u);
    failures += rtpl_expect_failure("command-length 3 (not 2 parts)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_COMMAND_LENGTH, output_file);

    /* Command parts: the byte-level 0x02 0x4A magic. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_PART_0, 7u, 0x24u);
    failures += rtpl_expect_failure("bad magic: part0 != 0x25",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_COMMAND_PART, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_PART_1, 7u, 0x1cu);
    failures += rtpl_expect_failure("bad command part: part1 != 0x1d",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_COMMAND_PART, output_file);

    /* Song type. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_SONG_TYPE, 3u, 3u);
    failures += rtpl_expect_failure("song-type 011 (midi, reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_UNSUPPORTED_SONG_TYPE, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_SONG_TYPE, 3u, 2u);
    failures += rtpl_expect_failure("song-type 010 (temporary)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_UNSUPPORTED_SONG_TYPE, output_file);

    /* Song sequence length. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_SEQUENCE_LENGTH, 8u, 0u);
    failures += rtpl_expect_failure("song-sequence-length 0",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_PATTERN_SEQUENCE, output_file);

    /* Pattern specifier 0 means "reuse a defined pattern", not an empty one. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_PATTERN_SPECIFIER, 8u, 0u);
    failures += rtpl_expect_failure("pattern-specifier 0 (reuse)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_ALREADY_DEFINED_PATTERN, output_file);

    /* Instruction ids. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_5_ID, 3u, 6u);
    failures += rtpl_expect_failure("instruction id 110 (undefined)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_1_ID, 3u, 7u);
    failures += rtpl_expect_failure("instruction id 111 (undefined)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID, output_file);

    /* Malformed note sequence: a RESERVED note value. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_4_NOTE, 4u, 13u);
    failures += rtpl_expect_failure("note-value 1101 (reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_RESERVED_NOTE_VALUE, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_4_NOTE, 4u, 15u);
    failures += rtpl_expect_failure("note-value 1111 (reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_RESERVED_NOTE_VALUE, output_file);

    /* Out-of-range field: a RESERVED note duration. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_4_DURATION, 3u, 6u);
    failures += rtpl_expect_failure("duration-code 110 (reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_RESERVED_DURATION, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_4_DURATION, 3u, 7u);
    failures += rtpl_expect_failure("duration-code 111 (reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_RESERVED_DURATION, output_file);

    /* Out-of-range field: a RESERVED style value. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_INSTRUCTION_2_VALUE, 2u, 3u);
    failures += rtpl_expect_failure("style-value 11 (reserved)",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_RESERVED_STYLE, output_file);

    /* Missing / illegal command termination. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_END, 8u, 1u);
    failures += rtpl_expect_failure("command-end non-zero",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_MISSING_COMMAND_END, output_file);

    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_COMMAND_END + 7u, 1u, 1u);
    failures += rtpl_expect_failure("command-end last bit set",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_MISSING_COMMAND_END, output_file);

    /* Trailing padding. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    rtpl_test_set_bits(buffer, BIT_TRAILING_PADDING, 1u, 1u);
    failures += rtpl_expect_failure("trailing padding bit set",
        buffer, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING, output_file);

    /* Extra whole octets of trailing data. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    buffer[sizeof(RTPL_UP_OTT_BYTES)] = 0x00u;
    buffer[sizeof(RTPL_UP_OTT_BYTES) + 1u] = 0x00u;
    failures += rtpl_expect_failure("2 extra trailing octets",
        buffer, sizeof(RTPL_UP_OTT_BYTES) + 2u,
        BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING, output_file);

    /* A single extra octet is still too much: padding is at most 7 bits. */
    memcpy(buffer, RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES));
    buffer[sizeof(RTPL_UP_OTT_BYTES)] = 0x00u;
    failures += rtpl_expect_failure("1 extra trailing octet",
        buffer, sizeof(RTPL_UP_OTT_BYTES) + 1u,
        BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING, output_file);

    /* The unmutated transcription must still decode, proving the mutations
     * above are the only difference and the transcription is faithful. */
    failures += rtpl_expect_failure("unmutated control (must NOT be rejected)",
        RTPL_UP_OTT_BYTES, sizeof(RTPL_UP_OTT_BYTES),
        BOUNCE_RTPL_OK, output_file);

    return failures;
}

int bounce_rtpl_verify_original_assets(const char *resource_root,
                                       FILE *output_file)
{
    int failures = 0;
    unsigned int index;

    if (resource_root == NULL) {
        return -1;
    }
    for (index = 0u; index < 3u; ++index) {
        const RtplExpectedSong *expected = &RTPL_EXPECTED_SONGS[index];
        char path[512];
        BounceRtplSong song;
        BounceRtplStatus status;
        int song_failures;
        int written = snprintf(path, sizeof(path), "%s/%s",
                               resource_root, expected->relative_path);

        if (output_file != NULL) {
            (void)fprintf(output_file, "asset: %s\n", expected->relative_path);
        }
        if (written < 0 || (size_t)written >= sizeof(path)) {
            ++failures;
            continue;
        }
        status = bounce_rtpl_decode_file(path, &song);
        if (status != BOUNCE_RTPL_OK) {
            ++failures;
            if (output_file != NULL) {
                (void)fprintf(output_file, "  FAIL decode: %s\n",
                              bounce_rtpl_status_text(status));
            }
            continue;
        }
        song_failures = rtpl_check_song(expected, &song, output_file);
        failures += song_failures;
        if (output_file != NULL) {
            (void)fprintf(output_file, "  %s (%d field mismatches)\n",
                          song_failures == 0 ? "PASS" : "FAIL", song_failures);
        }
    }

    /* Cross-check the transcription used by the negative tests against the file
     * on disk, so a divergence cannot silently weaken those tests. Uses its own
     * failure count so an earlier asset's failure cannot mislabel this. */
    {
        BounceRtplSong from_file;
        BounceRtplSong from_bytes;
        char path[512];
        int written = snprintf(path, sizeof(path), "%s/sounds/up.ott",
                               resource_root);
        int cross_failures = 0;

        if (written < 0 || (size_t)written >= sizeof(path)
            || bounce_rtpl_decode_file(path, &from_file) != BOUNCE_RTPL_OK
            || bounce_rtpl_decode(RTPL_UP_OTT_BYTES,
                                  sizeof(RTPL_UP_OTT_BYTES),
                                  &from_bytes) != BOUNCE_RTPL_OK) {
            cross_failures = 1;
        } else {
            unsigned int note;

            if (from_file.note_count != from_bytes.note_count) {
                cross_failures = 1;
            }
            for (note = 0u;
                 note < from_file.note_count && note < from_bytes.note_count;
                 ++note) {
                if (from_file.notes[note].note_value != from_bytes.notes[note].note_value
                    || from_file.notes[note].scale != from_bytes.notes[note].scale
                    || from_file.notes[note].duration_ms != from_bytes.notes[note].duration_ms) {
                    cross_failures = 1;
                }
            }
        }
        failures += cross_failures;
        if (output_file != NULL) {
            (void)fprintf(output_file,
                "negative-test transcription matches sounds/up.ott: %s\n",
                cross_failures == 0 ? "PASS" : "FAIL");
        }
    }

    failures += rtpl_run_negative_tests(output_file);

    if (output_file != NULL) {
        (void)fprintf(output_file,
            "RTPL self-test: %s (%d failures)\n"
            "no audio backend was opened; no PCM was generated\n",
            failures == 0 ? "PASS" : "FAIL", failures);
    }
    return failures == 0 ? 0 : -1;
}
