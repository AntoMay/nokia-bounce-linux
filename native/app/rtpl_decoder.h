#ifndef BOUNCE_NATIVE_APP_RTPL_DECODER_H
#define BOUNCE_NATIVE_APP_RTPL_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decoder for the ORIGINAL Nokia Bounce `.ott` audio resources.
 *
 * FORMAT: Nokia Smart Messaging OTA Ringing Tone, i.e. the bit grammar of
 * "Smart Messaging Specification" Revision 2.0.0 (Nokia Mobile Phones Ltd.,
 * 1999-05-17) section 3.8 "RINGING TONES", tables 3.8-1 .. 3.8-12.
 *
 * PROVENANCE OF EVERY RULE BELOW
 *   - Section 4, level 1: `Sound(byte[] data, Sound.FORMAT_TONE)` in
 *     `e.java:LoadSound` is how the original game loads `/sounds/up.ott`,
 *     `/sounds/pickup.ott` and `/sounds/pop.ott`, and FORMAT_TONE is defined
 *     by that same Javadoc as the OTA ringtone format.
 *   - Section 5, level 2: the Rev 2.0.0 grammar and bit tables.
 *   - Section 5, level 3: STEP 14B-19 decoded all three original assets and
 *     achieved a BYTE-EXACT RE-ENCODE of each (18/18, 16/16, 16/16 bytes), so
 *     the field widths used here are proven against the original data rather
 *     than assumed. The two widths that required correction are:
 *         * <command-part> is SEVEN bits, not five (spec tables print
 *           `0000 101` = 7 characters). Confirmed by Gammu's
 *           `BufferAlign(package, &StartBit)` between the two 7-bit writes
 *           with the comment "According to specification we need have next
 *           part octet-aligned".
 *         * <beats-per-minute> is FIVE bits, not four (32 table rows).
 *   - STEP 14B-20 proved the note/scale -> nominal-frequency mapping against
 *     four independent sources with 0.0 Hz maximum deviation.
 *
 * SCOPE / IMPLEMENTATION BOUNDARY -- READ BEFORE USE
 *   This module decodes BYTES INTO EVENTS. It performs no audio output of any
 *   kind. It opens no ALSA, PulseAudio, PipeWire, SDL or MIDI resource, it
 *   generates no PCM, and it synthesises no waveform.
 *
 *   It therefore carries, for every field, only information the source
 *   justifies. It deliberately contains NO waveform, NO duty cycle, NO
 *   amplitude or dB mapping, NO speaker model and NO oscillator assumption,
 *   because Smart Messaging Rev 2.0.0 specifies none of those (STEP 14B-22
 *   established: 0 occurrences of `square`, `sine`, `pulse`, `waveform`,
 *   `duty`, `harmonic`, `buzzer`, `oscillat*`, `speaker`, `wave`, `timbre`,
 *   `amplitude`, `envelope`, `attack` and `decay` in the entire
 *   specification).
 *
 *   "NOMINAL FREQUENCY" in this header means the frequency the FORMAT's own
 *   scale model defines for a note. It is NOT a claim about what any physical
 *   Nokia handset emitted; the real oscillator grid is
 *   HARDWARE-FREQUENCY-UNKNOWN (STEP 14B-20).
 *
 * TERMINOLOGY
 *   Scale codes are RETAINED IN THE ENCODED FORM of Table 3.8-9 (0=Scale-1,
 *   1=Scale-2, 2=Scale-3, 3=Scale-4) and are never silently renamed to
 *   scientific octaves. Scientific labels are presentation only.
 */

enum {
    /*
     * Upper bound on decoded notes. The largest original asset holds 4 notes
     * and the format's own <length-of-the-new-pattern> is one octet, so 64 is
     * a comfortable bound that still makes overflow a reportable error rather
     * than an allocation.
     */
    BOUNCE_RTPL_MAX_NOTES = 64
};

/* Table 3.8-2. Only <basic-song-type> (001) is decoded; the other three are
 * documented as reserved for future extension and are rejected, not guessed. */
typedef enum BounceRtplSongType {
    BOUNCE_RTPL_SONG_TYPE_INVALID = 0,
    BOUNCE_RTPL_SONG_TYPE_BASIC = 1
} BounceRtplSongType;

/* Table 3.8-10, kept in ENCODED order. Style 3 is RESERVED in the
 * specification and is rejected rather than interpreted. */
typedef enum BounceRtplStyle {
    BOUNCE_RTPL_STYLE_NATURAL = 0,   /* "rest between notes", specification default */
    BOUNCE_RTPL_STYLE_CONTINUOUS = 1, /* "no rest between notes" */
    BOUNCE_RTPL_STYLE_STACCATO = 2,  /* "shorter notes and longer rest period" */
    BOUNCE_RTPL_STYLE_RESERVED = 3
} BounceRtplStyle;

/* Table 3.8-7. Codes 6 and 7 are RESERVED and are rejected. */
typedef enum BounceRtplDuration {
    BOUNCE_RTPL_DURATION_FULL = 0,
    BOUNCE_RTPL_DURATION_HALF = 1,
    BOUNCE_RTPL_DURATION_QUARTER = 2,
    BOUNCE_RTPL_DURATION_EIGHTH = 3,
    BOUNCE_RTPL_DURATION_SIXTEENTH = 4,
    BOUNCE_RTPL_DURATION_THIRTY_SECOND = 5
} BounceRtplDuration;

/* Table 3.8-8. */
typedef enum BounceRtplDurationSpecifier {
    BOUNCE_RTPL_DURATION_SPECIFIER_NONE = 0,   /* factor 1 */
    BOUNCE_RTPL_DURATION_SPECIFIER_DOTTED = 1, /* factor 3/2 */
    BOUNCE_RTPL_DURATION_SPECIFIER_DOUBLE_DOTTED = 2, /* factor 7/4 */
    BOUNCE_RTPL_DURATION_SPECIFIER_TWO_THIRDS = 3      /* factor 2/3 */
} BounceRtplDurationSpecifier;

enum {
    /*
     * IMPLEMENTATION-NEUTRAL PLACEHOLDER.
     *
     * Smart Messaging Rev 2.0.0 Table 3.8-10 says only that Natural style has
     * a "rest between notes" and that Staccato has a "longer rest period".
     * No magnitude is given anywhere in the specification, in the Nokia UI API
     * Javadoc, or in any implementation that claims Nokia provenance (STEP
     * 14B-22). A rest EXISTS; its LENGTH IS UNDEFINED BY THE FORMAT.
     *
     * `style_rest_duration_ms` in BounceRtplSong is therefore reserved for a
     * later step's explicitly-declared choice and is ALWAYS this value here.
     * It carries no semantics, must not be read as a recovered original value,
     * and must not be presented as one. A future step may set it to a chosen
     * value ONLY together with a written declaration that the value is a
     * RECONSTRUCTION.
     */
    BOUNCE_RTPL_STYLE_REST_UNDEFINED = 0
};

/* One decoded note event. Contains only source-justified information. */
typedef struct BounceRtplNote {
    /* 0-based position in the song's ordered note list. */
    unsigned int order;
    /* Raw Table 3.8-6 code: 0=pause, 1=C .. 12=H, 13..15 RESERVED. */
    unsigned int note_value;
    /* Raw Table 3.8-9 code in force for this note: 0=Scale-1 (A=440 Hz),
     * 1=Scale-2 (A=880 Hz, specification default), 2=Scale-3 (A=1760 Hz),
     * 3=Scale-4 (A=3520 Hz). Retained encoded, never renormalised. */
    unsigned int scale;
    /* Raw Table 3.8-7 code (enum BounceRtplDuration). */
    unsigned int duration_code;
    /* Raw Table 3.8-8 code (enum BounceRtplDurationSpecifier). */
    unsigned int duration_specifier_code;
    /* Derived: tempo x duration code x duration specifier, in milliseconds.
     * This is note SOUND time only; it excludes any rest. */
    unsigned int duration_ms;
    /* Derived from note_value + scale by the specification's own 12-TET scale
     * model. NOMINAL, not hardware. */
    double nominal_frequency_hz;
    /* True when note_value == 0 (Table 3.8-6 "pause"). */
    bool is_pause;
} BounceRtplNote;

/* One fully decoded basic song. */
typedef struct BounceRtplSong {
    BounceRtplSongType song_type;
    /* Table 3.8-1 <text-length>. All three original assets are 0 (unnamed). */
    unsigned int title_length;
    /* <song-sequence-length>; the original assets are 1. */
    unsigned int pattern_count;
    /* Raw Table 3.8-3 <pattern-id> of the first pattern: 0=A .. 3=D part. */
    unsigned int first_pattern_id;
    /* Raw <loop-value> of the first pattern. 0 means no repeat. */
    unsigned int first_pattern_loop_value;
    /*
     * True when any decoded pattern had <loop-value> != 0, i.e. the format
     * asked for repetitions that this decoder does NOT expand. No original
     * Bounce asset needs expansion (all three use loop 0), so this is a
     * completeness flag for future callers, not a known gap in the originals.
     */
    bool unexpanded_repeats;
    /* Raw 5-bit Table 3.8-11 index, and the BPM it maps to. */
    unsigned int tempo_index;
    unsigned int tempo_bpm;
    /* True when a <style-instruction> was present. Absent means the
     * specification default, which is Natural. */
    bool style_present;
    BounceRtplStyle style;
    /* True when a <volume-instruction> was present. Absent means the
     * specification default level-7. NO level-to-amplitude or dB mapping is
     * derived: Table 3.8-12 defines 16 dimensionless levels only. */
    bool volume_present;
    unsigned int volume_level;
    unsigned int note_count;
    BounceRtplNote notes[BOUNCE_RTPL_MAX_NOTES];
    /* Sum of notes[i].duration_ms. Excludes any rest, because no rest
     * magnitude is defined by the format. */
    unsigned int total_note_duration_ms;
    /* ALWAYS BOUNCE_RTPL_STYLE_REST_UNDEFINED. See the enum note above. */
    unsigned int style_rest_duration_ms;
} BounceRtplSong;

/* Decode outcomes. Every rejection is reported, never silently repaired. */
typedef enum BounceRtplStatus {
    BOUNCE_RTPL_OK = 0,
    BOUNCE_RTPL_ERR_NULL_ARGUMENT = 1,
    BOUNCE_RTPL_ERR_EMPTY_INPUT = 2,
    BOUNCE_RTPL_ERR_TRUNCATED = 3,
    BOUNCE_RTPL_ERR_BAD_MAGIC = 4,
    BOUNCE_RTPL_ERR_BAD_COMMAND_LENGTH = 5,
    BOUNCE_RTPL_ERR_BAD_COMMAND_PART = 6,
    BOUNCE_RTPL_ERR_UNSUPPORTED_SONG_TYPE = 7,
    BOUNCE_RTPL_ERR_BAD_PATTERN_SEQUENCE = 8,
    BOUNCE_RTPL_ERR_ALREADY_DEFINED_PATTERN = 9,
    BOUNCE_RTPL_ERR_BAD_INSTRUCTION_ID = 10,
    BOUNCE_RTPL_ERR_RESERVED_NOTE_VALUE = 11,
    BOUNCE_RTPL_ERR_RESERVED_DURATION = 12,
    BOUNCE_RTPL_ERR_RESERVED_STYLE = 13,
    BOUNCE_RTPL_ERR_TOO_MANY_NOTES = 14,
    BOUNCE_RTPL_ERR_MISSING_COMMAND_END = 15,
    BOUNCE_RTPL_ERR_BAD_TRAILING_PADDING = 16,
    BOUNCE_RTPL_ERR_IO = 17
} BounceRtplStatus;

/* Human-readable, stable text for a status value. Never NULL. */
const char *bounce_rtpl_status_text(BounceRtplStatus status);

/*
 * Decode a `.ott` payload in memory.
 *
 * `data` is the BARE ringing-tone payload only: the original assets contain no
 * OTA transport wrapper. `length` is its exact byte count.
 *
 * On success 0 is returned and `song_out` is fully populated. On any failure
 * a non-zero BounceRtplStatus is returned and `song_out` is left zeroed, so a
 * caller can never observe a partially decoded song.
 *
 * Performs no allocation, no I/O and no unbounded read: every field is read
 * through a bounds-checked MSB-first bit reader, and the reader refuses to
 * advance past the end of `data`.
 */
BounceRtplStatus bounce_rtpl_decode(
    const unsigned char *data,
    size_t length,
    BounceRtplSong *song_out
);

/* Read one file into memory and decode it. Same contract as above; adds
 * BOUNCE_RTPL_ERR_IO when the file cannot be opened or read. */
BounceRtplStatus bounce_rtpl_decode_file(
    const char *filesystem_path,
    BounceRtplSong *song_out
);

/*
 * Derive a note's nominal frequency from a Table 3.8-9 scale code and a
 * Table 3.8-6 note code, using the specification's own scale model
 * (f = scale_A * 2^((note - 10) / 12), with scale_A 440/880/1760/3520 Hz for
 * scales 0..3 and note 10 = A).
 *
 * Returns a negative value for an out-of-range scale or note, so a caller can
 * distinguish "no such note" from "that note is at frequency 0".
 *
 * This is a NOMINAL frequency. It is not a hardware frequency claim.
 */
double bounce_rtpl_nominal_frequency_hz(unsigned int scale, unsigned int note_value);

/* Table 3.8-6 note name, or NULL for a reserved/out-of-range code. */
const char *bounce_rtpl_note_name(unsigned int note_value);

/* Table 3.8-9 scale label, or NULL when out of range. */
const char *bounce_rtpl_scale_label(unsigned int scale);

/* Table 3.8-10 style name, or NULL for the reserved code. */
const char *bounce_rtpl_style_name(unsigned int style);

/*
 * Print one decoded song as a deterministic multi-line block: song type,
 * tempo, style, volume, and then one line per note with its encoded scale,
 * scale label, note name, nominal frequency and duration, followed by the
 * total. Safe with a NULL song (prints nothing) and a NULL file.
 */
void bounce_rtpl_print_song(const BounceRtplSong *song, FILE *output_file);

/*
 * Self-test: decode the three ORIGINAL resources under `resource_root`
 * (for example "src/main/resources") and assert every value proven by
 * STEP 14B-19 and STEP 14B-20, then run the malformed-input rejection tests.
 *
 * Expected results asserted (these are the authoritative values; they are not
 * recomputed from the implementation and are never adjusted to match it):
 *
 *   sounds/up.ott     4 notes  C E G C   scales 3 2 2 3
 *                     120 120 120 120 ms   total 480 ms
 *   sounds/pickup.ott 3 notes  C G C     scales 3 2 3
 *                     240 120 240 ms       total 600 ms
 *   sounds/pop.ott    3 notes  G H C     scales 3 3 2
 *                     60 60 120 ms         total 240 ms
 *
 *   all three: basic song, 125 BPM, Natural style, no volume instruction,
 *              1 pattern (A-part), loop 0.
 *
 * Frequency tolerance is BOUNCE_RTPL_FREQUENCY_TOLERANCE_HZ below, which is
 * far tighter than the specification's own integer-rounded frequency table.
 *
 * Returns 0 when every assertion passed, -1 otherwise. When `output_file` is
 * non-NULL a per-asset report and every negative-test outcome are printed to
 * it; nothing is written to stdout or stderr by this function.
 */
int bounce_rtpl_verify_original_assets(const char *resource_root, FILE *output_file);

/*
 * Accepted |nominal frequency - expected| for the self-test, in Hz.
 * The specification's own table rounds to whole Hz (it lists C2 as 1047
 * and G2 as 1568), so 0.05 Hz is roughly two orders of magnitude tighter
 * than the source precision and still leaves the comparison meaningful.
 */
#define BOUNCE_RTPL_FREQUENCY_TOLERANCE_HZ 0.05

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_RTPL_DECODER_H */
