#ifndef BOUNCE_NATIVE_APP_GAME_TIMER_H
#define BOUNCE_NATIVE_APP_GAME_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-independent game-timer scheduling and Tick-entry boundary.
 *
 * ORIGINAL VERIFIED (see reverse/COLLISION-QUERY-FIDELITY-AUDIT.md):
 *   - BounceTimer.java:17  `this.timer.schedule(this, 0L, 40L)` gives a nominal
 *     40 ms fixed-delay period (~25 Hz).
 *   - b.java:848-850       `GameTimerTick()` calls `Tick()` directly.
 *   - There is no delta-time argument, no accumulator, no interpolation, and no
 *     catch-up loop anywhere in the original path: a java.util.TimerTask runs on
 *     a single thread, so a late run delays the next run rather than bursting.
 *   - b.java:835-839       `StartGameTimer()` is idempotent
 *     (`if (this.GameTimer != null) return;`).
 *
 * NATIVE IMPLEMENTATION BOUNDARY: the original's concrete java.util.Timer is not
 * modeled. This is a platform-independent scheduling contract so that X11, SDL,
 * or any other backend can drive the same tick, and it carries NO claim of runtime
 * parity with the Java Timer or of 25 Hz accuracy on any OS scheduler.
 *
 * This milestone deliberately models ONLY the scheduling contract and the
 * callback/Tick-entry boundary. The Tick() body is not executed: there is no
 * player update, collision, dynamic-thorn update, camera update, render, audio,
 * persistence, or input work in this module, and it takes no clock reading and no
 * frame delta. Deciding *when* a backend fires the callback is the backend's
 * responsibility.
 */
enum {
    /* BounceTimer.java:17 — nominal fixed-delay period. */
    BOUNCE_GAME_TIMER_PERIOD_MS = 40
};

/*
 * Timer state. Deliberately minimal: there is no deadline, no accumulator, no
 * missed-tick count, no previous-timestamp, and no delta field, because the
 * original has none. tick_entry_count is a boundary observation counter, not
 * physics state.
 */
typedef struct BounceGameTimer {
    bool started;
    /* e.java:221 Tick() entries observed. */
    uint64_t tick_entry_count;
    /*
     * Source branch selections observed at the e.java:262-266 dispatch:
     * a tick_dispatch_count increment means "the source would invoke
     * f.b() here". It is NOT a completed player tick.
     */
    uint64_t tick_dispatch_count;
    /*
     * f.b() frames actually entered (the pre-physics entry at f.java:652-669).
     * This is deliberately a SEPARATE counter from player_physics_count so
     * "entered the method" is never conflated with "ran the physics".
     */
    uint64_t player_tick_entry_count;
    /*
     * Completed executions of the f.b() physics body. Always 0 at the
     * pre-physics stop point: nothing past f.java:669 is executed.
     */
    uint64_t player_physics_count;
    /*
     * Dynamic-thorn animation steps actually run by this tick, counted only
     * when the e.java:282-283 guard allowed UpdateDynThorns() to execute. A
     * level with no dynamic thorns does not increment it.
     */
    uint64_t dynamic_thorn_update_count;
} BounceGameTimer;

/* Reset to the stopped state with a zeroed entry count. Safe with NULL. */
void bounce_game_timer_init(BounceGameTimer *timer);

/*
 * b.java:835-839 StartGameTimer(): idempotent. Returns 0 when the timer is
 * started, whether by this call or an earlier one, and -1 only for NULL.
 */
int bounce_game_timer_start(BounceGameTimer *timer);

bool bounce_game_timer_is_started(const BounceGameTimer *timer);

/*
 * b.java:841-846 StopGameTimer(): stop the timer. The source nulls its
 * BounceTimer reference, so no further GameTimerTick() can reach Tick(); every
 * native consumer already refuses to run while `started` is false. Safe with
 * NULL. Restored by bounce_game_timer_start().
 */
void bounce_game_timer_stop(BounceGameTimer *timer);

/* The nominal period in milliseconds; always BOUNCE_GAME_TIMER_PERIOD_MS. */
int bounce_game_timer_period_ms(const BounceGameTimer *timer);

/* Number of Tick() entries observed so far. 0 before any callback. */
uint64_t bounce_game_timer_tick_entry_count(const BounceGameTimer *timer);

/*
 * Number of player-tick DISPATCH boundaries selected so far, i.e. how many
 * times the source would have invoked f.b() (e.java:265). This is not a
 * completed player tick; see player_tick_count.
 */
uint64_t bounce_game_timer_tick_dispatch_count(const BounceGameTimer *timer);

/* Completed f.b() physics executions. Always 0 at the pre-physics boundary. */
uint64_t bounce_game_timer_player_physics_count(const BounceGameTimer *timer);

/* f.b() frames entered at the pre-physics stop point. */
uint64_t bounce_game_timer_player_tick_entry_count(const BounceGameTimer *timer);

/*
 * Record that the source's e.java:262-266 dispatch selected the player-tick
 * branch. This marks "the correct player tick would be invoked here" and then
 * STOPS: it does not execute f.b() and does not change
 * player_tick_entry_count or player_physics_count. Returns -1 for a NULL
 * timer, otherwise 0.
 */
int bounce_game_timer_record_dispatch(BounceGameTimer *timer);

/*
 * Record one f.b() frame entry (f.java:652). This counts METHOD ENTRY at the
 * pre-physics stop point; it does not imply that any physics ran, and it never
 * changes player_physics_count. Returns -1 for a NULL or unstarted timer,
 * otherwise 0.
 */
int bounce_game_timer_record_player_tick_entry(BounceGameTimer *timer);

/*
 * Record that the existing native physics continuation actually ran for this
 * tick: app_step_vertical(), then app_step_horizontal(), then the
 * app_update_camera_horizontal() hand-off, exactly as the previously working
 * app_run_player_ticks() path invoked them. This is distinct from
 * player_tick_entry_count, which only means the f.b() frame was entered.
 * Returns -1 for a NULL or unstarted timer, otherwise 0.
 */
int bounce_game_timer_record_player_physics(BounceGameTimer *timer);

/* Number of dynamic-thorn animation steps run by this tick. */
uint64_t bounce_game_timer_dynamic_thorn_update_count(const BounceGameTimer *timer);

/*
 * Record that the source's guarded UpdateDynThorns() call (e.java:282-283)
 * actually ran for this tick. Returns -1 for a NULL or unstarted timer,
 * otherwise 0.
 */
int bounce_game_timer_record_dynamic_thorn_update(BounceGameTimer *timer);

/*
 * The deterministic callback boundary, equivalent to the source path
 *
 *     timer callback  ->  GameTimerTick()  ->  Tick()  [entry]
 *
 * It requires a started timer, increments the Tick-entry count exactly once, and
 * then STOPS at the entry. The Tick() body is not executed and nothing else is
 * called. Returns 0 on a recorded entry, -1 for a NULL or not-started timer.
 */
int bounce_game_timer_callback(BounceGameTimer *timer);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_GAME_TIMER_H */
