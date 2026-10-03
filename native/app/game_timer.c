#include "game_timer.h"

#include <string.h>

void bounce_game_timer_init(BounceGameTimer *timer)
{
    if (timer == NULL)
        return;
    memset(timer, 0, sizeof *timer);
}

int bounce_game_timer_start(BounceGameTimer *timer)
{
    if (timer == NULL)
        return -1;

    /*
     * b.java:836-837: `if (this.GameTimer != null) return;`. The original timer
     * is idempotent, and a second start must not reset the entry count or imply
     * a second schedule.
     */
    timer->started = true;
    return 0;
}

bool bounce_game_timer_is_started(const BounceGameTimer *timer)
{
    return timer != NULL && timer->started;
}

void bounce_game_timer_stop(BounceGameTimer *timer)
{
    /*
     * b.java:841-846 StopGameTimer():
     *
     *   if (this.GameTimer == null) return;
     *   this.GameTimer.stop();
     *   this.GameTimer = null;
     *
     * The source nulls the reference, so a later StartGameTimer() creates a fresh
     * BounceTimer. The native owns the timer by value and has no reference to
     * clear, so the equivalent observable state is `started = false`: every
     * consumer already refuses to run while it is false --
     * bounce_game_timer_callback() at :63, and bounce_game_tick_dispatch()
     * through bounce_game_timer_is_started() at game.c:618. Nothing else is
     * needed, and no counter is disturbed, matching the source, which keeps its
     * counters on the game rather than on the timer object.
     */
    if (timer == NULL)
        return;
    timer->started = false;
}

int bounce_game_timer_period_ms(const BounceGameTimer *timer)
{
    /*
     * The period is the source-derived nominal value and does not depend on any
     * timer state. BounceTimer.java:17 schedules with a 40 ms fixed delay.
     */
    (void)timer;
    return BOUNCE_GAME_TIMER_PERIOD_MS;
}

uint64_t bounce_game_timer_tick_entry_count(const BounceGameTimer *timer)
{
    return timer == NULL ? 0u : timer->tick_entry_count;
}

uint64_t bounce_game_timer_tick_dispatch_count(const BounceGameTimer *timer)
{
    return timer == NULL ? 0u : timer->tick_dispatch_count;
}

uint64_t bounce_game_timer_player_physics_count(const BounceGameTimer *timer)
{
    return timer == NULL ? 0u : timer->player_physics_count;
}

uint64_t bounce_game_timer_player_tick_entry_count(const BounceGameTimer *timer)
{
    return timer == NULL ? 0u : timer->player_tick_entry_count;
}

int bounce_game_timer_record_dispatch(BounceGameTimer *timer)
{
    if (timer == NULL || !timer->started)
        return -1;

    /*
     * e.java:264-265 selected `this.aq.b()`. Recording the selection is the
     * dispatch boundary; the method entry is recorded separately by
     * bounce_game_timer_record_player_tick_entry(), and neither counter implies
     * that any physics ran.
     */
    ++timer->tick_dispatch_count;
    return 0;
}

int bounce_game_timer_record_player_tick_entry(BounceGameTimer *timer)
{
    if (timer == NULL || !timer->started)
        return -1;

    /*
     * The f.b() frame (f.java:652) was entered. This counts METHOD ENTRY at the
     * pre-physics stop point f.java:669 and nothing beyond it, so
     * player_physics_count is deliberately untouched and stays 0.
     */
    ++timer->player_tick_entry_count;
    return 0;
}

int bounce_game_timer_record_player_physics(BounceGameTimer *timer)
{
    if (timer == NULL || !timer->started)
        return -1;

    /*
     * The existing native physics continuation ran for this tick. This is the
     * f.b() body from f.java:670 onward, reached by delegating to the already
     * verified app_step_vertical() / app_step_horizontal() /
     * app_update_camera_horizontal() implementations, which are not modified or
     * re-implemented here.
     */
    ++timer->player_physics_count;
    return 0;
}

uint64_t bounce_game_timer_dynamic_thorn_update_count(const BounceGameTimer *timer)
{
    return timer == NULL ? 0u : timer->dynamic_thorn_update_count;
}

int bounce_game_timer_record_dynamic_thorn_update(BounceGameTimer *timer)
{
    if (timer == NULL || !timer->started)
        return -1;

    /*
     * e.java:282-283 ran UpdateDynThorns() for this tick, mutating only the
     * runtime owner's w/ae state. No LevelTiles write is recorded here, matching
     * the existing implementation.
     */
    ++timer->dynamic_thorn_update_count;
    return 0;
}

int bounce_game_timer_callback(BounceGameTimer *timer)
{
    if (timer == NULL || !timer->started)
        return -1;

    /*
     * b.java:848-850: `GameTimerTick()` calls `Tick()` directly, with no delta
     * argument and no gating inside the callback. The only guard here is that a
     * callback cannot arrive before the timer was started, which mirrors the
     * fact that the original callback exists only because StartGameTimer()
     * created the TimerTask.
     *
     * This is the Tick() ENTRY and nothing more. The Tick() body is not
     * executed, no clock is read, no deadline is advanced, and no missed tick is
     * accumulated or replayed: the original has no catch-up loop, so neither does
     * this boundary.
     */
    ++timer->tick_entry_count;
    return 0;
}
