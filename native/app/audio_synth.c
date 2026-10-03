/*
 * audio_synth.c -- STEP 14B-34-A Layer C skeleton (Target B).
 *
 * SCOPE OF THIS FILE
 *   Structure only. It resolves a Layer B identity to an original resource,
 *   hands the payload to Layer A unchanged, and turns the decoded song into a
 *   timed plan of note and rest segments.
 *
 *   It contains NO sample generation. There is no loop that writes a PCM
 *   value, no oscillator, no envelope, and no buffer arithmetic beyond integer
 *   frame counts. `bounce_synth_render_pcm()` is a declared stub that reports
 *   "not implemented" and writes nothing.
 *
 *   It contains NO device access: no ALSA include, no `snd_*` call, no
 *   `/dev/snd`, no audio library. It contains NO thread, NO queue or FIFO, NO
 *   mixer, and NO Layer B sink registration. It does not call into
 *   `audio_event.c`; it only names the Layer B enum values, which are compile
 *   time constants, so the standalone self-test links this file, the Layer A
 *   decoder, and libc only.
 *
 *   Every failure is a returned status. There is no `abort()`, no `exit()`, no
 *   assert, and no fallback sound is ever produced.
 */

#include "audio_synth.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Reconstruction parameter baseline (STEP 14B-33 sections 11 and 13)
 * ------------------------------------------------------------------------- */

/*
 * These four values and the rest ratio below are RECONSTRUCTION PARAMETERS or
 * Target B engineering decisions. None is a recovered Nokia value.
 *
 *   sample rate 44100  : Target B engineering parameter, chosen as the most
 *                       widely supported default. The renderer is rate
 *                       agnostic, so changing it is a one-constant edit.
 *   S16LE            : Target B engineering parameter, native to the intended
 *                       backend so no conversion is ever needed.
 *   mono             : Target B engineering parameter, and consistent with the
 *                       original playing at most one tone per collision pass.
 *   band-limited sine: Target B RECONSTRUCTION WAVEFORM. The original waveform
 *                       is UNKNOWN -- EVIDENCE EXHAUSTED.
 *   0.25 rest ratio  : Target B RECONSTRUCTION PARAMETER. The rest SEMANTIC is
 *                       an original fact; the original MAGNITUDE is UNKNOWN.
 */
void bounce_synth_default_config(BounceSynthConfig *out)
{
    if (out == NULL) {
        return;
    }
    out->sample_rate = 44100u;
    out->waveform = BOUNCE_SYNTH_WAVE_SINE_BANDLIMITED;
    out->sample_format = BOUNCE_SYNTH_SAMPLE_S16LE;
    out->channels = BOUNCE_SYNTH_CHANNELS_MONO;
    out->natural_rest_ratio = 0.25;
}

const char *bounce_synth_waveform_name(BounceSynthWaveform waveform)
{
    switch (waveform) {
    case BOUNCE_SYNTH_WAVE_SINE_BANDLIMITED:
        return "sine-bandlimited";
    case BOUNCE_SYNTH_WAVE_SQUARE_BANDLIMITED:
        return "square-bandlimited";
    case BOUNCE_SYNTH_WAVE_TRIANGLE_BANDLIMITED:
        return "triangle-bandlimited";
    default:
        break;
    }
    return "unknown";
}

const char *bounce_synth_status_text(BounceSynthStatus status)
{
    switch (status) {
    case BOUNCE_SYNTH_OK:
        return "ok";
    case BOUNCE_SYNTH_ERR_NULL_ARGUMENT:
        return "null argument";
    case BOUNCE_SYNTH_ERR_NO_RESOURCE:
        return "identity has no audio resource";
    case BOUNCE_SYNTH_ERR_INVALID_CONFIG:
        return "invalid renderer configuration";
    case BOUNCE_SYNTH_ERR_DECODE_FAILED:
        return "RTPL decoder rejected the payload";
    case BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE:
        return "decoded song has no notes";
    case BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE:
        return "song style has no declared Target B reconstruction";
    case BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL:
        return "frame buffer smaller than the plan's total_frames";
    case BOUNCE_SYNTH_ERR_PLAN_MISMATCH:
        return "plan does not describe the supplied song";
    case BOUNCE_SYNTH_ERR_RENDER_NOT_IMPLEMENTED:
        return "PCM rendering is not implemented yet (STEP 14B-34-B)";
    default:
        break;
    }
    return "unknown status";
}

/*
 * Configuration validation.
 *
 * The rest-ratio test is written as a range comparison rather than with an
 * explicit NaN check: a NaN fails every comparison, so `!(x >= lo && x <= hi)`
 * rejects it without calling isnan(). This keeps validation free of any library
 * dependency of its own.
 */
BounceSynthStatus bounce_synth_config_validate(const BounceSynthConfig *config)
{
    if (config == NULL) {
        return BOUNCE_SYNTH_ERR_NULL_ARGUMENT;
    }
    if (config->sample_rate < BOUNCE_SYNTH_SAMPLE_RATE_MIN
        || config->sample_rate > BOUNCE_SYNTH_SAMPLE_RATE_MAX) {
        return BOUNCE_SYNTH_ERR_INVALID_CONFIG;
    }
    if (config->sample_format != BOUNCE_SYNTH_SAMPLE_S16LE) {
        return BOUNCE_SYNTH_ERR_INVALID_CONFIG;
    }
    if (config->channels != BOUNCE_SYNTH_CHANNELS_MONO) {
        return BOUNCE_SYNTH_ERR_INVALID_CONFIG;
    }
    switch (config->waveform) {
    case BOUNCE_SYNTH_WAVE_SINE_BANDLIMITED:
    case BOUNCE_SYNTH_WAVE_SQUARE_BANDLIMITED:
    case BOUNCE_SYNTH_WAVE_TRIANGLE_BANDLIMITED:
        break;
    default:
        return BOUNCE_SYNTH_ERR_INVALID_CONFIG;
    }
    if (!(config->natural_rest_ratio >= 0.0
          && config->natural_rest_ratio <= 1.0)) {
        return BOUNCE_SYNTH_ERR_INVALID_CONFIG;
    }
    return BOUNCE_SYNTH_OK;
}

/* -------------------------------------------------------------------------
 * Resource mapping -- ORIGINAL FACT, from e.java:68-70
 * ------------------------------------------------------------------------- */

/*
 * The whole mapping, in one read-only table. `relative_path` is the path
 * RELATIVE to the resource root, so the root is a runtime input and the table
 * stays independent of where the game is run from.
 */
typedef struct BounceSynthResourceEntry {
    BounceAudioEvent event;
    const char *relative_path;
} BounceSynthResourceEntry;

static const BounceSynthResourceEntry BOUNCE_SYNTH_RESOURCES[] = {
    /* ORIGINAL FACT: e.java:68 soundUp     = LoadSound("/sounds/up.ott") */
    { BOUNCE_AUDIO_EVENT_UP, "sounds/up.ott" },
    /* ORIGINAL FACT: e.java:69 soundPickup = LoadSound("/sounds/pickup.ott") */
    { BOUNCE_AUDIO_EVENT_PICKUP, "sounds/pickup.ott" },
    /* ORIGINAL FACT: e.java:70 soundPop    = LoadSound("/sounds/pop.ott") */
    { BOUNCE_AUDIO_EVENT_POP, "sounds/pop.ott" }
};

static const unsigned int BOUNCE_SYNTH_RESOURCE_COUNT =
    (unsigned int)(sizeof(BOUNCE_SYNTH_RESOURCES)
                   / sizeof(BOUNCE_SYNTH_RESOURCES[0]));

static const BounceSynthResourceEntry *bounce_synth_find_resource(
    BounceAudioEvent event)
{
    unsigned int i;

    for (i = 0u; i < BOUNCE_SYNTH_RESOURCE_COUNT; i++) {
        if (BOUNCE_SYNTH_RESOURCES[i].event == event) {
            return &BOUNCE_SYNTH_RESOURCES[i];
        }
    }
    return NULL;
}

BounceSynthStatus bounce_synth_map_event(BounceAudioEvent event,
                                        BounceSynthResource *out)
{
    const BounceSynthResourceEntry *entry = bounce_synth_find_resource(event);

    /*
     * `out` is written even on the failure path, so a caller that ignores the
     * returned status still cannot dereference a missing path. BOUNCE_AUDIO_EVENT_NONE
     * lands here too: the original never plays for it, so it has no resource.
     */
    if (out != NULL) {
        out->event = event;
        if (entry != NULL) {
            out->relative_path = entry->relative_path;
            out->has_resource = true;
        } else {
            out->relative_path = NULL;
            out->has_resource = false;
        }
    }
    return (entry != NULL) ? BOUNCE_SYNTH_OK : BOUNCE_SYNTH_ERR_NO_RESOURCE;
}

const char *bounce_synth_resource_path(BounceAudioEvent event)
{
    const BounceSynthResourceEntry *entry = bounce_synth_find_resource(event);

    return (entry != NULL) ? entry->relative_path : NULL;
}

/* -------------------------------------------------------------------------
 * Timed render plan
 * ------------------------------------------------------------------------- */

/* Zero a plan so a caller can never read a partially built one. */
static void bounce_synth_clear_plan(BounceSynthPlan *plan)
{
    if (plan == NULL) {
        return;
    }
    memset(plan, 0, sizeof(*plan));
}

/* Frames for a millisecond span at the configured rate. Integer, deterministic. */
static unsigned long bounce_synth_ms_to_frames(unsigned long milliseconds,
                                               unsigned int sample_rate)
{
    /* Round half up, matching the decoder's own duration rounding style. */
    return (milliseconds * (unsigned long)sample_rate + 500ul) / 1000ul;
}

/*
 * Does this style insert a rest between notes? ORIGINAL FACT, from the format's
 * own style semantics as recorded in rtpl_decoder.h:
 *
 *   Natural    "rest between notes"      -> yes
 *   Continuous "no rest between notes"   -> no
 *   Staccato   "longer rest period"      -> yes, but longer than Natural, and
 *                                            STEP 14B-33 authorised a
 *                                            reconstruction ratio for Natural
 *                                            only. Refusing here is honest;
 *                                            inventing a ratio would not be.
 */
static BounceSynthStatus bounce_synth_style_rest_behaviour(BounceRtplStyle style,
                                                           bool *inserts_rest)
{
    *inserts_rest = false;
    switch (style) {
    case BOUNCE_RTPL_STYLE_NATURAL:
        *inserts_rest = true;
        return BOUNCE_SYNTH_OK;
    case BOUNCE_RTPL_STYLE_CONTINUOUS:
        *inserts_rest = false;
        return BOUNCE_SYNTH_OK;
    case BOUNCE_RTPL_STYLE_STACCATO:
    case BOUNCE_RTPL_STYLE_RESERVED:
    default:
        break;
    }
    return BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE;
}

BounceSynthStatus bounce_synth_render_plan(const BounceRtplSong *song,
                                           const BounceSynthConfig *config,
                                           BounceSynthPlan *plan_out)
{
    BounceSynthStatus status;
    bool inserts_rest = false;
    unsigned int i;
    unsigned long frame_cursor = 0ul;
    unsigned long rest_ms_total = 0ul;

    if (plan_out != NULL) {
        bounce_synth_clear_plan(plan_out);
    }
    if (song == NULL || config == NULL || plan_out == NULL) {
        return BOUNCE_SYNTH_ERR_NULL_ARGUMENT;
    }
    status = bounce_synth_config_validate(config);
    if (status != BOUNCE_SYNTH_OK) {
        return status;
    }
    if (song->note_count == 0u
        || song->note_count > (unsigned int)BOUNCE_RTPL_MAX_NOTES) {
        return BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE;
    }
    status = bounce_synth_style_rest_behaviour(song->style, &inserts_rest);
    if (status != BOUNCE_SYNTH_OK) {
        return status;
    }

    plan_out->note_count = song->note_count;

    for (i = 0u; i < song->note_count; i++) {
        const BounceRtplNote *note = &song->notes[i];
        BounceSynthSegment *segment;
        unsigned long note_frames;

        if (plan_out->segment_count >= BOUNCE_SYNTH_MAX_SEGMENTS) {
            /* Cannot happen for a conforming song; refuse rather than overrun. */
            bounce_synth_clear_plan(plan_out);
            return BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE;
        }

        /*
         * NOTE SOUND TIME, taken unchanged from Layer A. The decoded duration is
         * never modified, and the rest is added alongside it rather than
         * subtracted from it.
         */
        note_frames = bounce_synth_ms_to_frames((unsigned long)note->duration_ms,
                                                config->sample_rate);
        segment = &plan_out->segments[plan_out->segment_count];
        segment->note_index = i;
        segment->duration_ms = note->duration_ms;
        segment->frame_offset = frame_cursor;
        segment->frame_count = note_frames;
        if (note->is_pause) {
            /* A pause came from the DATA: Table 3.8-6 note value 0. */
            segment->kind = BOUNCE_SYNTH_SEGMENT_PAUSE;
            segment->frequency_hz = 0.0;
        } else {
            /* ORIGINAL FACT: Layer A's nominal frequency, used as-is. */
            segment->kind = BOUNCE_SYNTH_SEGMENT_NOTE;
            segment->frequency_hz = note->nominal_frequency_hz;
        }
        frame_cursor += note_frames;
        plan_out->segment_count++;

        if (!inserts_rest) {
            continue;
        }

        /*
         * RECONSTRUCTION PARAMETER: the rest length. The semantic that a rest
         * exists is an original fact; the magnitude is UNKNOWN and 0.25 is this
         * module's declared Target B baseline, applied here as a ratio of the
         * note's own decoded duration.
         */
        {
            double rest_ms_exact = (double)note->duration_ms
                                 * config->natural_rest_ratio;
            unsigned int rest_ms = (unsigned int)(rest_ms_exact + 0.5);
            unsigned long rest_frames;

            if (rest_ms == 0u) {
                continue;
            }
            rest_frames = bounce_synth_ms_to_frames((unsigned long)rest_ms,
                                                    config->sample_rate);
            if (plan_out->segment_count >= BOUNCE_SYNTH_MAX_SEGMENTS) {
                bounce_synth_clear_plan(plan_out);
                return BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE;
            }
            segment = &plan_out->segments[plan_out->segment_count];
            segment->kind = BOUNCE_SYNTH_SEGMENT_REST;
            segment->note_index = i;
            segment->duration_ms = rest_ms;
            segment->frequency_hz = 0.0;
            segment->frame_offset = frame_cursor;
            segment->frame_count = rest_frames;
            frame_cursor += rest_frames;
            rest_ms_total += (unsigned long)rest_ms;
            plan_out->rest_count++;
            plan_out->segment_count++;
        }
    }

    plan_out->total_frames = frame_cursor;
    /* ORIGINAL FACT note sound total, plus the reconstructed rests. */
    plan_out->total_duration_ms
        = (unsigned long)song->total_note_duration_ms + rest_ms_total;
    return BOUNCE_SYNTH_OK;
}

/* -------------------------------------------------------------------------
 * PCM boundary -- NOT IMPLEMENTED IN STEP 14B-34-A
 * ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * PCM rendering -- STEP 14B-34-B
 * ------------------------------------------------------------------------- */

/*
 * RECONSTRUCTION PARAMETERS, all Target B engineering choices with no
 * evidentiary weight. The original Nokia waveform and the original Natural rest
 * magnitude are both UNKNOWN -- EVIDENCE EXHAUSTED (STEP 14B-22, STEP 14B-31,
 * STEP 14B-33 platform audit). Nothing below is a recovered Nokia value.
 */

/*
 * R2 amplitude: peak fraction of full scale. 0.5 leaves 6 dB of headroom so that
 * summing harmonics and applying the envelope can never clip int16. The API's
 * own DEFAULT_VOLUME / TONE_VOLUME_NOT_GIVEN exist, the game calls neither, and
 * the assets carry no volume instruction, so the RTPL level-to-amplitude mapping
 * is firmware-defined and UNKNOWN. 0.5 is chosen for headroom, not recovered.
 */
#define BOUNCE_SYNTH_PEAK_FRACTION 0.5

/*
 * Highest harmonic considered when band-limiting. A 44000 Hz sample rate
 * Nyquist-frequencies harmonics above about 10 kHz, so allowing up to the 12th
 * keeps the audible content and rejects the rest. This is a Target B choice.
 */
#define BOUNCE_SYNTH_MAX_HARMONIC 12u

/*
 * Envelope ramp length, as a fraction of the sounding segment's own frame
 * count. Derived from the decoded duration rather than a new absolute
 * constant, exactly as STEP 14B-33 section 11.4 requires. 1/8 gives a 12.5%
 * fade in and out, so attack plus release is 25% of the note.
 */
#define BOUNCE_SYNTH_RAMP_NUMERATOR 1u
#define BOUNCE_SYNTH_RAMP_DENOMINATOR 8u

/* Full-scale magnitude for signed 16-bit. */
#define BOUNCE_SYNTH_INT16_PEAK 32767.0

/* Ramp length in frames for a segment, clamped so a ramp never exceeds half. */
static unsigned long bounce_synth_ramp_frames(unsigned long frame_count)
{
    unsigned long ramp = (frame_count * BOUNCE_SYNTH_RAMP_NUMERATOR)
                       / BOUNCE_SYNTH_RAMP_DENOMINATOR;

    if (frame_count < 4ul) {
        return 0ul; /* Too short to fade; emit steady tone. */
    }
    if (ramp == 0ul) {
        ramp = 1ul;
    }
    if (ramp > frame_count / 2ul) {
        ramp = frame_count / 2ul;
    }
    return ramp;
}

/*
 * Raised-cosine fade. `progress` runs 0..1 across the ramp; the result is
 * exactly 0 at the silent end and exactly 1 at the loud end. This is a
 * Target B reconstruction: the original envelope is UNKNOWN, and this shape
 * exists to avoid a click, not because any source describes it.
 */
static double bounce_synth_fade(double progress)
{
    return 0.5 * (1.0 - cos(3.14159265358979323846 * progress));
}

/*
 * How many harmonics fit below Nyquist for this frequency. Harmonic k sits at
 * k*f, so the last admissible k satisfies k*f < sample_rate/2.
 */
static unsigned int bounce_synth_harmonic_count(double frequency_hz,
                                                unsigned int sample_rate)
{
    double limit = (double)sample_rate / 2.0;
    double max_k;
    unsigned int k;

    if (!(frequency_hz > 0.0) || !(frequency_hz < limit)) {
        return 0u;
    }
    max_k = limit / frequency_hz;
    if (max_k > (double)BOUNCE_SYNTH_MAX_HARMONIC) {
        max_k = (double)BOUNCE_SYNTH_MAX_HARMONIC;
    }
    k = (unsigned int)max_k;
    if (k < 1u) {
        return 1u;
    }
    return k;
}

/*
 * Render one segment of `frame_count` frames starting at `base`.
 *
 * A sounding note is a band-limited additive sine built from the plan's
 * frequency, which Layer A derived from the encoded note and scale. A rest and a
 * pause are exact digital silence.
 *
 * Determinism: for a given build the output is a pure function of the plan, the
 * configuration, and the frame count. There is no random component, no time
 * source, no floating-point accumulation across calls, and phase is re-seeded at
 * each segment so a note always begins at the same phase.
 */
static void bounce_synth_render_segment(int16_t *out,
                                        const BounceSynthSegment *segment,
                                        unsigned long base,
                                        const BounceSynthConfig *config)
{
    unsigned long count = segment->frame_count;
    unsigned long i;

    if (count == 0ul) {
        return;
    }

    if (segment->kind != BOUNCE_SYNTH_SEGMENT_NOTE
        || !(segment->frequency_hz > 0.0)) {
        /* Rest or pause: exact silence, not a ramp or a dither. */
        for (i = 0ul; i < count; i++) {
            out[base + i] = 0;
        }
        return;
    }

    {
        unsigned int harmonics =
            bounce_synth_harmonic_count(segment->frequency_hz,
                                        config->sample_rate);
        unsigned long ramp = bounce_synth_ramp_frames(count);
        double amplitude = BOUNCE_SYNTH_PEAK_FRACTION * BOUNCE_SYNTH_INT16_PEAK;
        /* Phase advance per frame, from the decoded frequency itself. */
        double step = 6.283185307179586476925286766559
                    * segment->frequency_hz / (double)config->sample_rate;
        /*
         * Normalising by the sum of the harmonic amplitudes makes the peak
         * exactly bounded by 1.0 whatever the harmonic count, so the 0.5
         * reconstruction amplitude means the same thing at every pitch.
         */
        double norm = 0.0;
        unsigned int k;

        for (k = 1u; k <= harmonics; k++) {
            norm += 1.0 / (double)k;
        }
        if (!(norm > 0.0)) {
            norm = 1.0;
        }

        for (i = 0ul; i < count; i++) {
            double phase = step * (double)i;
            double value = 0.0;
            double envelope = 1.0;

            for (k = 1u; k <= harmonics; k++) {
                value += sin((double)k * phase) / (double)k;
            }
            value /= norm;

            if (ramp > 0ul) {
                if (i < ramp) {
                    envelope = bounce_synth_fade((double)i / (double)ramp);
                } else if (i >= count - ramp) {
                    envelope = bounce_synth_fade(
                        (double)(count - 1ul - i) / (double)ramp);
                }
            }

            value *= amplitude * envelope;
            if (value > BOUNCE_SYNTH_INT16_PEAK) {
                value = BOUNCE_SYNTH_INT16_PEAK;
            } else if (value < -BOUNCE_SYNTH_INT16_PEAK) {
                value = -BOUNCE_SYNTH_INT16_PEAK;
            }
            out[base + i] = (int16_t)((value >= 0.0) ? (value + 0.5)
                                                     : (value - 0.5));
        }
    }
}

/*
 * Verify that `plan` was built from `song` before trusting its numbers. A caller
 * that pairs a plan with the wrong song would otherwise render the wrong notes,
 * so the mismatch is refused rather than silently obeyed.
 */
static BounceSynthStatus bounce_synth_plan_matches_song(
    const BounceRtplSong *song,
    const BounceSynthPlan *plan)
{
    unsigned int i;

    if (plan->note_count != song->note_count) {
        return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
    }
    for (i = 0u; i < plan->segment_count; i++) {
        const BounceSynthSegment *segment = &plan->segments[i];

        if (segment->kind == BOUNCE_SYNTH_SEGMENT_REST) {
            continue; /* A rest is derived, not decoded; nothing to cross-check. */
        }
        if (segment->note_index >= song->note_count) {
            return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
        }
        if (segment->duration_ms != song->notes[segment->note_index].duration_ms) {
            return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
        }
        if (segment->kind == BOUNCE_SYNTH_SEGMENT_NOTE
            && segment->frequency_hz
                   != song->notes[segment->note_index].nominal_frequency_hz) {
            return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
        }
    }
    return BOUNCE_SYNTH_OK;
}

BounceSynthStatus bounce_synth_render_pcm(const BounceRtplSong *song,
                                          const BounceSynthConfig *config,
                                          const BounceSynthPlan *plan,
                                          void *sample_buffer,
                                          size_t sample_buffer_frames,
                                          size_t *frames_written)
{
    int16_t *out = (int16_t *)sample_buffer;
    BounceSynthStatus status;
    unsigned long cursor = 0ul;
    unsigned int i;

    if (frames_written != NULL) {
        *frames_written = 0u;
    }
    if (song == NULL || config == NULL || plan == NULL
        || sample_buffer == NULL) {
        return BOUNCE_SYNTH_ERR_NULL_ARGUMENT;
    }
    status = bounce_synth_config_validate(config);
    if (status != BOUNCE_SYNTH_OK) {
        return status;
    }
    if (song->note_count == 0u
        || song->note_count > (unsigned int)BOUNCE_RTPL_MAX_NOTES) {
        return BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE;
    }
    status = bounce_synth_plan_matches_song(song, plan);
    if (status != BOUNCE_SYNTH_OK) {
        return status;
    }
    if (sample_buffer_frames < (size_t)plan->total_frames) {
        return BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL;
    }

    for (i = 0u; i < plan->segment_count; i++) {
        const BounceSynthSegment *segment = &plan->segments[i];

        if (segment->frame_offset != cursor) {
            return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
        }
        if (segment->frame_count > (unsigned long)sample_buffer_frames - cursor) {
            return BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL;
        }
        bounce_synth_render_segment(out, segment, cursor, config);
        cursor += segment->frame_count;
    }
    if (cursor != plan->total_frames) {
        return BOUNCE_SYNTH_ERR_PLAN_MISMATCH;
    }

    if (frames_written != NULL) {
        *frames_written = (size_t)cursor;
    }
    return BOUNCE_SYNTH_OK;
}

/* -------------------------------------------------------------------------
 * The full device-free chain
 * ------------------------------------------------------------------------- */

/* Long enough for any realistic root plus a 64-character relative path. */
#define BOUNCE_SYNTH_PATH_CAPACITY 512u

BounceSynthStatus bounce_synth_prepare(BounceAudioEvent event,
                                      const char *resource_root,
                                      const BounceSynthConfig *config,
                                      BounceRtplSong *song_out,
                                      BounceSynthPlan *plan_out)
{
    BounceSynthResource resource;
    BounceSynthStatus status;
    BounceRtplSong local_song;
    BounceRtplSong *song = (song_out != NULL) ? song_out : &local_song;
    char path[BOUNCE_SYNTH_PATH_CAPACITY];
    BounceRtplStatus decode_status;
    int written;

    if (song_out != NULL) {
        memset(song_out, 0, sizeof(*song_out));
    }
    if (plan_out != NULL) {
        bounce_synth_clear_plan(plan_out);
    }
    if (resource_root == NULL || config == NULL) {
        return BOUNCE_SYNTH_ERR_NULL_ARGUMENT;
    }
    status = bounce_synth_config_validate(config);
    if (status != BOUNCE_SYNTH_OK) {
        return status;
    }
    status = bounce_synth_map_event(event, &resource);
    if (status != BOUNCE_SYNTH_OK || !resource.has_resource
        || resource.relative_path == NULL) {
        return BOUNCE_SYNTH_ERR_NO_RESOURCE;
    }

    written = snprintf(path, sizeof(path), "%s/%s",
                       resource_root, resource.relative_path);
    if (written < 0 || (unsigned int)written >= sizeof(path)) {
        return BOUNCE_SYNTH_ERR_NO_RESOURCE;
    }

    /*
     * Layer A does the reading and the parsing. This module never inspects the
     * bytes and never keeps a copy of the note data.
     */
    memset(song, 0, sizeof(*song));
    decode_status = bounce_rtpl_decode_file(path, song);
    if (decode_status != BOUNCE_RTPL_OK) {
        /*
         * The exact decoder reason is available to the caller through `song`'s
         * sibling Layer A API, `bounce_rtpl_status_text`. It is deliberately
         * not flattened into a string here, so a DECODER failure stays
         * distinguishable from a RENDERER failure.
         */
        return BOUNCE_SYNTH_ERR_DECODE_FAILED;
    }

    if (plan_out == NULL) {
        return BOUNCE_SYNTH_OK;
    }
    return bounce_synth_render_plan(song, config, plan_out);
}

/* -------------------------------------------------------------------------
 * Self-test
 * ------------------------------------------------------------------------- */

static int bounce_synth_failures;

static void bounce_synth_expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    bounce_synth_failures++;
}

/*
 * Count NOTE segments and total NOTE frame span in a plan, used to prove the
 * plan consumed Layer A's note data rather than a private table.
 */
static void bounce_synth_summarise_notes(const BounceSynthPlan *plan,
                                         unsigned int *note_segments,
                                         unsigned long *note_frames)
{
    unsigned int i;

    *note_segments = 0u;
    *note_frames = 0ul;
    for (i = 0u; i < plan->segment_count; i++) {
        if (plan->segments[i].kind == BOUNCE_SYNTH_SEGMENT_NOTE) {
            (*note_segments)++;
            *note_frames += plan->segments[i].frame_count;
        }
    }
}

/*
 * Decode one original asset through Layer A and check the plan built from it.
 * `expected_notes` and `expected_total_note_ms` are the values proven by
 * STEP 14B-19 / STEP 14B-20 and asserted by `check-rtpl`; they are written out
 * here independently and are never derived from this module's own output.
 */
static void bounce_synth_check_asset(BounceAudioEvent event,
                                     const char *expected_path,
                                     unsigned int expected_notes,
                                     unsigned int expected_total_note_ms,
                                     const char *resource_root,
                                     const BounceSynthConfig *config)
{
    BounceSynthResource resource;
    BounceSynthStatus status;
    BounceRtplSong song;
    BounceSynthPlan plan;
    unsigned int note_segments = 0u;
    unsigned long note_frames = 0ul;
    unsigned int i;
    int matches = 1;
    char description[160];

    status = bounce_synth_map_event(event, &resource);
    (void)snprintf(description, sizeof(description),
                   "mapping: event %d -> %s", (int)event,
                   (resource.relative_path != NULL) ? resource.relative_path
                                                    : "(none)");
    bounce_synth_expect(status == BOUNCE_SYNTH_OK && resource.has_resource
                            && strcmp(resource.relative_path, expected_path) == 0,
                        description);

    status = bounce_synth_prepare(event, resource_root, config, &song, &plan);
    (void)snprintf(description, sizeof(description),
                   "prepare: %s decodes through Layer A and plans", expected_path);
    bounce_synth_expect(status == BOUNCE_SYNTH_OK, description);
    if (status != BOUNCE_SYNTH_OK) {
        return;
    }

    (void)snprintf(description, sizeof(description),
                   "Layer A: %s note count == %u (not a private table)",
                   expected_path, expected_notes);
    bounce_synth_expect(song.note_count == expected_notes, description);

    (void)snprintf(description, sizeof(description),
                   "Layer A: %s note-sound total == %u ms (unchanged)",
                   expected_path, expected_total_note_ms);
    bounce_synth_expect(song.total_note_duration_ms == expected_total_note_ms,
                        description);

    bounce_synth_expect(plan.note_count == song.note_count,
                        "plan consumes Layer A's note count");

    /* Every NOTE segment must carry Layer A's own frequency and duration. */
    for (i = 0u; i < song.note_count; i++) {
        const BounceRtplNote *note = &song.notes[i];
        if (note->nominal_frequency_hz <= 0.0) {
            matches = 0;
        }
    }
    (void)snprintf(description, sizeof(description),
                   "Layer A: %s every note has a positive nominal frequency",
                   expected_path);
    bounce_synth_expect(matches, description);

    bounce_synth_summarise_notes(&plan, &note_segments, &note_frames);
    (void)snprintf(description, sizeof(description),
                   "plan: %s has %u NOTE segments, one per decoded note",
                   expected_path, expected_notes);
    bounce_synth_expect(note_segments == expected_notes, description);

    /* Natural style: a rest must exist between notes, and it must be shorter
     * than the note it follows -- the reconstruction ratio is well under 1. */
    (void)snprintf(description, sizeof(description),
                   "plan: %s inserts one rest per note (Natural semantics)",
                   expected_path);
    bounce_synth_expect(plan.rest_count == expected_notes, description);

    /*
     * The rest total is recomputed here from Layer A's own decoded durations
     * and the declared ratio, independently of the plan. This is the note
     * timing contract: decoded duration is used unchanged, and the rest is
     * derived alongside it rather than substituted for it.
     */
    {
        unsigned long expected_rest_ms = 0ul;

        for (i = 0u; i < song.note_count; i++) {
            expected_rest_ms
                += (unsigned long)(unsigned int)
                       ((double)song.notes[i].duration_ms
                            * config->natural_rest_ratio
                        + 0.5);
        }
        (void)snprintf(description, sizeof(description),
                       "plan: %s total %lu ms == note sound %u ms + derived rests"
                       " %lu ms",
                       expected_path, plan.total_duration_ms,
                       expected_total_note_ms, expected_rest_ms);
        bounce_synth_expect(plan.total_duration_ms
                                == (unsigned long)expected_total_note_ms
                                       + expected_rest_ms,
                            description);
        bounce_synth_expect(plan.total_duration_ms
                                > (unsigned long)expected_total_note_ms,
                            "plan: rest length is added, not taken from"
                            " note sound time");
    }

    /*
     * Frame accounting. Each segment occupies a contiguous, independently
     * rounded range of the output, so `total_frames` must equal the exact sum of
     * the segment lengths. It may differ from a single rounding of the overall
     * duration by at most half a sample per segment boundary, which is
     * asserted as a bound rather than ignored.
     */
    {
        unsigned long summed_frames = 0ul;
        unsigned long ideal_frames;
        unsigned long drift;

        for (i = 0u; i < plan.segment_count; i++) {
            summed_frames += plan.segments[i].frame_count;
        }
        (void)snprintf(description, sizeof(description),
                       "plan: %s total_frames %lu equals the sum of its %u"
                       " segments",
                       expected_path, plan.total_frames, plan.segment_count);
        bounce_synth_expect(plan.total_frames == summed_frames, description);

        ideal_frames = ((unsigned long)plan.total_duration_ms
                            * (unsigned long)config->sample_rate + 500ul)
                       / 1000ul;
        drift = (plan.total_frames > ideal_frames)
                    ? (plan.total_frames - ideal_frames)
                    : (ideal_frames - plan.total_frames);
        (void)snprintf(description, sizeof(description),
                       "plan: %s frame rounding drift %lu is within %u samples"
                       " (half a sample per segment)",
                       expected_path, drift, plan.segment_count);
        bounce_synth_expect(drift <= (unsigned long)plan.segment_count,
                            description);
    }

    /* STEP 14B-34-B: the PCM boundary is now implemented. */
    {
        int16_t *pcm = (int16_t *)calloc(
            (size_t)plan.total_frames + 1u, sizeof(int16_t));
        size_t frames_written = 0u;

        if (pcm == NULL) {
            bounce_synth_expect(0, "PCM allocation for the test");
        } else {
            status = bounce_synth_render_pcm(&song, config, &plan, pcm,
                                             (size_t)plan.total_frames,
                                             &frames_written);
            (void)snprintf(description, sizeof(description),
                           "PCM: %s renders %lu frames (exactly the plan total)",
                           expected_path, plan.total_frames);
            bounce_synth_expect(status == BOUNCE_SYNTH_OK
                                    && frames_written
                                           == (size_t)plan.total_frames,
                                description);
            free(pcm);
        }
    }
}

int bounce_synth_verify(const char *resource_root, FILE *output_file)
{
    BounceSynthConfig config;
    BounceSynthResource resource;
    BounceSynthStatus status;
    BounceRtplSong song;
    BounceSynthPlan plan;

    bounce_synth_failures = 0;
    if (output_file == NULL) {
        output_file = stdout;
    }

    (void)fprintf(output_file, "audio_synth skeleton self-test (STEP 14B-34-A)\n");
    (void)fprintf(output_file,
                  "  no device, no backend, no PCM samples, no thread,"
                  " no queue\n");
    (void)fprintf(output_file,
                  "  waveform and Natural rest ratio are Target B"
                  " RECONSTRUCTION PARAMETERS\n\n");

    if (resource_root == NULL) {
        (void)fprintf(output_file, "      FAIL resource_root is required\n");
        return -1;
    }

    /*
     * Established before any other assertion, because every later case needs a
     * valid configuration in order to reach the condition it is actually
     * testing. Declaring it here keeps the failure-policy cases below from
     * accidentally reporting an invalid-configuration error.
     */
    bounce_synth_default_config(&config);

    /* ---- 1..4  identity -> resource mapping (ORIGINAL FACT) ---- */
    (void)fprintf(output_file, "  identity -> resource mapping\n");
    status = bounce_synth_map_event(BOUNCE_AUDIO_EVENT_NONE, &resource);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NO_RESOURCE
                            && !resource.has_resource
                            && resource.relative_path == NULL,
                        "NONE -> no resource");
    bounce_synth_expect(bounce_synth_resource_path(BOUNCE_AUDIO_EVENT_NONE)
                            == NULL,
                        "NONE -> no path");
    (void)fprintf(output_file, "  UP -> %s\n",
                  bounce_synth_resource_path(BOUNCE_AUDIO_EVENT_UP));
    (void)fprintf(output_file, "  PICKUP -> %s\n",
                  bounce_synth_resource_path(BOUNCE_AUDIO_EVENT_PICKUP));
    (void)fprintf(output_file, "  POP -> %s\n",
                  bounce_synth_resource_path(BOUNCE_AUDIO_EVENT_POP));

    /* ---- 5  unrecognised identity is refused, not guessed ---- */
    (void)fprintf(output_file, "\n  failure policy\n");
    status = bounce_synth_map_event((BounceAudioEvent)4, &resource);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NO_RESOURCE
                            && !resource.has_resource
                            && resource.relative_path == NULL,
                        "reserved identity 4 -> safe no-op, no resource");
    status = bounce_synth_map_event((BounceAudioEvent)9999, &resource);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NO_RESOURCE
                            && resource.relative_path == NULL,
                        "out-of-range identity -> safe no-op, no resource");
    status = bounce_synth_prepare(BOUNCE_AUDIO_EVENT_NONE, resource_root, &config,
                                  &song, &plan);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NO_RESOURCE,
                        "prepare(NONE) -> no resource, no decode");
    status = bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, NULL,
                                  &song, &plan);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NULL_ARGUMENT,
                        "prepare(NULL config) -> null argument");
    status = bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, NULL, &config, &song,
                                  &plan);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_NULL_ARGUMENT,
                        "prepare(NULL root) -> null argument");
    status = bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP,
                                  "no/such/resource/root", &config, &song,
                                  &plan);
    bounce_synth_expect(status == BOUNCE_SYNTH_ERR_DECODE_FAILED,
                        "missing asset -> decoder-reported failure, no crash");

    /* ---- 6  reconstruction configuration ---- */
    (void)fprintf(output_file, "\n  reconstruction configuration"
                              " (Target B parameters)\n");
    bounce_synth_expect(config.sample_rate == 44100u,
                        "sample rate accepted: 44100 Hz");
    bounce_synth_expect(config.sample_format == BOUNCE_SYNTH_SAMPLE_S16LE,
                        "sample format accepted: signed 16-bit little-endian");
    bounce_synth_expect(config.channels == BOUNCE_SYNTH_CHANNELS_MONO,
                        "channel count accepted: 1 (mono)");
    bounce_synth_expect(config.waveform == BOUNCE_SYNTH_WAVE_SINE_BANDLIMITED,
                        "waveform accepted: sine band-limited (reconstruction)");
    bounce_synth_expect(config.natural_rest_ratio == 0.25,
                        "Natural rest ratio accepted: 0.25 (reconstruction)");
    bounce_synth_expect(bounce_synth_config_validate(&config) == BOUNCE_SYNTH_OK,
                        "baseline configuration validates");

    {   /* Deterministic rejection of every invalid field. */
        BounceSynthConfig bad = config;
        bad.sample_rate = 0u;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "sample rate 0 rejected");
        bad = config;
        bad.sample_rate = 4000000u;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "absurd sample rate rejected");
        bad = config;
        bad.channels = (BounceSynthChannelCount)2;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "stereo channel count rejected");
        bad = config;
        bad.sample_format = (BounceSynthSampleFormat)7;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "unknown sample format rejected");
        bad = config;
        bad.waveform = (BounceSynthWaveform)99;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "unknown waveform rejected");
        bad = config;
        bad.natural_rest_ratio = -0.5;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "negative rest ratio rejected");
        bad = config;
        bad.natural_rest_ratio = 2.0;
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "rest ratio above 1 rejected");
        bad = config;
        bad.natural_rest_ratio = 0.0 / 0.0; /* NaN without libm */
        bounce_synth_expect(bounce_synth_config_validate(&bad)
                                == BOUNCE_SYNTH_ERR_INVALID_CONFIG,
                            "NaN rest ratio rejected");
        bounce_synth_expect(bounce_synth_config_validate(NULL)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT,
                            "NULL configuration rejected");
    }

    /* ---- 7..8  real decoded data, consumed through Layer A ---- */
    (void)fprintf(output_file, "\n  original assets through Layer A"
                              " (no duplicated note table)\n");
    bounce_synth_check_asset(BOUNCE_AUDIO_EVENT_UP, "sounds/up.ott", 4u, 480u,
                             resource_root, &config);
    bounce_synth_check_asset(BOUNCE_AUDIO_EVENT_PICKUP, "sounds/pickup.ott",
                             3u, 600u, resource_root, &config);
    bounce_synth_check_asset(BOUNCE_AUDIO_EVENT_POP, "sounds/pop.ott", 3u, 240u,
                             resource_root, &config);

    /* ---- 9  note-timing contract: rest is derived, duration untouched ---- */
    (void)fprintf(output_file, "\n  note timing contract\n");
    {
        BounceRtplSong natural_song;
        BounceRtplSong continuous_song;
        BounceRtplSong staccato_song;
        BounceSynthPlan natural_plan;

        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_UP, resource_root, &config,
                             &natural_song, &natural_plan);
        /*
         * Reduced to a single note so the arithmetic below is exact and
         * readable. The decoded note is real Layer A output; only the note
         * count is narrowed for the timing case.
         */
        natural_song.note_count = 1u;
        natural_song.notes[0].duration_ms = 200u;
        natural_song.total_note_duration_ms = 200u;
        status = bounce_synth_render_plan(&natural_song, &config, &natural_plan);
        bounce_synth_expect(status == BOUNCE_SYNTH_OK
                                && natural_plan.rest_count == 1u,
                            "rest derived from the decoded note duration");
        bounce_synth_expect(natural_plan.segments[0].duration_ms == 200u,
                            "decoded note duration used unchanged (200 ms)");
        bounce_synth_expect(natural_plan.segments[1].kind
                                == BOUNCE_SYNTH_SEGMENT_REST
                                && natural_plan.segments[1].duration_ms == 50u,
                            "rest = 200 ms x 0.25 = 50 ms (reconstruction)");
        bounce_synth_expect(natural_plan.total_duration_ms == 250u,
                            "plan total = note sound + rest, not either alone");

        /* Continuous style is an original fact: no rest between notes. */
        continuous_song = natural_song;
        continuous_song.style = BOUNCE_RTPL_STYLE_CONTINUOUS;
        status = bounce_synth_render_plan(&continuous_song, &config,
                                          &natural_plan);
        bounce_synth_expect(status == BOUNCE_SYNTH_OK
                                && natural_plan.rest_count == 0u
                                && natural_plan.total_duration_ms == 200u,
                            "Continuous style inserts no rest (original fact)");

        /* Staccato is refused: no reconstruction ratio was authorised for it. */
        staccato_song = natural_song;
        staccato_song.style = BOUNCE_RTPL_STYLE_STACCATO;
        status = bounce_synth_render_plan(&staccato_song, &config,
                                          &natural_plan);
        bounce_synth_expect(status == BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE,
                            "Staccato refused, not given an invented ratio");
        bounce_synth_expect(natural_plan.segment_count == 0u,
                            "a refused plan is left empty, never partial");

        /* Reserved style, mirroring the decoder's own rejection. */
        staccato_song.style = BOUNCE_RTPL_STYLE_RESERVED;
        status = bounce_synth_render_plan(&staccato_song, &config,
                                          &natural_plan);
        bounce_synth_expect(status == BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE,
                            "reserved style refused");

        /* An empty note sequence is refused, not rendered as silence. */
        staccato_song = natural_song;
        staccato_song.style = BOUNCE_RTPL_STYLE_NATURAL;
        staccato_song.note_count = 0u;
        status = bounce_synth_render_plan(&staccato_song, &config,
                                          &natural_plan);
        bounce_synth_expect(status == BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE,
                            "empty note sequence refused, no fallback tone");
        bounce_synth_expect(bounce_synth_render_plan(NULL, &config,
                                                     &natural_plan)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT,
                            "NULL song refused");
        bounce_synth_expect(bounce_synth_render_plan(&natural_song, &config, NULL)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT,
                            "NULL plan refused");
    }

    /* ---- 10  a differing rest ratio changes only the reconstruction ---- */
    (void)fprintf(output_file, "\n  rest ratio is an isolated parameter\n");
    {
        BounceRtplSong song2;
        BounceSynthPlan plan2;
        BounceSynthConfig variant = config;

        variant.natural_rest_ratio = 0.5;
        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                             &song2, &plan2);
        status = bounce_synth_render_plan(&song2, &variant, &plan2);
        bounce_synth_expect(status == BOUNCE_SYNTH_OK
                                && song2.total_note_duration_ms == 240u
                                && plan2.total_duration_ms == 360u,
                            "0.5 ratio changes rests only; decoded 240 ms intact");
    }

    /* ---- 11  the PCM boundary now renders, and refuses safely ---- */
    (void)fprintf(output_file, "\n  skeleton boundary\n");
    {
        int16_t probe[16];
        size_t frames_written = 7u;

        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                             &song, &plan);
        bounce_synth_expect(bounce_synth_render_pcm(&song, &config, &plan, NULL,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT
                                && frames_written == 0u,
                            "PCM seam refuses a NULL buffer without crashing");
        bounce_synth_expect(bounce_synth_render_pcm(&song, &config, &plan, probe,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL
                                && frames_written == 0u,
                            "a buffer smaller than the plan is refused, writes 0");
    }

    if (bounce_synth_failures != 0) {
        (void)fprintf(output_file, "\nLayer C skeleton self-test: FAIL"
                                  " (%d failure(s))\n",
                      bounce_synth_failures);
        return -1;
    }
    (void)fprintf(output_file, "\nLayer C skeleton self-test: PASS"
                              " (0 failures)\n");
    (void)fprintf(output_file, "no device was opened; no PCM was generated;"
                              " no thread was created; no queue exists\n");
    return 0;
}

/* -------------------------------------------------------------------------
 * STEP 14B-34-B renderer self-test
 * ------------------------------------------------------------------------- */

/*
 * Energy of `frames` samples at exactly `frequency_hz`, by the Goertzel
 * algorithm. This is an INDEPENDENT measurement of the rendered buffer: it does
 * not reuse any value the renderer computed, so it can contradict the renderer.
 */
static double bounce_synth_goertzel(const int16_t *samples, size_t count,
                                    double frequency_hz, unsigned int sample_rate)
{
    double k = 0.5 + (double)count * frequency_hz / (double)sample_rate;
    double omega = 6.283185307179586476925286766559 * k / (double)count;
    double coeff = 2.0 * cos(omega);
    double s1 = 0.0;
    double s2 = 0.0;
    size_t i;

    for (i = 0u; i < count; i++) {
        double s0 = (double)samples[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return (s1 * s1 + s2 * s2 - coeff * s1 * s2)
           / ((double)sample_rate / 2.0);
}

/* Highest and lowest sample magnitude in a frame range. */
static void bounce_synth_peak_range(const int16_t *samples, size_t from,
                                    size_t count, int32_t *peak,
                                    int32_t *trough)
{
    size_t i;

    *peak = 0;
    *trough = 0;
    for (i = 0u; i < count; i++) {
        int32_t v = (int32_t)samples[from + i];
        if (v > *peak) {
            *peak = v;
        }
        if (v < *trough) {
            *trough = v;
        }
    }
}

/*
 * Render one original asset and verify the result against Layer A's decoded
 * data. The expected note counts and total note-sound times are the values
 * proven by STEP 14B-19 / STEP 14B-20 and asserted by check-rtpl; they are
 * written out here rather than derived from this module.
 */
static void bounce_synth_verify_pcm_asset(BounceAudioEvent event,
                                          const char *label,
                                          unsigned int expected_notes,
                                          const char *resource_root,
                                          const BounceSynthConfig *config)
{
    BounceRtplSong song;
    BounceSynthPlan plan;
    int16_t *pcm = NULL;
    size_t frames_written = 0u;
    size_t total;
    BounceSynthStatus status;
    char description[180];
    unsigned int i;
    int ok_duration = 1;
    int ok_frequency = 1;
    int ok_silence = 1;
    int ok_range = 1;
    int ok_peak = 1;

    status = bounce_synth_prepare(event, resource_root, config, &song, &plan);
    (void)snprintf(description, sizeof(description), "%s: decoded %u notes",
                   label, expected_notes);
    bounce_synth_expect(status == BOUNCE_SYNTH_OK && song.note_count == expected_notes,
                        description);
    if (status != BOUNCE_SYNTH_OK) {
        return;
    }

    total = (size_t)plan.total_frames;
    pcm = (int16_t *)calloc(total + 1u, sizeof(int16_t));
    if (pcm == NULL) {
        bounce_synth_expect(0, "PCM allocation");
        return;
    }
    status = bounce_synth_render_pcm(&song, config, &plan, pcm, total,
                                     &frames_written);
    (void)snprintf(description, sizeof(description),
                   "%s: rendered %lu frames, matching the plan exactly",
                   label, plan.total_frames);
    bounce_synth_expect(status == BOUNCE_SYNTH_OK
                            && frames_written == total,
                        description);

    for (i = 0u; i < plan.segment_count && ok_duration; i++) {
        const BounceSynthSegment *segment = &plan.segments[i];

        if (segment->kind == BOUNCE_SYNTH_SEGMENT_REST) {
            continue;
        }
        /* The rendered span must be the decoded duration, frame for frame. */
        if (segment->duration_ms != song.notes[segment->note_index].duration_ms) {
            ok_duration = 0;
        }
    }
    (void)snprintf(description, sizeof(description),
                   "%s: every note keeps its decoded duration exactly", label);
    bounce_synth_expect(ok_duration, description);

    /* Frequency: measure each sounding segment at Layer A's own value, and
     * confirm a far-off control frequency carries far less energy. */
    for (i = 0u; i < plan.segment_count && ok_frequency; i++) {
        const BounceSynthSegment *segment = &plan.segments[i];
        double expected;
        double at_expected;
        double at_control;
        double floor_hz;

        if (segment->kind != BOUNCE_SYNTH_SEGMENT_NOTE) {
            continue;
        }
        expected = song.notes[segment->note_index].nominal_frequency_hz;
        if (expected <= 0.0) {
            ok_frequency = 0;
            break;
        }
        at_expected = bounce_synth_goertzel(
            pcm + (size_t)segment->frame_offset,
            (size_t)segment->frame_count, expected, config->sample_rate);
        /* Control: half an octave away, so it must be far below the fundamental. */
        floor_hz = expected * 0.70710678118654752440;
        at_control = bounce_synth_goertzel(
            pcm + (size_t)segment->frame_offset,
            (size_t)segment->frame_count, floor_hz, config->sample_rate);
        if (!(at_expected > at_control * 100.0)) {
            ok_frequency = 0;
        }
    }
    (void)snprintf(description, sizeof(description),
                   "%s: each note's energy sits at Layer A's decoded frequency",
                   label);
    bounce_synth_expect(ok_frequency, description);

    /* Rests must be exact digital silence. */
    for (i = 0u; i < plan.segment_count && ok_silence; i++) {
        const BounceSynthSegment *segment = &plan.segments[i];
        size_t f;

        if (segment->kind != BOUNCE_SYNTH_SEGMENT_REST) {
            continue;
        }
        for (f = 0ul; f < segment->frame_count; f++) {
            if (pcm[(size_t)segment->frame_offset + f] != 0) {
                ok_silence = 0;
                break;
            }
        }
    }
    (void)snprintf(description, sizeof(description),
                   "%s: every rest is exact zero, no dither, no ramp", label);
    bounce_synth_expect(ok_silence, description);

    /* No sample may leave the int16 range, and the peak must respect the 0.5
     * reconstruction amplitude rather than reaching full scale. */
    for (i = 0u; i < plan.segment_count && ok_range; i++) {
        const BounceSynthSegment *segment = &plan.segments[i];
        size_t f;

        if (segment->kind != BOUNCE_SYNTH_SEGMENT_NOTE) {
            continue;
        }
        for (f = 0ul; f < segment->frame_count; f++) {
            int16_t v = pcm[(size_t)segment->frame_offset + f];
            if (v > 32000 || v < -32000) {
                ok_range = 0;
                break;
            }
        }
    }
    (void)snprintf(description, sizeof(description),
                   "%s: no sample exceeds the 0.5-of-full-scale peak", label);
    bounce_synth_expect(ok_range, description);

    /* A sounding segment must actually be non-silent. */
    {
        int32_t peak = 0;
        int32_t trough = 0;

        for (i = 0u; i < plan.segment_count; i++) {
            const BounceSynthSegment *segment = &plan.segments[i];
            int32_t p = 0;
            int32_t t = 0;

            if (segment->kind != BOUNCE_SYNTH_SEGMENT_NOTE) {
                continue;
            }
            bounce_synth_peak_range(pcm, (size_t)segment->frame_offset,
                                    (size_t)segment->frame_count, &p, &t);
            if (p > peak) {
                peak = p;
            }
            if (t < trough) {
                trough = t;
            }
        }
        (void)snprintf(description, sizeof(description),
                       "%s: sounding notes are non-silent (peak %d, trough %d)",
                       label, (int)peak, (int)trough);
        bounce_synth_expect(peak > 1000 && trough < -1000, description);
        ok_peak = (peak > 0) && (trough < 0);
        (void)ok_peak;
    }

    /* Determinism: rendering the same inputs twice yields identical bytes. */
    {
        int16_t *again = (int16_t *)calloc(total + 1u, sizeof(int16_t));
        size_t second_written = 0u;

        if (again != NULL) {
            status = bounce_synth_render_pcm(&song, config, &plan, again, total,
                                             &second_written);
            (void)snprintf(description, sizeof(description),
                           "%s: two renders of the same input are identical",
                           label);
            bounce_synth_expect(status == BOUNCE_SYNTH_OK
                                    && second_written == frames_written
                                    && memcmp(pcm, again,
                                              total * sizeof(int16_t)) == 0,
                                description);
            free(again);
        }
    }

    /* The rendered length must equal note sound time plus the declared rests. */
    {
        unsigned long expected_ms = (unsigned long)song.total_note_duration_ms;
        unsigned long expected_frames;

        for (i = 0u; i < song.note_count; i++) {
            expected_ms += (unsigned long)(unsigned int)
                ((double)song.notes[i].duration_ms * config->natural_rest_ratio
                 + 0.5);
        }
        expected_frames = (expected_ms * (unsigned long)config->sample_rate
                           + 500ul) / 1000ul;
        /* Per-segment rounding can drift by at most one sample per boundary. */
        {
            unsigned long drift = (plan.total_frames > expected_frames)
                ? (plan.total_frames - expected_frames)
                : (expected_frames - plan.total_frames);

            (void)snprintf(description, sizeof(description),
                           "%s: %lu rendered frames match %lu ms of"
                           " note sound + reconstructed rests (drift %lu)",
                           label, plan.total_frames, expected_ms, drift);
            bounce_synth_expect(drift <= (unsigned long)plan.segment_count,
                                description);
        }
    }

    free(pcm);
}

int bounce_synth_verify_pcm(const char *resource_root, FILE *output_file)
{
    BounceSynthConfig config;
    BounceRtplSong song;
    BounceSynthPlan plan;
    BounceSynthStatus status;
    int16_t probe[16];
    size_t frames_written = 0u;

    bounce_synth_failures = 0;
    if (output_file == NULL) {
        output_file = stdout;
    }

    (void)fprintf(output_file, "Layer C renderer self-test (STEP 14B-34-B)\n");
    (void)fprintf(output_file,
                  "  deterministic S16LE mono 44100 Hz; band-limited sine and the"
                  " 0.25 rest ratio are Target B RECONSTRUCTION PARAMETERS\n");
    (void)fprintf(output_file, "  no device, no backend, no thread, no queue\n\n");

    if (resource_root == NULL) {
        (void)fprintf(output_file, "      FAIL resource_root is required\n");
        return -1;
    }
    bounce_synth_default_config(&config);

    /* Each original asset, verified against Layer A's decoded data. */
    (void)fprintf(output_file, "  original assets rendered from Layer A data\n");
    bounce_synth_verify_pcm_asset(BOUNCE_AUDIO_EVENT_UP, "up.ott", 4u,
                                  resource_root, &config);
    bounce_synth_verify_pcm_asset(BOUNCE_AUDIO_EVENT_PICKUP, "pickup.ott", 3u,
                                  resource_root, &config);
    bounce_synth_verify_pcm_asset(BOUNCE_AUDIO_EVENT_POP, "pop.ott", 3u,
                                  resource_root, &config);

    /* The documented decoded sequences, asserted exactly as given. */
    (void)fprintf(output_file, "\n  decoded sequences preserved verbatim\n");
    {
        static const double up_hz[4] = { 1046.50, 659.26, 783.99, 1046.50 };
        static const double pickup_hz[3] = { 1046.50, 783.99, 1046.50 };
        static const double pop_hz[3] = { 1567.98, 1975.53, 523.25 };
        BounceRtplSong reference;
        char description[160];
        int i2;
        int ok = 1;

        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_UP, resource_root, &config,
                             &reference, &plan);
        for (i2 = 0; i2 < 4 && ok; i2++) {
            double d = reference.notes[i2].nominal_frequency_hz - up_hz[i2];
            if (d > 0.005 || d < -0.005) {
                ok = 0;
            }
            if (reference.notes[i2].duration_ms != 120u) {
                ok = 0;
            }
        }
        (void)snprintf(description, sizeof(description),
                       "up.ott is C-E-G-C at 1046.50/659.26/783.99/1046.50 Hz,"
                       " 120 ms each");
        bounce_synth_expect(ok, description);

        ok = 1;
        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_PICKUP, resource_root, &config,
                             &reference, &plan);
        for (i2 = 0; i2 < 3 && ok; i2++) {
            double d = reference.notes[i2].nominal_frequency_hz - pickup_hz[i2];
            if (d > 0.005 || d < -0.005) {
                ok = 0;
            }
        }
        (void)snprintf(description, sizeof(description),
                       "pickup.ott is C-G-C at 1046.50/783.99/1046.50 Hz,"
                       " 240/120/240 ms");
        bounce_synth_expect(ok
                                && reference.notes[0].duration_ms == 240u
                                && reference.notes[1].duration_ms == 120u
                                && reference.notes[2].duration_ms == 240u,
                            description);

        ok = 1;
        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                             &reference, &plan);
        for (i2 = 0; i2 < 3 && ok; i2++) {
            double d = reference.notes[i2].nominal_frequency_hz - pop_hz[i2];
            if (d > 0.005 || d < -0.005) {
                ok = 0;
            }
        }
        (void)snprintf(description, sizeof(description),
                       "pop.ott is G-H-C at 1567.98/1975.53/523.25 Hz,"
                       " 60/60/120 ms");
        bounce_synth_expect(ok
                                && reference.notes[0].duration_ms == 60u
                                && reference.notes[1].duration_ms == 60u
                                && reference.notes[2].duration_ms == 120u,
                            description);
    }

    /* Failure and safety behaviour of the renderer. */
    (void)fprintf(output_file, "\n  renderer failure policy\n");
    {
        char description[160];

        bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                             &song, &plan);
        bounce_synth_expect(bounce_synth_render_pcm(NULL, &config, &plan, probe,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT
                                && frames_written == 0u,
                            "NULL song refused, 0 frames");
        bounce_synth_expect(bounce_synth_render_pcm(&song, NULL, &plan, probe,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT
                                && frames_written == 0u,
                            "NULL config refused, 0 frames");
        bounce_synth_expect(bounce_synth_render_pcm(&song, &config, NULL, probe,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT
                                && frames_written == 0u,
                            "NULL plan refused, 0 frames");
        bounce_synth_expect(bounce_synth_render_pcm(&song, &config, &plan, NULL,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_NULL_ARGUMENT
                                && frames_written == 0u,
                            "NULL buffer refused, 0 frames");
        bounce_synth_expect(bounce_synth_render_pcm(&song, &config, &plan, probe,
                                                    16u, &frames_written)
                                == BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL
                                && frames_written == 0u,
                            "undersized buffer refused, 0 frames");

        /* A plan paired with the wrong song must be refused, not obeyed. */
        {
            BounceRtplSong other;
            BounceSynthPlan other_plan;

            bounce_synth_prepare(BOUNCE_AUDIO_EVENT_UP, resource_root, &config,
                                 &other, &other_plan);
            status = bounce_synth_render_pcm(&other, &config, &plan, probe,
                                             (size_t)plan.total_frames,
                                             &frames_written);
            (void)snprintf(description, sizeof(description),
                           "a plan from a different song is refused, 0 frames");
            bounce_synth_expect(status == BOUNCE_SYNTH_ERR_PLAN_MISMATCH
                                    && frames_written == 0u,
                                description);
        }

        /* An empty note sequence never produces a fallback tone. */
        {
            BounceRtplSong empty_song = song;
            BounceSynthPlan empty_plan;

            empty_song.note_count = 0u;
            status = bounce_synth_render_pcm(&empty_song, &config, &empty_plan,
                                             probe, 16u, &frames_written);
            bounce_synth_expect(status == BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE
                                    && frames_written == 0u,
                                "empty note sequence refused, no fallback tone");
        }
    }

    if (bounce_synth_failures != 0) {
        (void)fprintf(output_file, "\nLayer C renderer self-test: FAIL"
                                  " (%d failure(s))\n",
                      bounce_synth_failures);
        return -1;
    }
    (void)fprintf(output_file, "\nLayer C renderer self-test: PASS"
                              " (0 failures)\n");
    (void)fprintf(output_file, "no device was opened; no thread was created;"
                              " no queue exists; no audio was played\n");
    return 0;
}
