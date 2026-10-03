#include "debug_tracker.h"

#include <stdlib.h>
#include <string.h>

/*
 * STEP 13E -- terminal debug tracker. See debug_tracker.h for the contract.
 *
 * Every writer below is a small static helper that ends in one fflush(), so a
 * crash or an abrupt exit still leaves the most recent event on the stream. No
 * allocation happens on the emit path: the level test, the field reads and the
 * formatted write are all it costs, and nothing here sleeps, spins, or changes
 * pacing.
 *
 * TILE VOCABULARY. Only categories that PRODUCTION native code already acts on
 * are emitted, and each is emitted from the site that already performs the
 * effect:
 *
 *   CRYSTAL        7      vertical_slice.c:2368  (+200, checkpoint rewrite)
 *   CRYSTAL_BALL  29      vertical_slice.c:2513  (+1000, lives, tile rewrite)
 *   HOOP          13-16   vertical_slice.c:2287  app_collect_hoop(), +500
 *   WIDE_HOOP     21-24   vertical_slice.c:2287  app_collect_hoop(), +500
 *   DEFLATER      39-42   player.c:415           ballSize -> 12
 *   GRAVITY       47-50   vertical_slice.c:2710  powerUpGravity = 300
 *   BOOST         51-54   vertical_slice.c:2788  TODO_somePowerUp3 = 300
 *   DEATH_HAZARD  3-6,10  vertical_slice.c:2642  app_kill_ball()
 *   EXIT_DOOR     9       vertical_slice.c:2551  bounce_game_mark_level_complete()
 *
 * DEFLATER is the one entry with no [NBB-TILE] emit site, and that is deliberate.
 * Its only native producer is player.c:415, inside
 * player_constructor_apply_collision_state(), which is reached from the
 * constructor probes and not from the live frame loop; there is no gameplay site
 * to emit from and no BounceGame to read a tick from. The name is kept because
 * the effect is real, and the effect is already observable: whenever a deflation
 * happens, the per-frame edge detection reports the resulting
 * "[NBB-BALL] tick=N size=16 -> 12". Plumbing into player.c purely to print a name
 * the effect already reports would widen the change without adding information.
 *
 * There is deliberately NO entry for the pumper ids 43-46. Production native
 * code does not process them, so the tracker must not manufacture a PUMPER
 * event; adding processing to make one appear would be implementing a recorded
 * gap, which is out of scope here.
 */

/* --- low-level emit ------------------------------------------------------ */

/*
 * Single choke point for the enabled test. A disabled or sinkless tracker
 * returns here, so it costs one predictable branch per call site and nothing
 * else: no formatting, no allocation, no flush.
 */
static bool debug_want_sink(
    const BounceDebugTracker *tracker,
    BounceDebugLevel required
)
{
    if (tracker == NULL || tracker->sink == NULL)
        return false;
    if ((int)tracker->level < (int)required)
        return false;
    return true;
}

/* --- configuration ------------------------------------------------------- */

BounceDebugLevel bounce_debug_level_from_text(const char *text)
{
    if (text == NULL)
        return BOUNCE_DEBUG_LEVEL_DISABLED;
    /* Exactly one character, so "1x", "01" and " 1" are not silently accepted. */
    if (text[0] < '0' || text[0] > '3')
        return BOUNCE_DEBUG_LEVEL_DISABLED;
    if (text[1] != '\0')
        return BOUNCE_DEBUG_LEVEL_DISABLED;
    return (BounceDebugLevel)(text[0] - '0');
}

BounceDebugLevel bounce_debug_level_from_env(const char *name)
{
    if (name == NULL || name[0] == '\0')
        return BOUNCE_DEBUG_LEVEL_DISABLED;
    return bounce_debug_level_from_text(getenv(name));
}

const char *bounce_debug_flow_state_name(BounceAppState state)
{
    switch (state) {
    case BOUNCE_APP_STATE_MENU:
        return "MENU";
    case BOUNCE_APP_STATE_GAMEPLAY:
        return "GAMEPLAY";
    case BOUNCE_APP_STATE_PAUSE:
        return "PAUSE";
    case BOUNCE_APP_STATE_GAME_OVER:
        return "GAME_OVER";
    case BOUNCE_APP_STATE_LEVEL_COMPLETE:
        return "LEVEL_COMPLETE";
    case BOUNCE_APP_STATE_SPLASH:
        return "SPLASH";
    case BOUNCE_APP_STATE_ACTION_BOUNDARY:
        return "ACTION_BOUNDARY";
    case BOUNCE_APP_STATE_INSTRUCTIONS:
        return "INSTRUCTIONS";
    case BOUNCE_APP_STATE_HIGH_SCORE:
        return "HIGH_SCORE";
    case BOUNCE_APP_STATE_LEVEL_SELECTION:
        return "LEVEL_SELECTION";
    case BOUNCE_APP_STATE_GAMEPLAY_ENTRY:
        return "GAMEPLAY_ENTRY";
    case BOUNCE_APP_STATE_LEVEL_LOADED:
        return "LEVEL_LOADED";
    case BOUNCE_APP_STATE_SETTINGS:
        return "SETTINGS";
    case BOUNCE_APP_STATE_ABOUT:
        return "ABOUT";
    case BOUNCE_APP_STATE_GAME_END:
        return "GAME_END";
    default:
        break;
    }
    return "UNKNOWN";
}

static const char *debug_tile_name(BounceDebugTile tile)
{
    switch (tile) {
    case BOUNCE_DEBUG_TILE_CRYSTAL:
        return "CRYSTAL";
    case BOUNCE_DEBUG_TILE_CRYSTAL_BALL:
        return "CRYSTAL_BALL";
    case BOUNCE_DEBUG_TILE_HOOP:
        return "HOOP";
    case BOUNCE_DEBUG_TILE_WIDE_HOOP:
        return "WIDE_HOOP";
    case BOUNCE_DEBUG_TILE_DEFLATER:
        return "DEFLATER";
    case BOUNCE_DEBUG_TILE_BOOST:
        return "BOOST";
    case BOUNCE_DEBUG_TILE_GRAVITY:
        return "GRAVITY";
    case BOUNCE_DEBUG_TILE_DEATH_HAZARD:
        return "DEATH_HAZARD";
    case BOUNCE_DEBUG_TILE_EXIT_DOOR:
        return "EXIT_DOOR";
    default:
        break;
    }
    return "UNKNOWN";
}

/* --- lifecycle ----------------------------------------------------------- */

void bounce_debug_tracker_init(
    BounceDebugTracker *tracker,
    BounceDebugLevel level,
    FILE *sink
)
{
    if (tracker == NULL)
        return;
    memset(tracker, 0, sizeof *tracker);
    /* An out-of-range value is treated as disabled, never as "verbose". */
    if ((int)level < (int)BOUNCE_DEBUG_LEVEL_DISABLED
        || (int)level > (int)BOUNCE_DEBUG_LEVEL_VERBOSE) {
        level = BOUNCE_DEBUG_LEVEL_DISABLED;
    }
    tracker->level = level;
    tracker->sink = sink;
    tracker->observed_level_id = BOUNCE_GAME_LEVEL_ID_UNKNOWN;
}

void bounce_debug_tracker_set_sink(BounceDebugTracker *tracker, FILE *sink)
{
    if (tracker == NULL)
        return;
    tracker->sink = sink;
}

bool bounce_debug_tracker_enabled(const BounceDebugTracker *tracker)
{
    if (tracker == NULL)
        return false;
    if ((int)tracker->level <= (int)BOUNCE_DEBUG_LEVEL_DISABLED)
        return false;
    /* A tracker with no sink can emit nothing, so it counts as disabled. */
    return tracker->sink != NULL;
}

BounceDebugLevel bounce_debug_tracker_level(const BounceDebugTracker *tracker)
{
    return tracker == NULL ? BOUNCE_DEBUG_LEVEL_DISABLED : tracker->level;
}

uint64_t bounce_debug_tick(const BounceGame *game)
{
    if (game == NULL)
        return 0u;
    return game->game_timer.tick_entry_count;
}

bool bounce_debug_want(const BounceDebugTracker *tracker, BounceDebugLevel level)
{
    if (tracker == NULL)
        return false;
    return (int)tracker->level >= (int)level;
}

/* --- level 1: events ----------------------------------------------------- */

void bounce_debug_event_new_game(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] NEW_GAME tick=%llu level=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_level_start(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] LEVEL_START tick=%llu level=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_level_complete(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] LEVEL_COMPLETE tick=%llu level=%d hoops=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id,
        game->hoops_scored
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_game_over(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] GAME_OVER tick=%llu level=%d lives=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id,
        game->lives
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_game_end(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] GAME_END tick=%llu level=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_death(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] DEATH tick=%llu lives=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->lives
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_respawn(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-EVENT] RESPAWN tick=%llu level=%d lives=%d\n",
        (unsigned long long)bounce_debug_tick(game),
        game->level_id,
        game->lives
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_event_checkpoint(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t tile_x,
    int32_t tile_y,
    int32_t ball_size
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-CHECKPOINT] tick=%llu x=%ld y=%ld size=%ld\n",
        (unsigned long long)bounce_debug_tick(game),
        (long)tile_x,
        (long)tile_y,
        (long)ball_size
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_flow_change(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    BounceAppState from,
    BounceAppState to
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-FLOW] %s -> %s tick=%llu\n",
        bounce_debug_flow_state_name(from),
        bounce_debug_flow_state_name(to),
        (unsigned long long)bounce_debug_tick(game)
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_tile(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    BounceDebugTile tile,
    uint32_t tile_id,
    int32_t tile_x,
    int32_t tile_y
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-TILE] %s id=%lu at=%ld,%ld tick=%llu\n",
        debug_tile_name(tile),
        (unsigned long)tile_id,
        (long)tile_x,
        (long)tile_y,
        (unsigned long long)bounce_debug_tick(game)
    );
    (void)fflush(tracker->sink);
}

/* --- level 2: state transitions ------------------------------------------ */

void bounce_debug_death_tick(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t z,
    int32_t q
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_STATE) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-DEATH] tick=%llu z=%ld q=%ld\n",
        (unsigned long long)bounce_debug_tick(game),
        (long)z,
        (long)q
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_ball_size(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t from_size,
    int32_t to_size
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_STATE) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-BALL] tick=%llu size=%ld -> %ld\n",
        (unsigned long long)bounce_debug_tick(game),
        (long)from_size,
        (long)to_size
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_life(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int from_lives,
    int to_lives
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_STATE) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-LIFE] tick=%llu %d -> %d\n",
        (unsigned long long)bounce_debug_tick(game),
        from_lives,
        to_lives
    );
    (void)fflush(tracker->sink);
}

void bounce_debug_score(
    BounceDebugTracker *tracker,
    const BounceGame *game,
    int32_t from_score,
    int32_t to_score
)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_STATE) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-SCORE] tick=%llu %ld -> %ld\n",
        (unsigned long long)bounce_debug_tick(game),
        (long)from_score,
        (long)to_score
    );
    (void)fflush(tracker->sink);
}

/* --- level 3: verbose per-tick state ------------------------------------- */

void bounce_debug_player_state(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_VERBOSE) || game == NULL)
        return;
    fprintf(
        tracker->sink,
        "[NBB-PLAYER] tick=%llu flow=%s level=%d x=%ld y=%ld vx=%ld vy=%ld"
        " size=%ld p=%ld C=%ld z=%ld q=%ld lives=%d cp=%ld,%ld,%ld\n",
        (unsigned long long)bounce_debug_tick(game),
        bounce_debug_flow_state_name(game->flow.state),
        game->level_id,
        (long)game->player.TODO_unkX,
        (long)game->player.TODO_unkY,
        (long)game->player.horizontal_velocity,
        (long)game->player.o,
        (long)game->player.ballSize,
        (long)game->player.p,
        (long)game->player.C,
        (long)game->player.z,
        (long)game->player.q,
        game->lives,
        (long)game->checkpoint_tile_x,
        (long)game->checkpoint_tile_y,
        (long)game->checkpoint_ball_size
    );
    (void)fflush(tracker->sink);
}

/* --- edge detection ------------------------------------------------------ */

void bounce_debug_tracker_reseed(BounceDebugTracker *tracker, const BounceGame *game)
{
    if (tracker == NULL || game == NULL)
        return;
    tracker->seeded = true;
    tracker->observed_flow_state = game->flow.state;
    tracker->observed_level_id = game->level_id;
    tracker->observed_score = game->score;
    tracker->observed_lives = game->lives;
    tracker->observed_ball_size = game->player.ballSize;
    tracker->observed_checkpoint_x = game->checkpoint_tile_x;
    tracker->observed_checkpoint_y = game->checkpoint_tile_y;
    tracker->observed_checkpoint_ball_size = game->checkpoint_ball_size;
}

int bounce_debug_tracker_observe(BounceDebugTracker *tracker, const BounceGame *game)
{
    uint64_t tick;

    if (tracker == NULL || game == NULL)
        return -1;
    if (!debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_EVENTS))
        return 0;

    if (!tracker->seeded) {
        /* First observation establishes the baseline; it is not a change. */
        bounce_debug_tracker_reseed(tracker, game);
        bounce_debug_player_state(tracker, game);
        tracker->reported_state_tick = bounce_debug_tick(game);
        tracker->reported_state_tick_valid = true;
        return 0;
    }

    if (game->flow.state != tracker->observed_flow_state) {
        BounceAppState from = tracker->observed_flow_state;
        BounceAppState to = game->flow.state;

        tracker->observed_flow_state = to;
        if (to == BOUNCE_APP_STATE_GAMEPLAY && !tracker->seen_gameplay) {
            tracker->seen_gameplay = true;
            bounce_debug_event_new_game(tracker, game);
        }
        switch (to) {
        case BOUNCE_APP_STATE_GAMEPLAY:
            bounce_debug_event_level_start(tracker, game);
            break;
        case BOUNCE_APP_STATE_LEVEL_COMPLETE:
            bounce_debug_event_level_complete(tracker, game);
            break;
        case BOUNCE_APP_STATE_GAME_OVER:
            bounce_debug_event_game_over(tracker, game);
            break;
        case BOUNCE_APP_STATE_GAME_END:
            bounce_debug_event_game_end(tracker, game);
            break;
        default:
            break;
        }
        bounce_debug_flow_change(tracker, game, from, to);
    }

    if (game->level_id != tracker->observed_level_id) {
        tracker->observed_level_id = game->level_id;
    }

    if (game->score != tracker->observed_score) {
        int32_t from = tracker->observed_score;

        tracker->observed_score = game->score;
        bounce_debug_score(tracker, game, from, game->score);
    }

    if (game->lives != tracker->observed_lives) {
        int from = tracker->observed_lives;

        tracker->observed_lives = game->lives;
        bounce_debug_life(tracker, game, from, game->lives);
    }

    if (game->player.ballSize != tracker->observed_ball_size) {
        int32_t from = tracker->observed_ball_size;

        tracker->observed_ball_size = game->player.ballSize;
        bounce_debug_ball_size(tracker, game, from, game->player.ballSize);
    }

    if (game->checkpoint_tile_x != tracker->observed_checkpoint_x
        || game->checkpoint_tile_y != tracker->observed_checkpoint_y
        || game->checkpoint_ball_size != tracker->observed_checkpoint_ball_size) {
        tracker->observed_checkpoint_x = game->checkpoint_tile_x;
        tracker->observed_checkpoint_y = game->checkpoint_tile_y;
        tracker->observed_checkpoint_ball_size = game->checkpoint_ball_size;
        bounce_debug_event_checkpoint(
            tracker,
            game,
            game->checkpoint_tile_x,
            game->checkpoint_tile_y,
            game->checkpoint_ball_size
        );
    }

    tick = bounce_debug_tick(game);
    if (debug_want_sink(tracker, BOUNCE_DEBUG_LEVEL_VERBOSE)
        && (!tracker->reported_state_tick_valid || tick != tracker->reported_state_tick)) {
        tracker->reported_state_tick = tick;
        tracker->reported_state_tick_valid = true;
        bounce_debug_player_state(tracker, game);
    }
    return 0;
}
