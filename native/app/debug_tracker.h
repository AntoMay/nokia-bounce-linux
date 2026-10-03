#ifndef BOUNCE_NATIVE_APP_DEBUG_TRACKER_H
#define BOUNCE_NATIVE_APP_DEBUG_TRACKER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "game.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STEP 13E -- terminal debug tracker.
 *
 * NATIVE TOOLING ONLY. This module observes existing native state and writes it to a
 * FILE stream (stderr in production). It is not a gameplay feature and it is not a
 * reverse-engineering claim: nothing here is attributed to the recovered Nokia Bounce
 * source, and nothing here closes or alters any recorded GAP.
 *
 * WHAT IT IS NOT
 *   - It is not a second simulation clock. Every printed tick is the EXISTING
 *     native tick counter BounceGameTimer.tick_entry_count, incremented once per
 *     native gameplay simulation tick by bounce_game_timer_callback() at
 *     game_timer.c:165, which is reached from the live fixed-step path
 *     app_run_canonical_gameplay_tick() -> app_timer_callback_dispatch() ->
 *     bounce_game_timer_callback() (vertical_slice.c:5455, :4462). No counter is
 *     added here and the debug tick never drives gameplay.
 *   - It is not a second source of truth. The `observed_*` members below are a
 *     read-only observation cache used only to detect edges. No gameplay code
 *     reads them, they are never written by gameplay code, and
 *     bounce_debug_tracker_init() seeds them from the live state so the first
 *     observation is not reported as a change.
 *   - It never calls a gameplay, level-loading, respawn, collision, physics,
 *     rendering or UI function. It only reads fields and prints.
 *
 * LEVELS (the NBB_DEBUG environment variable)
 *   0  disabled            -- unset, "0", or any unrecognised value
 *   1  important events    -- level / checkpoint / death / respawn / flow / tiles
 *   2  + state transitions -- score, lives, ball size, and the per-tick death z/q ladder
 *   3  + verbose state     -- one player state line per distinct simulation tick
 *
 * Level 3 output is a per-tick text stream. At the native 40 ms tick period it
 * produces roughly 25 lines per second; on a terminal that is readable but it is
 * materially more expensive than levels 0-2. It is therefore opt-in only.
 */

#define BOUNCE_DEBUG_ENV_NAME "NBB_DEBUG"

typedef enum BounceDebugLevel {
    BOUNCE_DEBUG_LEVEL_DISABLED = 0,
    BOUNCE_DEBUG_LEVEL_EVENTS = 1,
    BOUNCE_DEBUG_LEVEL_STATE = 2,
    BOUNCE_DEBUG_LEVEL_VERBOSE = 3
} BounceDebugLevel;

/*
 * One record per notable tile category. Only tiles that PRODUCTION native code
 * already processes are named here; there is no entry for a tile the production
 * path never reaches, so the tracker can never report an event that the game did
 * not actually apply. See the tile vocabulary note in debug_tracker.c.
 */
typedef enum BounceDebugTile {
    BOUNCE_DEBUG_TILE_CRYSTAL = 0,
    BOUNCE_DEBUG_TILE_CRYSTAL_BALL,
    BOUNCE_DEBUG_TILE_HOOP,
    BOUNCE_DEBUG_TILE_WIDE_HOOP,
    BOUNCE_DEBUG_TILE_DEFLATER,
    BOUNCE_DEBUG_TILE_BOOST,
    BOUNCE_DEBUG_TILE_GRAVITY,
    BOUNCE_DEBUG_TILE_DEATH_HAZARD,
    BOUNCE_DEBUG_TILE_EXIT_DOOR
} BounceDebugTile;

struct BounceDebugTracker {
    BounceDebugLevel level;
    /* Destination stream. NULL disables output regardless of level. */
    FILE *sink;
    /* True once the observation cache below has been seeded. */
    bool seeded;
    /* True after the first GAMEPLAY entry, so NEW_GAME is emitted exactly once. */
    bool seen_gameplay;
    /* Observation cache: read-only edge detection, never read by gameplay. */
    BounceAppState observed_flow_state;
    int observed_level_id;
    int32_t observed_score;
    int observed_lives;
    int32_t observed_ball_size;
    int32_t observed_checkpoint_x;
    int32_t observed_checkpoint_y;
    int32_t observed_checkpoint_ball_size;
    /* Last simulation tick a level-3 state line was written for. */
    uint64_t reported_state_tick;
    bool reported_state_tick_valid;
};

/* --- configuration ------------------------------------------------------- */

/*
 * Parse one NBB_DEBUG value. Pure: no environment access, no output, no state.
 * Accepts exactly "0", "1", "2" and "3"; everything else, including NULL, yields
 * BOUNCE_DEBUG_LEVEL_DISABLED.
 */
BounceDebugLevel bounce_debug_level_from_text(const char *text);

/* Read and parse the named environment variable. Parsed once, at init. */
BounceDebugLevel bounce_debug_level_from_env(const char *name);

/* --- lifecycle ----------------------------------------------------------- */

void bounce_debug_tracker_init(
    BounceDebugTracker *tracker,
    BounceDebugLevel level,
    FILE *sink
);

/* Redirect output. NULL silences the tracker without changing its level. */
void bounce_debug_tracker_set_sink(BounceDebugTracker *tracker, FILE *sink);

/*
 * True when the tracker can actually emit: a non-zero level AND a non-NULL sink.
 * main() attaches the tracker to the app on exactly this condition, so an
 * unattached app has a NULL debug_tracker and every call site returns early.
 */
bool bounce_debug_tracker_enabled(const BounceDebugTracker *tracker);

/* The configured level, regardless of sink. */
BounceDebugLevel bounce_debug_tracker_level(const BounceDebugTracker *tracker);

/*
 * The EXISTING native simulation tick. This is a read of
 * BounceGameTimer.tick_entry_count, not a counter owned by this module.
 */
uint64_t bounce_debug_tick(const BounceGame *game);

/* True when `level` is at or above the tracker's configured level. */
bool bounce_debug_want(const BounceDebugTracker *tracker, BounceDebugLevel level);

/* Canonical BounceAppState name, or "UNKNOWN" for a value with no name. */
const char *bounce_debug_flow_state_name(BounceAppState state);

/* --- level 1: events ----------------------------------------------------- */

void bounce_debug_event_new_game(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_level_start(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_level_complete(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_game_over(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_game_end(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_death(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_respawn(BounceDebugTracker *tracker, const BounceGame *game);
void bounce_debug_event_checkpoint(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t tile_x,
    int32_t tile_y,
    int32_t ball_size
);
void bounce_debug_flow_change(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    BounceAppState from,
    BounceAppState to
);
void bounce_debug_tile(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    BounceDebugTile tile,
    uint32_t tile_id,
    int32_t tile_x,
    int32_t tile_y
);

/* --- level 2: state transitions ------------------------------------------ */

void bounce_debug_death_tick(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t z,
    int32_t q
);
void bounce_debug_ball_size(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t from_size,
    int32_t to_size
);
void bounce_debug_life(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int from_lives,
    int to_lives
);
void bounce_debug_score(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t from_score,
    int32_t to_score
);

/* --- level 3: verbose per-tick state ------------------------------------- */

void bounce_debug_player_state(BounceDebugTracker *tracker, const BounceGame *game);

/* --- edge detection ------------------------------------------------------ */

/*
 * Observe the live state once and report any edge it finds: flow state, level id,
 * score, lives, ball size and checkpoint. Called once per frame from the live
 * loop; it reads fields only and never calls a gameplay function.
 *
 * At level 3 it also writes at most one player-state line per distinct
 * simulation tick, so the line count follows the tick rate and not the frame
 * rate.
 *
 * Returns 0 on success, -1 for a NULL tracker or game.
 */
int bounce_debug_tracker_observe(BounceDebugTracker *tracker, const BounceGame *game);

/*
 * Re-seed the observation cache from the live state without reporting anything.
 * Used after a level load so a reset value is not reported as a change.
 */
void bounce_debug_tracker_reseed(BounceDebugTracker *tracker, const BounceGame *game);

#ifdef __cplusplus
}
#endif

#endif
