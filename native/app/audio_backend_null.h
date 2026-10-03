/*
 * audio_backend_null.h -- STEP 14B-34-C Null audio backend.
 *
 * WHAT IT IS
 *   A portable backend with the same interface as ALSA that produces no sound at
 *   all. It exists so that headless hosts, unit tests, and future Windows
 *   builds have somewhere to point the audio path when no real device exists or
 *   when producing sound would be inappropriate.
 *
 * WHAT IT IS NOT
 *   It is NOT a silent fallback. Nothing in this project substitutes it for a
 *   real backend automatically; if a caller chooses Null, that is a decision the
 *   caller made and can see. A failed ALSA open is reported as a failure, never
 *   quietly rerouted here.
 *
 * PORTABILITY
 *   Portable by construction: this header includes only <stddef.h> and
 *   <stdint.h>. It has no ALSA include, no Windows include, and no engine
 *   include, and it would compile unchanged on a platform with no audio library
 *   at all.
 *
 * MEMORY
 *   The state is caller-supplied and fixed size, so initialising a Null backend
 *   performs NO dynamic allocation. The backend counts frames, bytes, and write
 *   calls; it deliberately does NOT retain the PCM, so a test cannot
 *   accidentally grow a buffer or build a queue. There is no FIFO, no ring
 *   buffer, and no scheduler here.
 */

#ifndef BOUNCE_NATIVE_APP_AUDIO_BACKEND_NULL_H
#define BOUNCE_NATIVE_APP_AUDIO_BACKEND_NULL_H

#include <stddef.h>
#include <stdint.h>

#include "audio_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fixed-size, caller-owned state. Every counter is a plain integer so a test can
 * assert exact totals. Nothing here is a queue: the counts are summaries, not
 * storage.
 */
typedef struct BounceAudioNullState {
    /* 1 between a successful open and the matching close. */
    int open;
    /* 1 after a close, so a test can tell close from never-opened. */
    int closed_once;
    unsigned int sample_rate_hz;
    unsigned int channels;
    unsigned long long total_frames;
    unsigned long long total_bytes;
    unsigned long long write_count;
    unsigned long long drain_count;
    char device[32];
    char error[128];
} BounceAudioNullState;

/*
 * Populate `backend` so it dispatches to the Null implementation, using
 * `state` as its private storage. `state` is supplied by the caller and must
 * outlive the backend.
 *
 * `device_name` may be NULL, in which case BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT
 * is recorded. The name is stored for diagnostics only; no device is contacted.
 *
 * Returns BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT if either pointer is NULL,
 * having written nothing.
 */
BounceAudioBackendStatus bounce_audio_backend_null_init(
    BounceAudioBackend *backend,
    BounceAudioNullState *state,
    const char *device_name);

/* Zero the counters. Safe with NULL. */
void bounce_audio_backend_null_reset(BounceAudioNullState *state);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_BACKEND_NULL_H */
