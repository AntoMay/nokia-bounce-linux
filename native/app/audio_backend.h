/*
 * audio_backend.h -- STEP 14B-34-C portable audio backend interface.
 *
 * WHAT THIS FILE IS
 *   The device/output boundary for Target B. It carries rendered PCM to
 *   whatever output mechanism a build provides, and reports what happened. It
 *   is the last thing Layer C touches before hardware.
 *
 * WHAT THIS FILE IS NOT -- and this is the whole design constraint
 *   It is deliberately ignorant of everything above it:
 *
 *     no RTPL, no `.ott`, no note, no tempo, no style
 *     no UP / PICKUP / POP, no BounceAudioEvent, no Layer B identity
 *     no gameplay, no collision, no level, no tick
 *     no waveform, no frequency, no Natural rest, no envelope
 *     no FIFO, no queue, no ring buffer, no scheduler, no thread
 *     no platform header of any kind
 *
 *   A backend receives finished `int16_t` frames and nothing else. It cannot
 *   decide what a sound should be, because it is never told.
 *
 * PORTABILITY
 *   This header includes only <stddef.h> and <stdint.h>. It must stay that way:
 *   no <alsa/asoundlib.h>, no Windows headers, no SDL. A backend implementation
 *   may include whatever its platform needs; the interface may not.
 *
 *   Backends are expected to be added without touching Layer A, Layer B, the
 *   RTPL decoder, the renderer, or gameplay. Adding WASAPI means adding a new
 *   `audio_backend_<platform>.{h,c}` pair that populates a BounceAudioBackend,
 *   and nothing else.
 *
 * THREADING CONTRACT -- IMPORTANT
 *   `write_pcm` MAY BLOCK, because a real output device can. It is therefore
 *   documented here as REQUIRED to be called from a dedicated audio thread in a
 *   later step, never from the gameplay thread. This step creates no thread and
 *   starts no loop, so nothing here depends on that future guarantee. A backend
 *   must remain correct when called from whichever thread the integration layer
 *   chooses.
 *
 * ALLOCATION
 *   None of this interface allocates. The caller owns both the
 *   BounceAudioBackend and whatever backend-private state it points at, so a
 *   backend can be built into fixed storage and a test can run with no heap.
 */

#ifndef BOUNCE_NATIVE_APP_AUDIO_BACKEND_H
#define BOUNCE_NATIVE_APP_AUDIO_BACKEND_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Status -- every backend reports through this, never by crashing
 * ------------------------------------------------------------------------- */

/*
 * A backend returns one of these from every operation. There is no fallback
 * path: if a device cannot be opened, the caller is told so and decides what to
 * do. A backend must NEVER silently substitute another device, another
 * backend, or a synthesised tone.
 */
typedef enum BounceAudioBackendStatus {
    BOUNCE_AUDIO_BACKEND_OK = 0,
    /* A required pointer was NULL. Nothing was touched. */
    BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT = 1,
    /* The operation needs an open backend, or the backend is already open. */
    BOUNCE_AUDIO_BACKEND_ERR_STATE = 2,
    /* The requested PCM format is not one this backend can accept. */
    BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT = 3,
    /* No usable output device exists on this host. */
    BOUNCE_AUDIO_BACKEND_ERR_NOT_AVAILABLE = 4,
    /* The device could not be opened. */
    BOUNCE_AUDIO_BACKEND_ERR_OPEN = 5,
    /* The device opened but the requested format could not be negotiated. */
    BOUNCE_AUDIO_BACKEND_ERR_CONFIGURE = 6,
    /* A write failed. */
    BOUNCE_AUDIO_BACKEND_ERR_WRITE = 7,
    /* The device went away mid-stream. */
    BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST = 8
} BounceAudioBackendStatus;

/* Stable human-readable text for a status. Never NULL. */
const char *bounce_audio_backend_status_text(BounceAudioBackendStatus status);

/* -------------------------------------------------------------------------
 * PCM format carried across the boundary
 * ------------------------------------------------------------------------- */

/*
 * SIGNED 16-BIT LITTLE-ENDIAN is the only sample format Target B defines.
 *
 * This is a RECONSTRUCTION / ENGINEERING PARAMETER, not a recovered Nokia value:
 * the format is what STEP 14B-34-B's renderer produces and what the intended
 * backend consumes natively, so no conversion is ever required. No original
 * evidence describes any PCM layout, because the original handed raw `.ott`
 * bytes to a native boundary and never exposed samples (STEP 14B-31).
 */
typedef enum BounceAudioBackendSampleFormat {
    BOUNCE_AUDIO_BACKEND_S16_LE = 0
} BounceAudioBackendSampleFormat;

/*
 * Device name used when the caller does not choose one. "default" is ALSA's
 * own resolution, so it honours the host's configuration instead of hard-coding
 * a card or device number. No `hw:` plug name is ever hard-coded anywhere in
 * this module.
 */
#define BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT "default"

typedef struct BounceAudioBackendFormat {
    /* 44100 Hz for Target B. An engineering parameter, not a Nokia fact. */
    unsigned int sample_rate_hz;
    /* 1 (mono) for Target B, matching the original's one-tone-per-pass use. */
    unsigned int channels;
    BounceAudioBackendSampleFormat sample_format;
    /* 0 means "let the backend choose". */
    size_t period_frames;
} BounceAudioBackendFormat;

/* Fill `out` with the Target B PCM format: 44100 Hz, S16_LE, mono. */
void bounce_audio_backend_default_format(BounceAudioBackendFormat *out);

/* Non-zero when `format` is internally consistent. Never dereferences NULL. */
int bounce_audio_backend_format_is_valid(const BounceAudioBackendFormat *format);

/* -------------------------------------------------------------------------
 * The backend instance
 * ------------------------------------------------------------------------- */

typedef struct BounceAudioBackend BounceAudioBackend;

typedef BounceAudioBackendStatus (*BounceAudioBackendOpenFn)(
    BounceAudioBackend *self,
    const BounceAudioBackendFormat *format,
    const char *device_name);

typedef BounceAudioBackendStatus (*BounceAudioBackendWriteFn)(
    BounceAudioBackend *self,
    const int16_t *samples,
    size_t frames);

typedef BounceAudioBackendStatus (*BounceAudioBackendDrainFn)(
    BounceAudioBackend *self);

typedef void (*BounceAudioBackendCloseFn)(BounceAudioBackend *self);

typedef const char *(*BounceAudioBackendNameFn)(const BounceAudioBackend *self);

typedef const char *(*BounceAudioBackendErrorFn)(const BounceAudioBackend *self);

/*
 * A backend is a function table plus one private pointer. There is no base-class
 * inheritance trick and no size assumption: `state` points at storage the caller
 * supplied to the backend's own initialiser, so a backend of any size fits in
 * the same two words here.
 */
struct BounceAudioBackend {
    BounceAudioBackendOpenFn open;
    BounceAudioBackendWriteFn write_pcm;
    BounceAudioBackendDrainFn drain;
    BounceAudioBackendCloseFn close;
    BounceAudioBackendNameFn name;
    BounceAudioBackendErrorFn last_error;
    void *state;
};

/* Non-zero when every function pointer is present. Never dereferences NULL. */
int bounce_audio_backend_is_complete(const BounceAudioBackend *backend);

/*
 * Shared write precondition, so every backend rejects the same inputs the same
 * way instead of each inventing a policy.
 *
 * Returns BOUNCE_AUDIO_BACKEND_OK when the call may proceed. It returns
 * ERR_NULL_ARGUMENT for a NULL backend or a NULL sample pointer with a non-zero
 * frame count, and ERR_STATE when the backend reports itself unusable. A write
 * of zero frames is a deliberate no-op and returns OK, because an empty buffer
 * is not a caller error.
 *
 * Backends call this first and return its result unchanged.
 */
BounceAudioBackendStatus bounce_audio_backend_precheck_write(
    const BounceAudioBackend *backend,
    const int16_t *samples,
    size_t frames);

/* -------------------------------------------------------------------------
 * Self-test
 * ------------------------------------------------------------------------- */

/*
 * The STEP 14B-34-C self-test lives in audio_backend_check.c, NOT here and NOT
 * in audio_backend.c, and that placement is deliberate.
 *
 * The test spans three modules: the portable interface, the Null backend, and
 * the STEP 14B-34-B renderer. Putting it in audio_backend.c would make the
 * portable interface depend on one specific backend and on the renderer, which
 * inverts the layering this checkpoint exists to establish. So the header stays
 * a pure interface with no test entry point, and the runner owns its own
 * assertions.
 */

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_BACKEND_H */
