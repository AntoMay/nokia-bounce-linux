/*
 * audio_synth.h -- STEP 14B-34-A Layer C skeleton (Target B).
 *
 * WHAT THIS FILE IS
 *   The internal Layer C structure for Target B -- native Linux audio
 *   reconstruction. It fixes the boundary between the already-finished layers
 *   and the audio device that does not exist yet:
 *
 *       BounceAudioEvent          (Layer B, identity only, unchanged)
 *            |
 *            v
 *       resource mapping          (this file: 3 identities -> 3 assets)
 *            |
 *            v
 *       bounce_rtpl_decode_file() (Layer A, unchanged, the ONLY parser)
 *            |
 *            v
 *       timed render plan         (this file: note/rest segments, NO samples)
 *            |
 *            v
 *       PCM   -- NOT IMPLEMENTED YET (STEP 14B-34-B)
 *            |
 *            v
 *       backend -- NOT IMPLEMENTED YET (STEP 14B-34-C)
 *
 * WHAT THIS FILE IS NOT
 *   It is NOT the RTPL parser. It does not include any parsing logic, does not
 *   read `.ott` bytes itself, and holds no note table. It calls Layer A and
 *   consumes Layer A's `BounceRtplSong` as-is.
 *
 *   It is NOT a playback implementation. It opens no audio device, links no
 *   audio library, creates no thread, allocates no queue, and mixes nothing.
 *   It generates no PCM sample: `bounce_synth_render_pcm()` is an explicit
 *   stub that reports "not implemented" so that fact is testable rather than
 *   merely asserted in a comment.
 *
 *   It is NOT a Layer B extension. Nothing here is added to `BounceAudioEvent`,
 *   and no Layer B state is read or written. Layer B remains identity-only and
 *   remains the gameplay -> audio boundary.
 *
 * ---------------------------------------------------------------------------
 * EVIDENCE LABELLING -- READ THIS BEFORE CHANGING ANY DEFAULT
 * ---------------------------------------------------------------------------
 * Every value in this header is one of exactly three kinds. The kind is stated
 * at the point of use. Do not silently promote a reconstruction into a fact.
 *
 *   ORIGINAL FACT
 *       Recovered from the original evidence. Examples: the identity-to-asset
 *       mapping (`e.java:68-70`), the decoded note list, `nominal_frequency_hz`,
 *       `duration_ms`, and the statement that Natural style means a rest occurs
 *       between notes.
 *
 *   RECONSTRUCTION PARAMETER
 *       A Target B engineering choice with NO evidentiary weight. Declared by
 *       STEP 14B-33, applied here. This includes the waveform, the Natural rest
 *       ratio, the sample rate, the sample format and the channel count.
 *
 *   UNKNOWN
 *       Not determined by any available evidence. The two load-bearing ones are
 *       the original waveform/timbre and the original Natural rest magnitude.
 *       Both are `UNKNOWN -- EVIDENCE EXHAUSTED` (STEP 14B-22, STEP 14B-31,
 *       STEP 14B-33 platform audit). They are NOT reopened by this module, and
 *       nothing here may be described as a recovered Nokia value.
 *
 * In particular: the default waveform below is a band-limited sine and the
 * default rest ratio is 0.25. Neither is a claim about any Nokia handset. A
 * comment must never claim that Nokia used a sine, a square wave, a 25% rest,
 * or any other recovered value.
 */

#ifndef BOUNCE_NATIVE_APP_AUDIO_SYNTH_H
#define BOUNCE_NATIVE_APP_AUDIO_SYNTH_H

#include "audio_event.h"
#include "rtpl_decoder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Status -- a separate domain from Layer A's, so a failure is attributable
 * ------------------------------------------------------------------------- */

/*
 * Deliberately a distinct type from `BounceRtplStatus`. A caller can therefore
 * tell a DECODER failure from a RENDERER failure without matching on text, as
 * STEP 14B-33 section 18 requires. Every status below means "this module
 * declined"; none of them ever aborts, terminates, or synthesises a fallback
 * sound.
 */
typedef enum BounceSynthStatus {
    BOUNCE_SYNTH_OK = 0,
    BOUNCE_SYNTH_ERR_NULL_ARGUMENT = 1,
    BOUNCE_SYNTH_ERR_NO_RESOURCE = 2,
    BOUNCE_SYNTH_ERR_INVALID_CONFIG = 3,
    /* Layer A refused the payload. The caller holds the exact BounceRtplStatus. */
    BOUNCE_SYNTH_ERR_DECODE_FAILED = 4,
    BOUNCE_SYNTH_ERR_EMPTY_NOTE_SEQUENCE = 5,
    BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE = 6,
    /* The caller's frame buffer is smaller than plan->total_frames. */
    BOUNCE_SYNTH_ERR_BUFFER_TOO_SMALL = 7,
    /* The plan does not describe the song it was supplied with. */
    BOUNCE_SYNTH_ERR_PLAN_MISMATCH = 8,
    /*
     * The PCM boundary was a stub up to and including STEP 14B-34-A. Retained so
     * that historical reports and any out-of-tree caller referring to the old
     * seam keep a meaningful name; STEP 14B-34-B no longer returns it.
     */
    BOUNCE_SYNTH_ERR_RENDER_NOT_IMPLEMENTED = 9
} BounceSynthStatus;

/* Stable human-readable text for a status. Never NULL. */
const char *bounce_synth_status_text(BounceSynthStatus status);

/* -------------------------------------------------------------------------
 * Reconstruction parameters (STEP 14B-33 section 11 and 13)
 * ------------------------------------------------------------------------- */

/*
 * TARGET B RECONSTRUCTION WAVEFORM.
 *
 * The original waveform is UNKNOWN -- EVIDENCE EXHAUSTED. The governing
 * Smart Messaging specification, the Nokia UI API Javadoc, 616 classes across
 * two Nokia SDK generations, the original game source, the original `.ott`
 * bytes, the Nokia 5100 / NPM-6 service documentation, and every local binary
 * were searched and none names a waveform (STEP 14B-22, STEP 14B-31).
 *
 * `SINE_BANDLIMITED` is therefore the Target B implementation BASELINE only. It
 * was chosen on engineering grounds, chiefly that a sine has a single spectral
 * bin so the frequency assertions in the self-test are waveform-independent and
 * a future substitution is provably safe. It was NOT chosen because it sounds
 * like a Nokia handset, and it is NOT a claim about any Nokia device.
 *
 * The alternatives below exist so the choice is a one-value edit. They are not
 * implemented by STEP 14B-34-A and are not claimed to be any more or less
 * Nokia-like than the baseline.
 */
typedef enum BounceSynthWaveform {
    BOUNCE_SYNTH_WAVE_SINE_BANDLIMITED = 0, /* Target B reconstruction baseline */
    BOUNCE_SYNTH_WAVE_SQUARE_BANDLIMITED,    /* selectable alternative */
    BOUNCE_SYNTH_WAVE_TRIANGLE_BANDLIMITED   /* selectable alternative */
} BounceSynthWaveform;

/*
 * TARGET B RECONSTRUCTION SAMPLE FORMAT.
 *
 * Signed 16-bit little-endian. Chosen because it is ALSA's native interchange
 * format, so no conversion is needed anywhere, and because it is the easiest
 * format to assert byte-exactly in a self-test. Not a recovered Nokia format.
 */
typedef enum BounceSynthSampleFormat {
    BOUNCE_SYNTH_SAMPLE_S16LE = 0
} BounceSynthSampleFormat;

/*
 * TARGET B RECONSTRUCTION CHANNEL COUNT.
 *
 * Mono. Justified twice: it is the direct path for the sample format above, and
 * it matches how the original actually used audio -- `f.java:432` makes `sound`
 * a per-`a()` local and `f.java:647-648` plays at most one tone per collision
 * pass, so the game never needed stereo.
 */
typedef enum BounceSynthChannelCount {
    BOUNCE_SYNTH_CHANNELS_MONO = 1
} BounceSynthChannelCount;

/* Upper bound accepted for a Target B sample rate. Deterministic, not a probe. */
#define BOUNCE_SYNTH_SAMPLE_RATE_MIN 8000u
#define BOUNCE_SYNTH_SAMPLE_RATE_MAX 192000u

/*
 * The complete reconstruction parameter block.
 *
 * Everything in this struct is a RECONSTRUCTION PARAMETER or a Target B
 * engineering decision. Nothing in it is an ORIGINAL FACT.
 */
typedef struct BounceSynthConfig {
    unsigned int sample_rate;      /* Target B: 44100 */
    BounceSynthWaveform waveform;  /* Target B: SINE_BANDLIMITED */
    BounceSynthSampleFormat sample_format; /* Target B: S16LE */
    BounceSynthChannelCount channels;      /* Target B: MONO (1) */
    /*
     * TARGET B RECONSTRUCTION PARAMETER -- Natural rest ratio.
     *
     * The SEMANTIC is an ORIGINAL FACT: Smart Messaging Rev 2.0.0 defines
     * Natural style as a "rest between notes", and `rtpl_decoder.h` records that
     * as `BOUNCE_RTPL_STYLE_NATURAL`.
     *
     * The MAGNITUDE is UNKNOWN. The format specifies none, the Nokia Sound API
     * does not expose the style vocabulary at all, and no platform document
     * supplies a value (STEP 14B-22, STEP 14B-31, STEP 14B-33). The 0.25
     * default is a Target B reconstruction: it is expressed as a ratio of the
     * note's own decoded duration so it scales with the asset instead of
     * introducing a second absolute timing constant, and it is held in exactly
     * one place so it can be changed without touching any other file.
     *
     * The original Nokia rest magnitude was NOT 0.25 of anything. This number
     * is a declared reconstruction, not a recovered value.
     */
    double natural_rest_ratio;
} BounceSynthConfig;

/* Fill `out` with the Target B baseline declared by STEP 14B-33. */
void bounce_synth_default_config(BounceSynthConfig *out);

/* Stable human-readable name of a waveform. Never NULL. */
const char *bounce_synth_waveform_name(BounceSynthWaveform waveform);

/*
 * Validate a configuration.
 *
 * Returns BOUNCE_SYNTH_OK when the configuration is usable and
 * BOUNCE_SYNTH_ERR_INVALID_CONFIG otherwise. Rejects, deterministically: a
 * sample rate outside the accepted range, a NaN or out-of-range rest ratio, an
 * unsupported sample format, a channel count other than mono, and any
 * unrecognised waveform. Safe with `config == NULL` or `out == NULL`; never
 * aborts, never terminates.
 */
BounceSynthStatus bounce_synth_config_validate(const BounceSynthConfig *config);

/* -------------------------------------------------------------------------
 * Resource mapping (ORIGINAL FACT, from e.java:68-70)
 * ------------------------------------------------------------------------- */

/*
 * The mapping is ORIGINAL FACT and is total and static: three identities, three
 * resources, no dynamic selection.
 *
 *     BOUNCE_AUDIO_EVENT_UP     -> sounds/up.ott       e.java:68
 *     BOUNCE_AUDIO_EVENT_PICKUP -> sounds/pickup.ott   e.java:69
 *     BOUNCE_AUDIO_EVENT_POP    -> sounds/pop.ott      e.java:70
 *     BOUNCE_AUDIO_EVENT_NONE   -> no resource
 *
 * The table is `static const` and read-only. `NONE` has no resource because the
 * original never plays for it, and any unrecognised identity is refused rather
 * than guessed.
 */
typedef struct BounceSynthResource {
    BounceAudioEvent event;
    const char *relative_path; /* NULL when the identity has no resource */
    bool has_resource;
} BounceSynthResource;

/*
 * Resolve an identity to its resource.
 *
 * Returns BOUNCE_SYNTH_OK for a mapped identity and
 * BOUNCE_SYNTH_ERR_NO_RESOURCE for BOUNCE_AUDIO_EVENT_NONE or any
 * unrecognised identity. In the latter case `out` is still fully written, with
 * `has_resource == false` and `relative_path == NULL`, so a caller that
 * ignores the status still cannot dereference a missing path.
 */
BounceSynthStatus bounce_synth_map_event(BounceAudioEvent event,
                                        BounceSynthResource *out);

/* Read-only access to the mapping table. Returns NULL for NONE/unknown. */
const char *bounce_synth_resource_path(BounceAudioEvent event);

/* -------------------------------------------------------------------------
 * Timed render plan -- the boundary STEP 14B-34-B fills in
 * ------------------------------------------------------------------------- */

/*
 * A plan segment is a span of the song in TIME, with no samples attached. This
 * is the whole deliverable of STEP 14B-34-A and it is deliberately not audio.
 */
typedef enum BounceSynthSegmentKind {
    /* A sounding note. `frequency_hz` is the decoder's nominal value. */
    BOUNCE_SYNTH_SEGMENT_NOTE = 0,
    /*
     * A silence inserted between notes because the song's style has a rest.
     * This is the segment whose length the unknown Natural rest magnitude would
     * govern. `note_index` is the index of the note that FOLLOWS it.
     */
    BOUNCE_SYNTH_SEGMENT_REST = 1,
    /*
     * A decoded note whose Table 3.8-6 value is 0, i.e. a pause. This is a
     * silence that came from the DATA, which is a different fact from the
     * style-induced rest above, so the two are not conflated. No original
     * Bounce asset contains a pause; the branch exists so the plan is total.
     */
    BOUNCE_SYNTH_SEGMENT_PAUSE = 2
} BounceSynthSegmentKind;

typedef struct BounceSynthSegment {
    BounceSynthSegmentKind kind;
    unsigned int note_index;    /* index into song->notes; unset for a rest */
    unsigned int duration_ms;   /* NOTE: for a rest this is the derived value */
    double frequency_hz;        /* 0.0 for a rest or a pause */
    unsigned long frame_offset; /* cumulative frames from the start of the song */
    unsigned long frame_count;  /* frames this segment occupies */
} BounceSynthSegment;

/* Two segments per note at most: a sounding note (or pause) plus one rest. */
#define BOUNCE_SYNTH_MAX_SEGMENTS (2u * BOUNCE_RTPL_MAX_NOTES)

typedef struct BounceSynthPlan {
    unsigned int segment_count;
    BounceSynthSegment segments[BOUNCE_SYNTH_MAX_SEGMENTS];
    unsigned long total_frames;
    unsigned long total_duration_ms; /* note sound time PLUS the derived rests */
    unsigned int note_count;         /* equals song->note_count */
    unsigned int rest_count;         /* style-induced rests only */
} BounceSynthPlan;

/*
 * Build the timed plan for a decoded song.
 *
 * Consumes Layer A's `BounceRtplSong` directly; copies no note table and
 * re-parses nothing. Uses `notes[i].duration_ms` UNCHANGED as note sound time
 * and derives each rest as `duration_ms * config->natural_rest_ratio`. The
 * decoded duration is never modified, and the decoder is never called from
 * here.
 *
 * Style handling, all from the format's own semantics:
 *   Natural    -> a rest is inserted after each note  (ORIGINAL FACT)
 *   Continuous -> no rest is inserted               (ORIGINAL FACT)
 *   Staccato   -> refused with BOUNCE_SYNTH_ERR_UNSUPPORTED_STYLE. Its rest is
 *                 known to be longer than Natural's, but STEP 14B-33 authorised
 *                 a reconstruction ratio for Natural only, and inventing a
 *                 Staccato ratio here would be an undeclared reconstruction. No
 *                 original asset uses Staccato, so refusing is safe.
 *   reserved   -> refused, mirroring the decoder's own rejection.
 *
 * Returns BOUNCE_SYNTH_OK on success. Fails safely, never aborts and never
 * terminates, with BOUNCE_SYNTH_ERR_NULL_ARGUMENT, _INVALID_CONFIG,
 * _EMPTY_NOTE_SEQUENCE or _UNSUPPORTED_STYLE. `plan_out` is always fully
 * written first, so a caller can never read a partially built plan.
 */
BounceSynthStatus bounce_synth_render_plan(const BounceRtplSong *song,
                                           const BounceSynthConfig *config,
                                           BounceSynthPlan *plan_out);

/*
 * The PCM boundary. NOT IMPLEMENTED BY STEP 14B-34-A.
 *
 * This exists as a declared, testable seam. It always returns
 * BOUNCE_SYNTH_ERR_RENDER_NOT_IMPLEMENTED and writes no samples, so the
 * self-test can prove that no sample generation exists yet rather than taking
 * the absence on trust. STEP 14B-34-B replaces the body.
 */
BounceSynthStatus bounce_synth_render_pcm(const BounceRtplSong *song,
                                          const BounceSynthConfig *config,
                                          const BounceSynthPlan *plan,
                                          void *sample_buffer,
                                          size_t sample_buffer_frames,
                                          size_t *frames_written);

/*
 * The full device-free chain: identity -> resource -> Layer A decode -> plan.
 *
 * This is the whole of Layer C that STEP 14B-34-A provides. It reads one file,
 * hands the bytes to Layer A unchanged, and returns a plan. It opens no audio
 * device, links no audio library, and starts no thread.
 *
 * `resource_root` is a filesystem directory such as "src/main/resources", the
 * same convention `check-rtpl` and `assets` already use. `song_out` may be NULL
 * if the caller only wants the plan. `plan_out` may be NULL if the caller only
 * wants the decoded song.
 */
BounceSynthStatus bounce_synth_prepare(BounceAudioEvent event,
                                      const char *resource_root,
                                      const BounceSynthConfig *config,
                                      BounceRtplSong *song_out,
                                      BounceSynthPlan *plan_out);

/* -------------------------------------------------------------------------
 * Self-test
 * ------------------------------------------------------------------------- */

/*
 * STEP 14B-34-A skeleton self-test.
 *
 * Proves the mapping, the reconstruction configuration, the note-timing
 * contract, and the absence of everything a skeleton must not contain. It
 * consumes the ORIGINAL `.ott` assets through Layer A, so the plan is checked
 * against real decoded data rather than a hand-written table.
 *
 * `resource_root` is required. `output_file` may be NULL. Returns 0 when every
 * assertion passed and -1 otherwise. It never opens a device, never plays, and
 * never alters Layer B state.
 */
int bounce_synth_verify(const char *resource_root, FILE *output_file);

/*
 * STEP 14B-34-B renderer self-test.
 *
 * Verifies the deterministic S16LE renderer against the original assets. It
 * proves that every rendered note carries the frequency Layer A decoded, that
 * every note keeps its decoded duration exactly, that rests are exact silence,
 * that no sample leaves the int16 range, and that the renderer is a pure
 * function of its inputs. It opens no device, links no audio library, and
 * starts no thread.
 *
 * `resource_root` is required. `output_file` may be NULL. Returns 0 when every
 * assertion passed and -1 otherwise.
 */
int bounce_synth_verify_pcm(const char *resource_root, FILE *output_file);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_SYNTH_H */
