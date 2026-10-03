/*
 * audio_player.h -- STEP 14B-34-D audio integration layer (Target B).
 *
 * WHAT THIS FILE IS
 *   The seam that finally connects Layer B to the portable backend:
 *
 *       gameplay --emit--> Layer B sink --(enqueue)--> bounded FIFO
 *                    --> dedicated audio thread --(render, write)--> backend
 *
 *   It exists for one overriding reason: a real audio device can block, so
 *   nothing that might block may run on the gameplay thread. The sink therefore
 *   does only O(1) work -- validate an identity, copy a small descriptor, signal
 *   a condition variable -- and every decode, render, and device write happens
 *   on the audio thread.
 *
 * WHAT IT IS NOT
 *   It is not a renderer, a decoder, a scheduler, or a mixer.
 *
 *     It does not decide what a sound should be. That is Layer B's identity and
 *       Layer A's / the renderer's job, and neither file is touched.
 *     It does not parse RTPL, choose a waveform, derive a frequency, size a
 *       rest, or shape an envelope. It calls the STEP 14B-34-B renderer
 *       unchanged.
 *     It is not a mixer. Overlapping plays are played sequentially, which is an
 *       IMPLEMENTATION POLICY, not a recovered behaviour.
 *     It is not a queue of unlimited growth. The FIFO is bounded, pre-allocated
 *       once, and freed at shutdown.
 *
 * THREADING
 *   One producer (the gameplay thread, through the Layer B sink) and one
 *   consumer (the audio thread). A single mutex guards the FIFO, the stop flag,
 *   and the counters. A single condition variable signals the consumer.
 *
 *   The mutex is held only for pointer and counter work. The blocking backend
 *   write is deliberately performed with the mutex RELEASED, so a slow device
 *   cannot stall the producer. Shutdown therefore joins the thread before the
 *   backend is closed, which is what makes the close safe.
 *
 * PCM OWNERSHIP
 *   Every queued slot owns a pre-allocated PCM buffer that was allocated once,
 *   at create time, and freed once, at destroy time. No buffer ever crosses a
 *   thread boundary as a borrowed pointer: the producer stores only an event
 *   identity, and the audio thread fills and writes the slot's own buffer. No
 *   caller's stack buffer is retained, so there is no dangling pointer and no
 *   use-after-free.
 *
 * EVIDENCE
 *   The waveform, the hardware timbre, and the Natural rest magnitude remain
 *   UNKNOWN / EVIDENCE EXHAUSTED. The queue capacity, the drop policy, the drain
 *   policy, and the pre-decode cache are IMPLEMENTATION POLICY chosen for this
 *   layer, and are labelled as such at their definitions.
 */

#ifndef BOUNCE_NATIVE_APP_AUDIO_PLAYER_H
#define BOUNCE_NATIVE_APP_AUDIO_PLAYER_H

#include <stddef.h>
#include <stdio.h>

#include "audio_backend.h"
#include "audio_event.h"

/*
 * Whether this translation unit may reference the ALSA backend.
 *
 * The default is 0, so a plain compile of this module is portable to a host with
 * no audio library at all. The game build defines it to 1 when pkg-config finds
 * ALSA, which is the only place the dependency is wanted. With it at 0, the
 * AUTO and ALSA backend choices both resolve to "unavailable" and audio is
 * simply off, which is a correct outcome rather than a build failure.
 */
#ifndef BOUNCE_AUDIO_HAVE_ALSA
#define BOUNCE_AUDIO_HAVE_ALSA 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Which backend to use.
 *
 * AUTO is the runtime default and it does NOT fall back. If ALSA cannot be
 * opened, the player reports that it is unavailable and the caller is expected
 * to install no sink at all, leaving the game's existing no-op behaviour in
 * place. Silently substituting the Null backend would hide a real failure, so
 * AUTO never does it. NULL is available only when a caller asks for it by name,
 * for tests, headless hosts, and debugging.
 */
typedef enum BounceAudioPlayerBackend {
    BOUNCE_AUDIO_PLAYER_BACKEND_AUTO = 0, /* try ALSA; on failure, audio off */
    BOUNCE_AUDIO_PLAYER_BACKEND_NULL = 1, /* explicit, for tests/headless */
    BOUNCE_AUDIO_PLAYER_BACKEND_ALSA = 2  /* force ALSA, never fall back */
} BounceAudioPlayerBackend;

/*
 * IMPLEMENTATION POLICY -- the layer's own tuning, not a recovered value.
 *
 * QUEUE_CAPACITY: how many pending events may exist at once. A tick is 40 ms and
 * a rendered sound is 300-750 ms, so the audio thread consumes roughly one
 * sound per 19 ticks; sustained rapid triggering therefore WILL fill the queue
 * and will drop, which is correct and bounded rather than growing without limit.
 * Eight covers one full burst of a single tick's footprint walk.
 *
 * NO FRAME CONSTANT IS HARDCODED: the per-slot buffer size is derived at create
 * time from the largest planned song plus 25% headroom, so memory tracks the
 * data instead of a guessed constant.
 */
#define BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY 8u
#define BOUNCE_AUDIO_PLAYER_HEADROOM_PERCENT 25u

/*
 * Shutdown policy. The original behaviour is UNKNOWN, so this is declared
 * rather than inferred.
 *
 * DISCARD (the default) drops anything still queued at shutdown, so the game
 * never waits. DRAIN waits up to `drain_timeout_ms` for the worker to finish
 * what is already queued, so a sound in flight is allowed to finish.
 */
typedef enum BounceAudioPlayerShutdown {
    BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD = 0,
    BOUNCE_AUDIO_PLAYER_SHUTDOWN_DRAIN = 1
} BounceAudioPlayerShutdown;

typedef struct BounceAudioPlayerConfig {
    /* Filesystem root holding sounds/up.ott, pickup.ott, pop.ott. */
    const char *resource_root;
    BounceAudioPlayerBackend backend;
    /* NULL means the backend's own default, which is ALSA "default". */
    const char *device_name;
    BounceAudioPlayerShutdown shutdown;
    /* Only consulted for DRAIN. 0 means do not wait. */
    unsigned int drain_timeout_ms;
} BounceAudioPlayerConfig;

typedef struct BounceAudioPlayerStats {
    /* Identities the sink accepted and queued. */
    unsigned long long events_enqueued;
    /* NONE, an unknown identity, or an event with no asset: rejected, not queued. */
    unsigned long long events_rejected;
    /* Events discarded because the FIFO was full (drop-oldest policy). */
    unsigned long long events_dropped;
    /* Events the audio thread rendered and handed to the backend. */
    unsigned long long events_played;
    unsigned long long frames_played;
    /* Every failure the backend or the renderer reported. */
    unsigned long long render_errors;
    unsigned long long backend_errors;
    /* Events still queued when the worker stopped. */
    unsigned long long discarded_on_shutdown;
    /* 0 while work may be queued, 1 once the player is shutting down. */
    int accepting;
} BounceAudioPlayerStats;

typedef struct BounceAudioPlayer BounceAudioPlayer;

/* Fill `out` with the documented defaults: AUTO backend, DISCARD shutdown. */
void bounce_audio_player_default_config(BounceAudioPlayerConfig *out);

/*
 * Create the player, pre-decode the three ORIGINAL assets, open the backend, and
 * start the audio thread.
 *
 * Returns NULL on any failure -- a missing asset, a decode error, an unopenable
 * backend, a thread that could not be started. It never aborts, never exits,
 * and never reports success while being unusable. On failure it cleans up
 * everything it had already built, so the caller needs no partial teardown.
 *
 * A NULL result is NORMAL on a host with no audio device, and the correct
 * response is to carry on without audio.
 */
BounceAudioPlayer *bounce_audio_player_create(
    const BounceAudioPlayerConfig *config);

/*
 * Install this player's sink as Layer B's consumer and remove it again on
 * destroy. Safe to call when no sink was installed.
 */
int bounce_audio_player_attach_sink(BounceAudioPlayer *player);
int bounce_audio_player_detach_sink(BounceAudioPlayer *player);

/* Non-zero once the player is running and accepting events. */
int bounce_audio_player_is_running(const BounceAudioPlayer *player);

/* The backend actually in use, e.g. "alsa" or "null". Never NULL. */
const char *bounce_audio_player_backend_name(const BounceAudioPlayer *player);

/* Copy the current counters under the lock. Safe with NULL. */
int bounce_audio_player_stats(BounceAudioPlayer *player,
                              BounceAudioPlayerStats *out);

/*
 * Stop accepting, wake the worker, join it, close the backend, and free
 * everything. After this returns the player pointer is invalid.
 *
 * Idempotence: a second call with the same pointer is not supported, so callers
 * must call it exactly once. It performs no blocking device operation of its own
 * beyond the declared drain timeout.
 */
void bounce_audio_player_destroy(BounceAudioPlayer *player);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_PLAYER_H */
