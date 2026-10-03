#ifndef BOUNCE_NATIVE_APP_AUDIO_EVENT_H
#define BOUNCE_NATIVE_APP_AUDIO_EVENT_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STEP 14B-27 -- LAYER B, GAMEPLAY AUDIO EVENT IDENTITY.
 *
 * This is the narrow boundary between a gameplay event and the sound that the
 * original played for it. It carries an IDENTITY and nothing else.
 *
 * WHAT THIS IS NOT
 *   - It is not audio playback. Nothing here opens a device, writes PCM, or
 *     produces a waveform. There is deliberately no `play()` call.
 *   - It is not the RTPL decoder. It does not include `rtpl_decoder.h`, does
 *     not call `bounce_rtpl_decode()`, and never passes a `BounceRtplSong`
 *     through gameplay code. Layer A and Layer B stay separate.
 *   - It is not an event framework. There is no queue, ring buffer, FIFO,
 *     dispatcher, or general-purpose event type.
 *
 * WHY AN IDENTITY RATHER THAN A TILE
 *   The original plays one of three resources, and SEVERAL different gameplay
 *   causes share each of them. STEP 14B-25 traced the resource mapping:
 *
 *       e.java:68  soundUp     = LoadSound("/sounds/up.ott")
 *       e.java:69  soundPickup = LoadSound("/sounds/pickup.ott")
 *       e.java:70  soundPop    = LoadSound("/sounds/pop.ott")
 *
 *   and all 17 gameplay call sites use one of those three objects. So the
 *   correct granularity is the SOUND, not the tile: crystal, gravity, jump and
 *   boost are four distinct causes that all play `pickup.ott`.
 *
 *   STEP 14B-26 established that `BounceDebugTile` is NOT a sound identity:
 *   CRYSTAL, EXIT_DOOR, GRAVITY and BOOST all map onto `pickup.ott`, and POP
 *   is not a tile event at all. That enum is an observation vocabulary and is
 *   deliberately not reused here.
 *
 * WHY NO PLAYBACK ARGUMENT
 *   STEP 14B-25 established that all 17 gameplay sites pass `1`; the only other
 *   `play()` in the whole program is `play(3)` at `e.java:468`, which is the
 *   load-time IOException fallback and is NOT gameplay. `play(1)` is therefore
 *   an INVARIANT of the original's gameplay audio, and storing a field that is
 *   provably constant in 17/17 cases would carry no information. It belongs to
 *   the future adapter, not to the event.
 *
 * WHY EMISSION IS SYNCHRONOUS
 *   In the original, `Sound sound = null` is declared at `f.java:432` INSIDE
 *   `f.a(x, y, tileY, tileX)` and the single `sound.play(1)` at `f.java:648` is
 *   in that same method. The footprint walk `TODO_CheckBallCollision`
 *   (`f.java:159-164`) calls `a()` once PER TILE, and returns early only on a
 *   blocking verdict -- so a walk visiting two sound-producing tiles performs
 *   TWO plays, one per `a()` invocation.
 *
 *   Emitting synchronously at the point each arm is decided therefore
 *   reproduces that exactly: one native walk containing N sound arms emits N
 *   events, in walk order, and the consumer sees each one immediately. No
 *   storage is involved, so no event can overwrite another, and no ordering
 *   field is needed because the order IS the program order.
 *
 *   This is why there is deliberately no "last sound" or "pending sound"
 *   field. A single scalar would silently drop every event but the last of a
 *   multi-sound walk, which is exactly the collapse the original does not do.
 */

/*
 * The three gameplay sound identities, plus one sentinel. Exactly three
 * meaningful values, matching the three original resources one-to-one.
 *
 * BOUNCE_AUDIO_EVENT_NONE is an IMPLEMENTATION-NEUTRAL SENTINEL, justified by
 * the surrounding C API. Every consumer-facing API in this codebase takes a
 * pointer or a context that may legitimately be absent, and treats that absence
 * as "no operation performed" rather than as an error: see
 * `bounce_collision_query`, `bounce_debug_event_death` (whose `app_debug()`
 * accessor returns NULL when no tracker is attached), and
 * `bounce_visual_assets_draw_tile`. NONE follows that existing convention, so a
 * caller with no audio consumer installed can pass NONE -- or can more simply
 * install no sink at all. It is NOT a fourth sound: no gameplay event maps to
 * it, and bounce_audio_event_emit() never forwards it to a consumer.
 */
typedef enum BounceAudioEvent {
    BOUNCE_AUDIO_EVENT_NONE = 0,   /* sentinel: no sound. Never emitted. */
    BOUNCE_AUDIO_EVENT_UP = 1,     /* sounds/up.ott     -- hoop collection */
    BOUNCE_AUDIO_EVENT_PICKUP = 2, /* sounds/pickup.ott -- crystal, gravity,
                                    *                      jump, boost */
    BOUNCE_AUDIO_EVENT_POP = 3     /* sounds/pop.ott    -- death */
} BounceAudioEvent;

/*
 * The consumer boundary. A future audio adapter installs one of these; until
 * then none is installed and `bounce_audio_emit()` is a no-op.
 *
 * `context` is opaque and is returned unchanged, matching the caller-pointer
 * convention used by `bounce_debug_tile()` and the other observation sinks.
 *
 * No such consumer is implemented in this checkpoint.
 */
typedef void (*BounceAudioEventSink)(void *context, BounceAudioEvent event);

/*
 * Install the consumer. Passing a NULL sink or NULL context removes any
 * previously installed consumer, after which emission is a silent no-op.
 * Returns 0 on success.
 *
 * This is the whole of the "wiring" that exists so far: it lets a later
 * checkpoint attach an adapter without changing any gameplay call site.
 */
int bounce_audio_event_set_sink(BounceAudioEventSink sink, void *context);

/* True when a consumer is installed. Safe to call at any time. */
bool bounce_audio_event_has_sink(void);

/*
 * Emit one gameplay sound identity, synchronously, in program order.
 *
 * Returns 0 when a consumer received the event, 1 when the event was
 * BOUNCE_AUDIO_EVENT_NONE, and -1 for a NULL app-independent misuse (a
 * reserved enum value). With no sink installed it returns 0 and does nothing,
 * so gameplay is unaffected while no audio layer exists.
 *
 * This does not play anything. It reports an identity and returns.
 */
int bounce_audio_event_emit(BounceAudioEvent event);

/* Stable human-readable name, for reports and tests. Never NULL. */
const char *bounce_audio_event_name(BounceAudioEvent event);

/*
 * Self-test for the emission boundary.
 *
 * Proves, without a queue and without touching gameplay, that sequential
 * emissions reach the consumer immediately and in order -- the property
 * STEP 14B-27 requires and the property a scalar field could not provide.
 *
 * `output_file` may be NULL. Returns 0 when every assertion passed, -1
 * otherwise. The sink installed for the duration of the test is always
 * removed before returning, including on failure.
 */
int bounce_audio_event_verify(FILE *output_file);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_EVENT_H */
