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

#include <stdatomic.h>
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
    /*
     * BL-004 PARITY FIX -- the loudest |sample| seen since the last reset, and the
     * peak of the most recent write.
     *
     * THIS IS A SUMMARY, NOT STORAGE, and that is the distinction the header above
     * draws. Nothing here retains PCM: there is no buffer, no queue and no ring, so
     * a test still cannot grow memory or build a backlog. Two integers answer a
     * question the existing counters could not -- what actually reached the output
     * boundary, numerically.
     *
     * IT EXISTS BECAUSE THE GAIN IS INVISIBLE OTHERWISE. audio_player.c applies
     * volume to slot->pcm immediately before write_pcm() and then hands the buffer
     * away, so with the PCM discarded the only observable effect of a volume change
     * was the absence of sound -- which a Null backend produces anyway. Without
     * this, "volume 0 is silent" and "nothing happened at all" were the same
     * observation, and the volume setting had no test that could fail.
     *
     * peak_sample holds the maximum over every write since the reset;
     * last_write_peak holds the maximum of the single most recent write, which is
     * what lets one sound be compared against another at two volumes.
     *
     * WHY THEY ARE ATOMIC RATHER THAN PLAIN unsigned int. The audio worker is the
     * only writer of these two fields and it does so with player->mutex RELEASED --
     * the worker deliberately drops the lock across render and write so a slow
     * device never blocks the gameplay thread -- while a reader such as
     * bounce_audio_player_null_peak() runs on that gameplay thread. So the two
     * genuinely race, and on the volume field's own reasoning ("a 32-bit write is
     * atomic, so no lock is needed") this would be a data race: undefined behaviour
     * by C11, even where it happens to work. These are _Atomic so the race is
     * well-defined rather than merely unlikely to be observed.
     *
     * ATOMIC RATHER THAN A LOCK, DELIBERATELY. The alternative -- taking
     * player->mutex to read, or adding a dedicated mutex taken inside
     * write_pcm() -- would put a lock acquisition on the audio hot path, once per
     * rendered block, to protect two words that no control flow depends on. All the
     * accesses here are single loads and stores of a lock-free type, so
     * memory_order_relaxed is sufficient: there is no data to order with respect
     * to, only two independent counters that must never be seen half-updated. The
     * build asserts the lock-freedom at compile time rather than leaving it to
     * chance; see audio_backend_null.c.
     */
    atomic_uint peak_sample;
    atomic_uint last_write_peak;
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
