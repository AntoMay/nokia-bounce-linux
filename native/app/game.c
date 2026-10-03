#include "game.h"

#include "level_loader.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *bounce_game_copy_path(const char *path)
{
    size_t length;
    char *copy;

    if (path == NULL || path[0] == '\0')
        return NULL;

    length = strlen(path);
    if (length == SIZE_MAX)
        return NULL;

    copy = (char *)malloc(length + 1u);
    if (copy == NULL)
        return NULL;

    memcpy(copy, path, length + 1u);
    return copy;
}

int bounce_game_level_id_from_path(const char *level_path, int fallback)
{
    const char *name;
    const char *digits;
    size_t name_length;
    int value = 0;
    unsigned int index;

    if (level_path == NULL)
        return fallback;

    name = strrchr(level_path, '/');
    name = name == NULL ? level_path : name + 1;
    name_length = strlen(name);
    if (name_length != 11u || strncmp(name, "J2MElvl.", 8u) != 0)
        return fallback;
    digits = name + 8;
    for (index = 0u; index < 3u; ++index) {
        if (digits[index] < '0' || digits[index] > '9')
            return fallback;
        value = value * 10 + (int)(digits[index] - '0');
    }
    if (digits[3] != '\0' || value < BOUNCE_APP_FIRST_LEVEL_ID
        || value > BOUNCE_APP_LAST_LEVEL_ID)
        return fallback;
    return value;
}

const char *bounce_game_state_name(BounceGameState state)
{
    switch (state) {
        case BOUNCE_GAME_STATE_SPLASH:
            return "SPLASH";
        case BOUNCE_GAME_STATE_MENU:
            return "MENU";
        case BOUNCE_GAME_STATE_PLAYING:
            return "PLAYING";
        case BOUNCE_GAME_STATE_LEVEL_COMPLETE:
            return "LEVEL_COMPLETE";
        case BOUNCE_GAME_STATE_DEAD:
            return "DEAD";
        case BOUNCE_GAME_STATE_RESPAWN:
            return "RESPAWN";
        case BOUNCE_GAME_STATE_GAME_OVER:
            return "GAME_OVER";
        case BOUNCE_GAME_STATE_PAUSED:
            return "PAUSED";
        case BOUNCE_GAME_STATE_ACTION_BOUNDARY:
            return "ACTION_BOUNDARY";
        case BOUNCE_GAME_STATE_INSTRUCTIONS:
            return "INSTRUCTIONS";
        case BOUNCE_GAME_STATE_HIGH_SCORE:
            return "HIGH_SCORE";
        case BOUNCE_GAME_STATE_LEVEL_SELECTION:
            return "LEVEL_SELECTION";
        case BOUNCE_GAME_STATE_GAMEPLAY_ENTRY:
            return "GAMEPLAY_ENTRY";
        case BOUNCE_GAME_STATE_LEVEL_LOADED:
            return "LEVEL_LOADED";
        /* STEP 12X-P4-B: not "GAME_OVER", which is the death result. */
        case BOUNCE_GAME_STATE_GAME_END:
            return "GAME_END";
        /* NATIVE EXTENSION mirrors. */
        case BOUNCE_GAME_STATE_SETTINGS:
            return "SETTINGS";
        case BOUNCE_GAME_STATE_ABOUT:
            return "ABOUT";
        case BOUNCE_GAME_STATE_UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

/*
 * STEP 12X-P4-C-D-B: Java `int` addition, defined for every input.
 *
 * A Java `int` is a signed 32-bit two's-complement value whose addition wraps
 * silently, with no saturation and no exception. C signed integer overflow is
 * undefined behaviour, so `score += amount` written directly would not have a
 * defined result for extreme inputs and would be a portability hazard. The
 * addition is therefore performed in the unsigned domain and copied back, which
 * is exactly two's-complement wrap-around by construction.
 *
 * This is the same helper, and the same technique, already used by this project
 * at collision_query.c:68 and dyn_thorns.c:16; it is repeated here as a
 * file-static rather than promoted to a shared header, matching that existing
 * convention rather than introducing a new one.
 */
static int32_t java_int32_add(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left + (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

int bounce_game_init(BounceGame *game)
{
    if (game == NULL)
        return -1;

    memset(game, 0, sizeof *game);
    bounce_app_flow_init(&game->flow);
    bounce_input_reset(&game->input);
    game->state = BOUNCE_GAME_STATE_SPLASH;
    game->level_id = BOUNCE_GAME_LEVEL_ID_UNKNOWN;
    game->lives = 0;
    game->hoops_scored = 0;
    /* STEP 12X-P4-C-D-B: the memset above already zeroes this; the explicit
     * assignment states the intent beside the other session counters and keeps
     * the default correct if the memset is ever removed. */
    game->score = 0;
    /* STEP 45A: e.java:55 `public boolean GodMode = false;`. The memset above
     * already zeroes this; the explicit assignment states the intent beside the
     * other session flags and keeps the default correct if the memset is ever
     * removed. It must stay false: false is GodMode OFF, i.e. the pre-45A
     * behaviour in which f.KillBall()'s body always runs. */
    game->god_mode = false;
    game->l = 0;
    game->k = 0;
    game->v = 0;
    return 0;
}

int bounce_game_sync_ui_state(BounceGame *game)
{
    if (game == NULL)
        return -1;

    switch (game->flow.state) {
        case BOUNCE_APP_STATE_SPLASH:
            game->state = BOUNCE_GAME_STATE_SPLASH;
            return 0;
        case BOUNCE_APP_STATE_MENU:
            game->state = BOUNCE_GAME_STATE_MENU;
            return 0;
        case BOUNCE_APP_STATE_ACTION_BOUNDARY:
            game->state = BOUNCE_GAME_STATE_ACTION_BOUNDARY;
            return 0;
        case BOUNCE_APP_STATE_INSTRUCTIONS:
            game->state = BOUNCE_GAME_STATE_INSTRUCTIONS;
            return 0;
        case BOUNCE_APP_STATE_HIGH_SCORE:
            game->state = BOUNCE_GAME_STATE_HIGH_SCORE;
            return 0;
        case BOUNCE_APP_STATE_LEVEL_SELECTION:
            /*
             * STEP 12X-P4-C-D-B: the New Game score reset is deliberately NOT
             * placed here, and this comment records why, because the obvious
             * placement looks correct and is not.
             *
             * BOUNCE_APP_STATE_LEVEL_SELECTION has exactly one writer in the
             * whole tree, app_flow.c:339, inside the
             * `action == BOUNCE_MENU_ACTION_NEW_GAME` branch of
             * bounce_app_flow_menu_select(). Transitioning into it is therefore a
             * genuine "New Game was chosen" signal, and it was the first
             * candidate implementation.
             *
             * IT IS INSUFFICIENT, and shipping it would be silently wrong.
             * bounce_app_flow_menu_select() only routes to Level Selection when
             * available_level_count > 1 (app_flow.c:338). On every other New Game
             * -- which includes EVERY fresh install, because a virgin store has
             * available_level_count == 0 -- it takes the else branch at
             * app_flow.c:341-350, calls
             * bounce_app_flow_record_start_level(FIRST_LEVEL_ID), goes straight
             * to ACTION_BOUNDARY, and OVERWRITES menu.pending_action from
             * BOUNCE_MENU_ACTION_NEW_GAME to
             * BOUNCE_MENU_ACTION_START_LEVEL. The New Game decision is
             * therefore not observable from the game layer in that case.
             *
             * The consequence is not theoretical. With hoops integrated later,
             * a player can score on Level 1, die, return to the menu and choose
             * New Game while available_level_count is still 0; a reset keyed on
             * this transition would not fire and the new game would inherit the
             * old total. A reset that is correct only for players who have
             * already finished a level is worse than no reset at all, because it
             * hides the defect.
             *
             * The architectural gap is therefore recorded rather than papered
             * over: the New Game decision is made and consumed entirely inside
             * the flow layer, and no existing game-layer signal distinguishes
             * "new session" from "continue" at the point where a level start is
             * committed. bounce_game_continue_level() is the game-layer Continue
             * entry, but bounce_app_flow_continue_level() shares
             * bounce_app_flow_record_start_level() with the New Game route, so
             * bounce_game_activate_staged_level() -- where lives is committed --
             * cannot tell them apart either, and the deferred_init latch has
             * already been cleared by bounce_game_load_level_entry() earlier in
             * the same chain.
             *
             * Closing this needs exactly one of: a flow-to-game New Game signal,
             * or a session-started flag on BounceGame. Both are lifecycle
             * changes, so both are out of scope for this milestone and are
             * deferred to a named follow-up. Until then the score is preserved
             * across New Game, which is the same value Java would keep for a
             * Continue and the only wrong-by-omission case; nothing here can
             * produce a fabricated or partially reset total.
             */
            game->state = BOUNCE_GAME_STATE_LEVEL_SELECTION;
            return 0;
        case BOUNCE_APP_STATE_GAMEPLAY_ENTRY:
            /* No level, player, runtime, or tick is created by this mirror. */
            game->state = BOUNCE_GAME_STATE_GAMEPLAY_ENTRY;
            return 0;
        case BOUNCE_APP_STATE_LEVEL_LOADED:
            /* Level ownership lives in BounceGame; the mirror only names it. */
            game->state = BOUNCE_GAME_STATE_LEVEL_LOADED;
            return 0;
        /* STEP 12X-P4-B: the game-complete terminal destination. */
        case BOUNCE_APP_STATE_GAME_END:
            game->state = BOUNCE_GAME_STATE_GAME_END;
            return 0;
        case BOUNCE_APP_STATE_SETTINGS:
            /* NATIVE EXTENSION destination; the mirror only names it. */
            game->state = BOUNCE_GAME_STATE_SETTINGS;
            return 0;
        case BOUNCE_APP_STATE_ABOUT:
            /* NATIVE EXTENSION destination; the mirror only names it. */
            game->state = BOUNCE_GAME_STATE_ABOUT;
            return 0;
        case BOUNCE_APP_STATE_GAMEPLAY:
        case BOUNCE_APP_STATE_PAUSE:
        case BOUNCE_APP_STATE_GAME_OVER:
        case BOUNCE_APP_STATE_LEVEL_COMPLETE:
            return 0;
    }
    return -1;
}

int bounce_game_enter_menu(BounceGame *game)
{
    if (game == NULL)
        return -1;

    /* Keep the application-flow owner and gameplay mirror synchronized. */
    if (game->flow.state == BOUNCE_APP_STATE_SPLASH) {
        if (bounce_app_flow_apply(
                &game->flow,
                BOUNCE_APP_EVENT_SPLASH_DONE
            ) != 0)
            return -1;
    } else if (game->flow.state != BOUNCE_APP_STATE_MENU) {
        if (bounce_app_flow_apply(
                &game->flow,
                BOUNCE_APP_EVENT_MENU
            ) != 0)
            return -1;
    }
    bounce_input_reset(&game->input);
    game->state = BOUNCE_GAME_STATE_MENU;
    return 0;
}

void bounce_game_release_level(BounceGame *game)
{
    if (game == NULL)
        return;

    bounce_runtime_level_tiles_destroy(game->runtime);
    bounce_level_destroy(game->level);
    free(game->level_path);

    game->runtime = NULL;
    game->level = NULL;
    game->level_path = NULL;
    game->level_id = BOUNCE_GAME_LEVEL_ID_UNKNOWN;
    bounce_player_reset(&game->player);
    memset(&game->spawn, 0, sizeof game->spawn);
    game->lives = 0;
    game->hoops_scored = 0;
    game->level_complete = false;
    game->dead = false;
    game->respawn_requested = false;
    game->game_over = false;
    game->l = 0;
    game->k = 0;
    game->v = 0;
    game->next_player_tick_ms = 0;
}

int bounce_game_load_level(
    BounceGame *game,
    const char *level_path,
    int level_id
)
{
    BounceLevel *new_level = NULL;
    BounceRuntimeLevelTiles *new_runtime = NULL;
    BouncePlayer new_player;
    BouncePlayerSpawn new_spawn;
    char *new_path = NULL;
    BounceLevelLoadStatus status;
    int resolved_level_id;

    if (game == NULL || level_path == NULL || level_path[0] == '\0'
        || level_id < BOUNCE_GAME_LEVEL_ID_UNKNOWN)
        return -1;
    resolved_level_id = bounce_game_level_id_from_path(
        level_path,
        level_id
    );

    new_path = bounce_game_copy_path(level_path);
    if (new_path == NULL)
        return -1;

    status = bounce_level_load_file(level_path, &new_level);
    if (status != BOUNCE_LEVEL_LOAD_OK || new_level == NULL)
        goto failure;

    new_runtime = bounce_runtime_level_tiles_create(new_level);
    if (new_runtime == NULL
        || bounce_runtime_level_tiles_width(new_runtime)
            != bounce_level_width(new_level)
        || bounce_runtime_level_tiles_height(new_runtime)
            != bounce_level_height(new_level))
        goto failure;

    if (bounce_player_initialize_from_level(
            new_level,
            &new_player,
            &new_spawn
        ) != 0)
        goto failure;

    /* Keep the old context untouched until every new component is ready. */
    bounce_game_release_level(game);
    game->level = new_level;
    game->runtime = new_runtime;
    game->level_path = new_path;
    game->level_id = resolved_level_id;
    if (resolved_level_id != BOUNCE_GAME_LEVEL_ID_UNKNOWN)
        game->flow.level_id = resolved_level_id;
    /*
     * D-13 -- e.java:119 `this.p = 120;`, which is the only write of `p` in
     * the whole Java tree.
     *
     * InitializeGame() sets it right after LoadLevelId and right before it
     * creates the player, so it belongs to the level load rather than to the
     * gameplay-entry metadata step. Arming it here therefore covers BOTH level
     * starts that go through this function -- the af level skip, whose reload
     * lands in the RESET_GATE arm of app_timer_callback_dispatch, and the
     * legacy bounce_game_start_level() diagnostic route -- and neither of them
     * can be reached without re-showing the level banner for 120 ticks.
     *
     * It is NOT reset by a respawn: f.java:274-280 and e.java:277-280 call
     * a(x, y, ballSize, 0, 0), not InitializeGame(), so a death correctly
     * leaves `p` alone. bounce_app_flow_begin_gameplay_entry() records the same
     * constant for the staged entry routes, so this write is idempotent with it
     * rather than a second source of truth.
     *
     * Cannot fail: `flow` is a member of `game`, which the guard at the top of
     * this function already proved non-NULL.
     */
    (void)bounce_app_flow_arm_entry_countdown(&game->flow);
    game->player = new_player;
    game->spawn = new_spawn;
    game->lives = 0;
    game->hoops_scored = 0;
    game->level_complete = false;
    game->dead = false;
    game->respawn_requested = false;
    game->game_over = false;
    /* e.java:87 clears TODO_ExitUnlocked in the level-init path, immediately
     * before InitializeGame(). bounce_game_load_level() is the native
     * counterpart of that per-level reset, so the flag is cleared here beside
     * hoops_scored, which e.java:83 and e.java:118 clear in the same phase.
     * The completion clear at e.java:315 belongs to the not-yet-implemented
     * completion pipeline and is deliberately not reproduced. */
    game->TODO_ExitUnlocked = false;
    /* STEP 12F — door animation state initialization. */
    game->door_image_offset = 0;
    game->door_open_flag = false;
    game->l = 0;
    game->k = 0;
    game->v = 0;
    return 0;

failure:
    bounce_runtime_level_tiles_destroy(new_runtime);
    bounce_level_destroy(new_level);
    free(new_path);
    return -1;
}

int bounce_game_activate_loaded_level(BounceGame *game, int lives)
{
    if (game == NULL
        || game->state != BOUNCE_GAME_STATE_MENU
        || game->flow.state != BOUNCE_APP_STATE_MENU
        || game->level == NULL
        || game->runtime == NULL
        || !game->player.initialized
        || lives < 0)
        return -1;

    if (bounce_app_flow_apply(&game->flow, BOUNCE_APP_EVENT_START) != 0)
        return -1;

    game->lives = lives;
    game->hoops_scored = 0;
    game->level_complete = false;
    game->dead = false;
    game->respawn_requested = false;
    game->game_over = false;
    game->state = BOUNCE_GAME_STATE_PLAYING;
    return 0;
}

int bounce_game_start_level(
    BounceGame *game,
    const char *level_path,
    int level_id,
    int lives
)
{
    if (game == NULL
        || level_path == NULL
        || level_id < BOUNCE_GAME_LEVEL_ID_UNKNOWN
        || lives < 0
        || game->state != BOUNCE_GAME_STATE_MENU
        || game->flow.state != BOUNCE_APP_STATE_MENU)
        return -1;

    if (bounce_game_load_level(game, level_path, level_id) != 0)
        return -1;

    if (bounce_game_activate_loaded_level(game, lives) != 0) {
        bounce_game_release_level(game);
        return -1;
    }
    return 0;
}

int bounce_game_level_resource_path(
    int level_id,
    char *buffer,
    size_t buffer_size
)
{
    int written;

    if (buffer == NULL
        || level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;
    written = snprintf(
        buffer,
        buffer_size,
        "src/main/resources/levels/J2MElvl.%03d",
        level_id
    );
    if (written < 0 || (size_t)written >= buffer_size)
        return -1;
    return 0;
}

/*
 * Release only the level this context owns. Runtime tiles, player, spawn,
 * counters, camera, and the tick deadline are intentionally untouched.
 */
static void bounce_game_release_level_only(BounceGame *game)
{
    bounce_level_destroy(game->level);
    free(game->level_path);
    game->level = NULL;
    game->level_path = NULL;
}

int bounce_game_load_level_entry(BounceGame *game, int level_id)
{
    char level_path[64];
    char *new_path;
    BounceLevel *new_level = NULL;
    BounceLevelLoadStatus status;
    int resolved_level_id;

    if (game == NULL)
        return -1;
    /*
     * A runtime tile copy means a gameplay-shaped level is already installed;
     * replacing only the level would split ownership, so refuse instead.
     */
    if (game->runtime != NULL)
        return -1;
    if (bounce_game_level_resource_path(level_id, level_path, sizeof level_path)
        != 0)
        return -1;
    resolved_level_id = bounce_game_level_id_from_path(level_path, level_id);
    if (resolved_level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || resolved_level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;

    new_path = bounce_game_copy_path(level_path);
    if (new_path == NULL)
        return -1;

    status = bounce_level_load_file(level_path, &new_level);
    if (status != BOUNCE_LEVEL_LOAD_OK || new_level == NULL) {
        bounce_level_destroy(new_level);
        free(new_path);
        return -1;
    }

    /* Only level ownership changes; nothing else in the context is written. */
    bounce_game_release_level_only(game);
    game->level = new_level;
    game->level_path = new_path;
    game->level_id = resolved_level_id;
    game->flow.level_id = resolved_level_id;
    /*
     * STEP 12X-G-P3 -- the b.java:213 latch clear.
     *
     * b.LoadLevelId() opens with `this.d = false` (b.java:213), which is what
     * makes the e.java:222 consumption one-shot: the latch is cleared by the
     * load that InitializeGame performs, not by the tick that requested it, so a
     * second tick cannot re-enter InitializeGame.
     *
     * This function is the production counterpart of LoadLevelId, and it is the
     * one place both consumers reach: the staged entry
     * (app_advance_gameplay_entry_to_level_loaded) and the deferred-init branch
     * of bounce_game_tick_dispatch(). Putting the clear here therefore mirrors
     * the single Java site rather than duplicating it per caller.
     *
     * PLACED AFTER THE OWNERSHIP SWAP, NOT AT THE HEAD. Java clears first and
     * then reads the resource stream, so a Java load that threw IOException
     * consumed the latch and left the previous level in place. This loader is
     * atomic on failure -- see the header note at game.h:35-39 -- and it has two
     * rejections that must not disarm the latch: the `game->runtime != NULL`
     * ownership guard at :400, which is exactly the state a completed run is in,
     * and a missing or malformed resource. Clearing only once a level is really
     * installed keeps a deferred init retryable instead of silently dropping it,
     * and it cannot change any existing behaviour: a run that reaches this
     * function with the latch already false is unaffected.
     */
    game->deferred_init = false;
    return 0;
}

int bounce_game_initialize_player_entry(BounceGame *game)
{
    BouncePlayer new_player;
    BouncePlayerSpawn new_spawn;

    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_LEVEL_LOADED
        || game->level == NULL
        /* This step must not build or depend on runtime tile copies. */
        || game->runtime != NULL
        /* Repeated initialization is rejected; the player is preserved. */
        || game->player.initialized)
        return -1;

    /*
     * The existing seam reads only the parsed level: header conversion,
     * constructor defaults, the aq.l/aq.o literal 0/0 assignment, and the
     * tile-coordinate assignment. It performs no movement, camera, or timer
     * work.
     */
    if (bounce_player_initialize_from_level(
            game->level,
            &new_player,
            &new_spawn
        ) != 0)
        return -1;

    /*
     * f.java:127-131 selects the ball constructor from the same `ballSize`
     * argument: `if (ballSize == 12) UseRegularBall(); else UseBigBall();`.
     * `isBigBall` is the header field that argument is derived from
     * (level.c:221-223), so this is that exact branch. UseBigBall() is a
     * constructor step, so it belongs here rather than in a later gameplay
     * stage. The original runs it against the LevelTiles bytes LoadLevelId has
     * just written (e.java:121 -> e.java:127), which is exactly the pristine
     * BounceLevel owned here: no runtime copy exists at this boundary and none
     * is built.
     *
     * The probe reads only TODO_unkX/TODO_unkY, which the composition above did
     * not move, so running it after that composition yields the same constructor
     * inputs. The record order remains probe -> aq.l/aq.o -> camera, because
     * camera initialization is not performed here.
     */
    if (bounce_level_is_big_ball(game->level) != 0u
        && bounce_player_use_big_ball_level(&new_player, game->level) != 0)
        return -1;

    /*
     * STEP 12X-P4-C-D-C-B4-C -- establish the initial checkpoint here.
     *
     * This is the native counterpart of Java's InitializeGame() (e.java:115-124), whose
     * last two statements are:
     *
     *     e.java:121   a(TODO_ballInitialX * 12 + 6, TODO_ballInitialY * 12 + 6, BallSize, 0, 0);
     *     e.java:122   this.aq.a(TODO_ballInitialX, TODO_ballInitialY);
     *
     * and f.java:134-138, the setter, copies the tile coordinates and a snapshot of the
     * current ball size. So the level's spawn cell IS the first checkpoint, and the source
     * has no separate "no checkpoint yet" state because of it. This is the same boundary,
     * and the same reason.
     *
     * The three values are already computed: bounce_player_initialize_from_level() calls
     * bounce_level_initialize_new_game_player_data(), which performs the f.a(tileX, tileY)
     * assignment, and player.c:155-157 stores the results into new_spawn.tile_x,
     * new_spawn.tile_y and new_spawn.ball_size. Copying them here adds no new derivation and
     * calls no new function.
     *
     * `new_spawn` is READ here and never written past this point. It remains the level's
     * immutable initial spawn, which is what the existing spawn-immutability assertions
     * require. A level transition, a New Game and a Continue all reach this function, so
     * all three reset the checkpoint -- which is exactly Java's behaviour, because its
     * Continue is always followed by the next level's InitializeGame(). Death and the
     * respawn that consumes the checkpoint do NOT come through here and leave it intact.
     */
    game->checkpoint_tile_x = new_spawn.tile_x;
    game->checkpoint_tile_y = new_spawn.tile_y;
    game->checkpoint_ball_size = new_spawn.ball_size;
    /* Only the embedded player and spawn records change. */
    game->player = new_player;
    game->spawn = new_spawn;
    return 0;
}

int bounce_game_initialize_runtime_entry(BounceGame *game)
{
    BounceRuntimeLevelTiles *new_runtime;

    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_LEVEL_LOADED
        || game->level == NULL
        /* The player constructor must already have read the pristine bytes. */
        || !game->player.initialized
        /* Repeated initialization is rejected; the existing copy is preserved. */
        || game->runtime != NULL
        /* This step is not the gameplay transition. */
        || game->state != BOUNCE_GAME_STATE_LEVEL_LOADED)
        return -1;

    if (bounce_app_flow_entry_phase(&game->flow)
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED
        || bounce_app_flow_entry_runtime_tiles_created(&game->flow))
        return -1;

    /*
     * The existing verified constructor already performs exactly the two
     * source-derived initializations this seam needs, and nothing else:
     *   - copies every structural BounceLevel tile byte into a mutable
     *     uint16_t cell (b.java:243-247, which reads the same array it later
     *     mutates in place), and
     *   - decodes the 8-byte dynamic-thorn records into independent mutable
     *     w/ae state (b.java:299-318, b.java:340-363).
     *
     * No new runtime object is introduced. The copy is built off to the side
     * and installed only on success, so a failure leaves the context unchanged.
     * BounceLevel is read-only here and is never modified.
     */
    new_runtime = bounce_runtime_level_tiles_create(game->level);
    if (new_runtime == NULL
        || bounce_runtime_level_tiles_width(new_runtime)
            != bounce_level_width(game->level)
        || bounce_runtime_level_tiles_height(new_runtime)
            != bounce_level_height(game->level)) {
        bounce_runtime_level_tiles_destroy(new_runtime);
        return -1;
    }

    if (bounce_app_flow_record_runtime_initialized(&game->flow) != 0) {
        bounce_runtime_level_tiles_destroy(new_runtime);
        return -1;
    }

    /* Only the single runtime ownership slot changes. */
    game->runtime = new_runtime;
    return 0;
}

int bounce_game_activate_staged_level(BounceGame *game)
{
    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_LEVEL_LOADED
        || game->state != BOUNCE_GAME_STATE_LEVEL_LOADED
        || game->level == NULL
        || game->runtime == NULL
        || !game->player.initialized
        || bounce_app_flow_entry_phase(&game->flow)
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED
        || !bounce_app_flow_entry_camera_initialized(&game->flow)
        || bounce_app_flow_entry_gameplay_activated(&game->flow))
        return -1;

    /*
     * Record the flow transition first. It re-validates every prerequisite, so
     * a rejection leaves both flow and state untouched; the gameplay mirror is
     * only updated once the application transition has actually committed.
     */
    if (bounce_app_flow_record_gameplay_activated(&game->flow) != 0)
        return -1;

    /*
     * Native stand-in for display.setCurrent(this.v) (BounceGame.java:155).
     * bounce_game_sync_ui_state() deliberately does not mirror GAMEPLAY, so the
     * state is set explicitly here, exactly as bounce_game_activate_loaded_level
     * does for the legacy combined path.
     *
     * No timer is started: flow.next_tick_ms stays 0 and timer_started stays
     * false. No tick, movement, collision, dyn-thorn update, renderer, repaint,
     * audio, persistence, or input work happens here.
     */
    game->state = BOUNCE_GAME_STATE_PLAYING;

    /*
     * Initial lives, mirrored from the value the flow already owns.
     *
     * Java passes 3 as a literal argument on the new-game call:
     *   e.java:151   this.v.a(paramInt, 0, 3);
     * that is a(paramInt = level, 0 = initial score, 3 = initial lives), and it
     * is the only place a fresh gameplay run is given its life count. The native
     * flow already carries that number: BOUNCE_APP_ENTRY_INITIAL_LIVES
     * (app_flow.h:135) is recorded into gameplay_entry.initial_lives at
     * app_flow.c:684 when START_LEVEL is recorded, and read back through
     * bounce_app_flow_entry_initial_lives() (app_flow.c:913).
     *
     * Nothing in this staged chain wrote game->lives before now. bounce_game_init
     * (game.c:110) leaves it 0, bounce_game_load_level_entry (game.c:382) swaps
     * level ownership only, and bounce_game_initialize_player_entry (game.c:426)
     * writes only player and spawn. So production entered PLAYING with lives == 0,
     * the first KillBall (f.java:219, n.lives--) produced 0 - 1 == -1, and because
     * the game-over boundary is `lives < 0` (e.java:268, f.java:662) the very
     * first death stopped the game timer instead of the fourth. That made one
     * thorn look like the game exiting.
     *
     * This mirrors the same field the legacy combined path already mirrors here:
     * bounce_game_activate_loaded_level() sets game->lives = lives at game.c:312,
     * and the comment above notes this function mirrors that one for the state
     * assignment. The legacy path receives `lives` as a parameter; the staged
     * path has no such parameter, so it reads the flow-owned value instead of
     * introducing a second literal 3. BOUNCE_APP_ENTRY_INITIAL_LIVES stays the
     * single source of truth.
     *
     * Placement is with the other gameplay-mirror writes, after
     * bounce_app_flow_record_gameplay_activated() has committed, so a rejected
     * transition still leaves both flow and state untouched. This is a new-game
     * activation only: the sole production caller is app_begin_canonical_gameplay
     * (vertical_slice.c:4665), which is guarded by
     * flow.state == BOUNCE_APP_STATE_GAMEPLAY_ENTRY and
     * menu.pending_action == BOUNCE_MENU_ACTION_START_LEVEL (:4649-4650), so no
     * other lifecycle can reach it.
     *
     * The Death Core is untouched: KillBall still decrements, the q countdown
     * still runs 7 -> 0, respawn still happens for lives >= 0 including
     * lives == 0, and the lives < 0 boundary is unchanged.
     */
    game->lives = bounce_app_flow_entry_initial_lives(&game->flow);
    /*
     * STEP 12X-P4-C-D-B-R2 -- the New Game score reset, consumed here.
     *
     * This is the single activation boundary of the staged chain, and the comment
     * above already records why it is exact rather than merely convenient: the sole
     * production caller is app_begin_canonical_gameplay(), which is gated on
     * flow.state == BOUNCE_APP_STATE_GAMEPLAY_ENTRY and
     * menu.pending_action == BOUNCE_MENU_ACTION_START_LEVEL, so no other lifecycle
     * can reach it. A level load, a level transition, a death, a respawn and a menu
     * return therefore cannot trigger this line, which is exactly the property the
     * Java lifecycle requires (Step 13.27.11).
     *
     * The consume clears the flag as it reads, so the reset is one-shot: a later
     * Continue within the same session finds the flag false and preserves the score.
     *
     * Placement is with the other gameplay-mirror writes, after
     * bounce_app_flow_record_gameplay_activated() has committed, so a rejected
     * transition still leaves flow, state and score untouched.
     *
     * Only bounce_game_reset_score() is called, and only when the flag is set. No
     * other score field, no high score, no persistence and no UI reads or writes
     * score here.
     */
    if (bounce_app_flow_consume_new_game_score_reset(&game->flow) != 0
        && bounce_game_reset_score(game) != 0)
        return -1;
    return 0;
}

int bounce_game_initialize_game_timer(BounceGame *game)
{
    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_GAMEPLAY
        || game->state != BOUNCE_GAME_STATE_PLAYING
        || game->level == NULL
        || game->runtime == NULL
        || !game->player.initialized
        || bounce_app_flow_entry_phase(&game->flow)
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED
        || !bounce_app_flow_entry_gameplay_activated(&game->flow)
        || bounce_game_timer_is_started(&game->game_timer))
        return -1;

    /*
     * Start the platform-independent timer first. It is idempotent, matching
     * b.java:836-837, and carries the source-derived nominal 40 ms period. If
     * the flow record then rejects, the timer is left started rather than being
     * silently rewound, because the original does not un-start a timer either.
     */
    if (bounce_game_timer_start(&game->game_timer) != 0)
        return -1;

    if (bounce_app_flow_record_timer_initialized(&game->flow) != 0)
        return -1;
    return 0;
}

int bounce_game_tick_dispatch(
    BounceGame *game,
    bool reset_pending,
    bool splash_active,
    bool camera_gate_needed,
    BouncePlayerTickBody body,
    void *body_context,
    BounceTickDispatchBranch *branch_out
)
{
    if (branch_out != NULL)
        *branch_out = BOUNCE_TICK_DISPATCH_RESET_GATE;
    /*
     * STEP 12X-G-P3 -- the e.java:222 deferred-initialization latch, the first
     * statement of the tick exactly as it is the first statement of Java's
     * Tick():
     *
     *   e.java:222-226   if (this.d) { InitializeGame(); repaint(); return; }
     *
     * THIS FUNCTION IS THE NATIVE Tick(). e.Tick() is called by
     * b.GameTimerTick() (b.java:848-850), which is the sole run() body of
     * BounceTimer (BounceTimer.java:20-22, scheduled every 40 ms at :17), and
     * this function is the sole per-timer-callback gameplay entry on the native
     * side, reached through app_run_canonical_gameplay_tick().
     *
     * PLACED BEFORE THE GUARD DELIBERATELY. The guard below requires GAMEPLAY,
     * PLAYING, an installed level and runtime, an initialized player, the
     * TIMER_INITIALIZED phase, gameplay_activated, timer_started and a started
     * timer. A run that has just completed satisfies none of those: it is in
     * LEVEL_COMPLETE and the Step 12X-G-R2 tail has stopped its timer. Placing
     * the check after the guard would make the latch permanently unreachable,
     * because nothing but a real re-entry can rebuild that state. Java has no
     * such precondition at all -- the d test is unconditional, which is what
     * lets it run while the level-complete form is still on screen.
     *
     * THE LOAD IS THE EXISTING PRODUCTION LOADER, NOT A NEW PATH. There is no
     * second initialization here: the branch calls
     * bounce_game_load_level_entry(), the same function the staged entry uses
     * and the same function that carries the b.java:213 clear. Loading the level
     * is also exactly what InitializeGame() is distinguished by, since it
     * delegates to LoadLevelId(this.level).
     *
     * THE FINISHED RUN IS RELEASED FIRST, AND THAT IS NOT AN EXTRA STEP. Java's
     * LoadLevelId has no ownership guard at all: it assigns this.level and the
     * previous level's state array is simply dropped, so by the time the new
     * level exists the old one no longer does. The native loader refuses while a
     * runtime tile copy is installed (game.c:400), because replacing only the
     * level would split ownership of the copy. The completed run is therefore
     * released through bounce_game_release_level() -- the single existing
     * release mechanism, the same one app_end_canonical_gameplay() uses for the
     * Step 12X-G-R4 terminal release -- and only then is the level loaded. This
     * reproduces Java's order of effects instead of adding a step to it.
     * bounce_game_release_level() leaves flow.level_id untouched, so the level
     * being loaded is read before the release and is still correct after it.
     *
     * THE NATIVE-ONLY RE-STAGING IS NOT DONE HERE. The runtime tile copy, the
     * camera update, the player initialization and the activation that follow a
     * fresh load are the staged-entry chain
     * (app_advance_player_initialized_to_runtime_initialized and onwards), and
     * every one of those steps requires flow.state to be LEVEL_LOADED with
     * gameplay_entry.phase advanced, a state only a legitimate re-entry may
     * establish. Performing them from a terminal run would be inventing the
     * CONTINUE path that Step 12X-G-P4 owns, so this branch stops at the load
     * and leaves the run in LEVEL_COMPLETE.
     *
     * THE TIMER IS NOT RESTARTED, mirroring Java exactly. InitializeGame() does
     * not touch the timer; Java restarts it separately, from
     * BounceGame.java:201 via a(false, 0) -> :153 StartGameTimer(). Stopping it
     * here keeps the run in the state Step 12X-G-R2 established, and prevents a
     * follow-up tick from reaching the guard below and returning -1, which the
     * frame loop would turn into x11_context.failed = 1.
     *
     * A FAILED LOAD RETURNS -1 AND LEAVES THE LATCH ARMED, because the clear
     * lives inside the loader and only runs on success. That keeps a deferred
     * init retryable, and the frame loop's error funnel at vertical_slice.c
     * still handles a genuine failure exactly as it handles every other tick
     * failure.
     */
    if (game != NULL && game->deferred_init) {
        int deferred_level_id = game->flow.level_id;

        bounce_game_release_level(game);
        bounce_game_timer_stop(&game->game_timer);
        return bounce_game_load_level_entry(game, deferred_level_id);
    }
    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_GAMEPLAY
        || game->state != BOUNCE_GAME_STATE_PLAYING
        || game->level == NULL
        || game->runtime == NULL
        || !game->player.initialized
        || bounce_app_flow_entry_phase(&game->flow)
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_TIMER_INITIALIZED
        || !bounce_app_flow_entry_gameplay_activated(&game->flow)
        || !game->flow.gameplay_entry.timer_started
        || !bounce_game_timer_is_started(&game->game_timer))
        return -1;

    /*
     * e.java:222-226. The source runs InitializeGame() and repaint() here and
     * returns. Neither is executed: InitializeGame is a full re-initialization
     * belonging to a later milestone, and repaint is presentation. Recording the
     * branch is the whole point, so the early return is preserved exactly.
     */
    if (reset_pending) {
        if (branch_out != NULL)
            *branch_out = BOUNCE_TICK_DISPATCH_RESET_GATE;
        return 0;
    }

    /*
     * e.java:227-256. The source performs splash/menu handling, calls repaint(),
     * and returns. The native splash is the already-verified
     * BOUNCE_APP_STATE_SPLASH application path (app_update_ui /
     * bounce_app_flow_tick), which this seam must not duplicate or invoke.
     */
    if (splash_active) {
        if (branch_out != NULL)
            *branch_out = BOUNCE_TICK_DISPATCH_SPLASH_GATE;
        return 0;
    }

    /* e.java:258-259. */
    if (bounce_app_flow_tick_pre_countdown(&game->flow) < 0)
        return -1;

    /*
     * e.java:261 `synchronized (this.aq)`: a NATIVE SINGLE-THREAD BOUNDARY.
     * The native core is single-threaded and the player is embedded by value, so
     * there is no contended monitor and no mutex is introduced.
     */

    /*
     * e.java:262-263. The condition is evaluated by the caller's already-verified
     * native gate; no camera formula is duplicated here. e() is not invoked from
     * this seam: it would mutate l/k/v, which this dispatcher does not own. The
     * branch is recorded, and the caller supplies e() through the layer that owns
     * the file-local app_update_camera(). Mutually exclusive with the player body
     * below, exactly as e() and this.aq.b() are in the source.
     */
    if (camera_gate_needed) {
        if (branch_out != NULL)
            *branch_out = BOUNCE_TICK_DISPATCH_CAMERA;
        return 0;
    }

    /*
     * e.java:264-265 `else { this.aq.b(); }` is a single source statement that
     * both selects the branch and calls f.b(). The native records those two
     * aspects separately so neither counter can imply that physics ran.
     */
    if (bounce_game_timer_record_dispatch(&game->game_timer) != 0)
        return -1;

    /*
     * The PLAYER_TICK ENTRY: f.b() (f.java:652) is entered only through its
     * pre-physics prologue (f.java:653-669), which performs local
     * initializations, the z == 2 test, the division-toward-zero tile
     * coordinates, and the water sample, and then STOPS before f.java:670. The
     * f.b() physics body past f.java:669 is NOT executed, so no gravity, velocity
     * change, movement, collision, power-up, death, respawn, score, audio, or
     * rendering effect occurs.
     */
    if (bounce_player_tick_entry(
            &game->player,
            game->runtime,
            &game->player.tick_entry
        ) != 0)
        return -1;
    if (bounce_game_timer_record_player_tick_entry(&game->game_timer) != 0)
        return -1;

    /*
     * Thin continuation: run the EXISTING native physics for f.java:670 onward.
     * The body delegates to app_step_vertical(), app_step_horizontal(), and
     * app_update_camera_horizontal() in the previously working order; it is
     * supplied by the layer that owns those file-local functions, so none of
     * them is modified, copied, renamed, or refactored here.
     *
     * A NULL body stops after the f.b() prologue, which is the pre-integration
     * behavior and remains supported.
     */
    if (body != NULL) {
        if (body(game, body_context) != 0)
            return -1;
        if (bounce_game_timer_record_player_physics(&game->game_timer) != 0)
            return -1;
    }

    if (branch_out != NULL)
        *branch_out = BOUNCE_TICK_DISPATCH_PLAYER_TICK;

    /*
     * STEP 12X-G-R2 -- DEFERRED TIMER STOP, the tick-safe boundary.
     *
     * b.java:835-846 StartGameTimer()/StopGameTimer(), and the two source stops
     * this reproduces:
     *
     *   e.java:270            StopGameTimer();          // game over
     *   BounceGame.java:200   this.v.StopGameTimer();  // ShowLevelComplete()
     *
     * WHY THIS IS HERE AND NOT AT THE TRANSITION. Both native transitions run
     * INSIDE the body above: bounce_game_mark_level_complete() from
     * app_vertical_collision_info() (tile 9), and app_respawn_after_death()
     * from app_player_physics_continuation() (vertical_slice.c:3812). Step
     * 12X-G-R1 proved that stopping the timer from either of them, mid-tick,
     * made this same tick fail at :756 above, because
     * bounce_game_timer_record_player_physics() rejects !started
     * (game_timer.c:115-116). That -1 propagated to the frame loop at
     * vertical_slice.c:15833, which set x11_context.failed = 1 and returned
     * EXIT_FAILURE, closing the window on the very first portal entry.
     *
     * The guard at :756 is CORRECT and is deliberately not weakened. The defect
     * was stopping a timer that the running tick still had to account for.
     *
     * WHY THIS IS THE RIGHT SPOT. It is the last statement of the tick that ran
     * the body, so every timer-bookkeeping call in the tick has already
     * succeeded by the time it executes -- record_dispatch (:722),
     * record_player_tick_entry (:740), the body's own
     * record_dynamic_thorn_update (vertical_slice.c:3833), and
     * record_player_physics (:756). It is still inside the same call, so no
     * other tick can observe an intermediate state.
     *
     * NO NEW STATE. game->state is the lifecycle signal that already exists and
     * that both transitions already set: BOUNCE_GAME_STATE_GAME_OVER at
     * game.c:801 and BOUNCE_GAME_STATE_LEVEL_COMPLETE at game.c:862. The two
     * terminal values are named explicitly rather than tested as "!= PLAYING"
     * so that no other end-state can silently acquire a timer stop it did not
     * have before.
     *
     * WHY NORMAL GAMEPLAY IS UNTOUCHED. A tick that ends in
     * BOUNCE_GAME_STATE_PLAYING skips this entirely, so started stays true and
     * every counter behaves exactly as before. The stop is idempotent
     * (game_timer.c:31-35), so a repeated visit is harmless, and
     * bounce_game_timer_init() is a full memset (game_timer.c:5-10), so the
     * next level start is unaffected.
     */
    if (game->state == BOUNCE_GAME_STATE_LEVEL_COMPLETE
        || game->state == BOUNCE_GAME_STATE_GAME_OVER
        /* STEP 12X-P4-B: the game-complete terminal state. Java stops the timer
         * inside ShowGameEnd (BounceGame.java:178); native keeps that stop at
         * this tick tail, which is the only place it is safe (see the R2 note
         * above), so the new state has to be listed here or the timer would keep
         * running with no gameplay state able to consume a tick. */
        || game->state == BOUNCE_GAME_STATE_GAME_END)
        bounce_game_timer_stop(&game->game_timer);
    return 0;
}

int bounce_game_apply_event(BounceGame *game, BounceAppEvent event)
{
    if (game == NULL)
        return -1;

    if (event == BOUNCE_APP_EVENT_NEXT_LEVEL) {
        /* Progression/loading policy is intentionally a later milestone. */
        return -1;
    }
    if (event == BOUNCE_APP_EVENT_SPLASH_DONE)
        return bounce_game_enter_menu(game);

    if (event == BOUNCE_APP_EVENT_START) {
        if (game->state != BOUNCE_GAME_STATE_MENU
            || game->flow.state != BOUNCE_APP_STATE_MENU
            || game->level == NULL
            || game->runtime == NULL
            || !game->player.initialized
            || bounce_app_flow_apply(&game->flow, event) != 0)
            return -1;
        game->state = BOUNCE_GAME_STATE_PLAYING;
        return 0;
    }

    if (bounce_app_flow_apply(&game->flow, event) != 0)
        return -1;

    switch (event) {
        case BOUNCE_APP_EVENT_PAUSE:
            game->state = BOUNCE_GAME_STATE_PAUSED;
            break;
        case BOUNCE_APP_EVENT_RESUME:
            game->state = BOUNCE_GAME_STATE_PLAYING;
            break;
        case BOUNCE_APP_EVENT_GAME_OVER:
            game->game_over = true;
            game->state = BOUNCE_GAME_STATE_GAME_OVER;
            break;
        case BOUNCE_APP_EVENT_LEVEL_COMPLETE:
            game->level_complete = true;
            game->state = BOUNCE_GAME_STATE_LEVEL_COMPLETE;
            break;
        case BOUNCE_APP_EVENT_GAME_END:
            /*
             * STEP 12X-P4-B. Deliberately does NOT set game->game_over: that flag
             * stands for the death result (e.java:271), and Step 12X-P4-A
             * established that the two terminal forms must stay distinct.
             */
            game->state = BOUNCE_GAME_STATE_GAME_END;
            break;
        case BOUNCE_APP_EVENT_MENU:
            game->state = BOUNCE_GAME_STATE_MENU;
            break;
        case BOUNCE_APP_EVENT_START:
        case BOUNCE_APP_EVENT_NEXT_LEVEL:
        case BOUNCE_APP_EVENT_SPLASH_DONE:
            return -1;
    }
    return 0;
}

int bounce_game_mark_dead(BounceGame *game)
{
    if (game == NULL || game->state != BOUNCE_GAME_STATE_PLAYING)
        return -1;

    /* Death side effects and life decrement remain outside this milestone. */
    game->dead = true;
    game->state = BOUNCE_GAME_STATE_DEAD;
    return 0;
}

int bounce_game_request_respawn(BounceGame *game)
{
    if (game == NULL || game->state != BOUNCE_GAME_STATE_DEAD)
        return -1;

    /* The boundary is explicit; respawn timing/position policy is UNKNOWN. */
    game->respawn_requested = true;
    game->state = BOUNCE_GAME_STATE_RESPAWN;
    return 0;
}

int bounce_game_mark_level_complete(BounceGame *game)
{
    if (game == NULL || game->state != BOUNCE_GAME_STATE_PLAYING)
        return -1;

    /*
     * STEP 12T — Native Level Complete Entry Boundary.
     *
     * Native equivalent of Java e.java:313-316:
     *   this.e = false;              // finish event is one-time (tile-9 collision)
     *   this.TODO_ExitUnlocked = false;  // clear exit unlock
     *
     * The finish event (tile-9 collision) is a one-time transition — no persistent
     * flag to clear. The state check above (PLAYING) provides one-way protection.
     *
     * STEP 12X-G — the APPLICATION-level transition this boundary was missing.
     *
     * Step 12X-F-R1 traced the defect this closes. Marking only the game state
     * left flow->state at BOUNCE_APP_STATE_GAMEPLAY, so on the very next frame
     *
     *   app_canonical_gameplay_active()   vertical_slice.c:4932
     *       = (flow->state == GAMEPLAY && timer_is_started)   -> still TRUE
     *   app_run_canonical_gameplay_tick() -> app_timer_callback_dispatch()
     *   bounce_game_tick_dispatch()       game.c:658
     *       || game->state != BOUNCE_GAME_STATE_PLAYING      -> TRUE
     *       return -1
     *
     * and that -1 propagated to the frame loop at vertical_slice.c:15833, which
     * set x11_context.failed = 1 and returned EXIT_FAILURE. Reaching the exit
     * terminated the process. Runtime-reproduced before this change.
     *
     * The fix is the transition itself, not a suppression: the game state and
     * the application state are now moved together through the event the flow
     * layer already owns, exactly as the game-over arm does at
     * vertical_slice.c:1991 with BOUNCE_APP_EVENT_GAME_OVER. Once
     * flow->state is BOUNCE_APP_STATE_LEVEL_COMPLETE,
     * app_canonical_gameplay_active() is false, app_update_production_game()
     * takes its documented "gameplay has ended" route (vertical_slice.c:5026,
     * which returns 0 by design), and render_app_state() dispatches
     * render_level_complete_state() through the normal switch at
     * vertical_slice.c:4912. The game.c:658 guard is deliberately left intact
     * and is now simply never reached from this path.
     *
     * The fallible flow call is made first so a rejection leaves this function
     * with no partial effect. bounce_app_flow_apply() only accepts
     * BOUNCE_APP_EVENT_LEVEL_COMPLETE from BOUNCE_APP_STATE_GAMEPLAY
     * (app_flow.c:150-154), which is the state the source's q()/repaint() pair
     * at e.java:311-312 is running in.
     *
     * TIMER. BounceGame.java:199-213 ShowLevelComplete() opens with
     * `this.v.StopGameTimer();` at :200, the same call the game-over arm makes
     * for e.java:270.
     *
     * STEP 12X-G-R2: that stop is NOT performed here. This function runs inside
     * the gameplay tick body, and stopping the timer mid-tick made the SAME tick
     * fail at the bounce_game_timer_record_player_physics() call that
     * bounce_game_tick_dispatch() makes after the body returns, because that
     * call rejects a stopped timer (game_timer.c:115-116). The -1 propagated to
     * the frame loop and closed the window. The stop now happens at the end of
     * the same tick, in bounce_game_tick_dispatch(), which is the one place
     * where every timer-bookkeeping call has already completed. game->state is
     * set to BOUNCE_GAME_STATE_LEVEL_COMPLETE below exactly as before, and that
     * existing state is what the deferred stop keys on, so no new state and no
     * new flag was needed.
     *
     * This list was written when the block above was first created and listed what
     * Step 12T did NOT implement. Its status has since changed, so it is restated
     * here against the current tree rather than left to become a false claim:
     *
     *   this.d = true (deferred-init gate)   IMPLEMENTED, Step 12X-G-P3
     *   AddScore(5000)                       IMPLEMENTED, Step 12X-P4-C-D-C-B3
     *   final-level branch                   IMPLEMENTED, Step 12X-P4-B
     *   the CONTINUE transition              IMPLEMENTED, Step 12X-G-P4
     *   TODO_ShowInstructions(true) form     still deferred
     *   the ShowLevelComplete() Form         still deferred
     *   the RMS write inside WriteToStore()  still deferred
     *
     * Persistence is the one item on this list that is wholly absent: native has no
     * WriteToStore() and no RMS, so Java's e.java:319 between the bonus and the
     * final-level test has no counterpart and is elided rather than reordered.
     *
     * render_level_complete_state() is the pre-existing native placeholder
     * (vertical_slice.c:4827, explicitly labelled "TEMPORARY NATIVE UI
     * REPRESENTATION"). It is not redesigned here.
     *
     * STEP 12X-G-P2 -- the progression part of this block now DOES run.
     *
     * bounce_app_flow_complete_level() below performs e.java:317 this.level++
     * and then BounceGame.java:418-420, which raises MaxLevels to
     * min(level, 11) when the level has advanced past it. It runs AFTER the
     * fallible flow transition and after the two game-state writes, so a
     * rejected transition still leaves the level and the count untouched: the
     * e.java:313 guard is the PLAYING state check above, and a block that never
     * ran cannot have advanced anything.
     *
     * It is called last, and only here. The two entry points into the Java
     * block are e.java:286-297, which is reached exactly when
     * TODO_ExitUnlocked and z are both set and the camera window covers the
     * exit, and the e.java:407 debug key. This function has exactly one caller,
     * app_vertical_collision_info() at vertical_slice.c:2195, so the mutation
     * cannot run twice for one completion: game->state leaves PLAYING above and
     * the state check rejects any second call.
     *
     * ORDER MATCHES THE SOURCE. e.java:317 advances the level, e.java:318 scores,
     * e.java:319 writes the store, and only then does e.java:320 read
     * this.level. Scoring is absent, so advancing the level and then
     * synchronising the count is the same two-mutation order with the score
     * elided, not a reordering.
     */
    if (bounce_app_flow_apply(
            &game->flow,
            BOUNCE_APP_EVENT_LEVEL_COMPLETE
        ) != 0)
        return -1;
    game->TODO_ExitUnlocked = false;
    game->level_complete = true;
    game->state = BOUNCE_GAME_STATE_LEVEL_COMPLETE;
    /*
     * STEP 12X-G-P3 -- the e.java:316 latch arm.
     *
     * Java arms the deferred-init latch in the completion block, immediately
     * before the increment:
     *
     *   e.java:316   this.d = true;
     *   e.java:317   this.level++;
     *
     * so this write is placed before bounce_app_flow_complete_level() below in
     * order to keep that order. It is placed after the two game-state writes
     * because those are native's stand-in for the e.java:314 `this.e = false`
     * that has to happen first.
     *
     * ARMING IS NOT CONTINUATION. The latch is consumed by the first statement
     * of bounce_game_tick_dispatch(), and no tick is dispatched while the flow
     * is in LEVEL_COMPLETE: app_canonical_gameplay_active() requires
     * flow.state == BOUNCE_APP_STATE_GAMEPLAY, and the Step 12X-G-R2 tail has
     * already stopped the timer. Nothing in the tree can reach the tick from
     * here, so the application stays in LEVEL_COMPLETE with the latch armed and
     * waiting. No input path consumes it; that is deferred to Step 12X-G-P4.
     */
    game->deferred_init = true;
    /*
     * The level becomes the NEXT level, so a later load reads the advanced
     * value. flow->level_id is the authoritative field; game->level_id is left
     * alone on purpose, because it mirrors the currently loaded resource and is
     * reset to BOUNCE_GAME_LEVEL_ID_UNKNOWN by bounce_game_release_level().
     */
    if (bounce_app_flow_complete_level(&game->flow) != 0)
        return -1;
    /*
     * STEP 12X-P4-C-D-C-B3 -- the completion bonus.
     *
     * e.java:313-327, the whole completion block:
     *
     *     if (this.e) {
     *         this.e = false;
     *         this.TODO_ExitUnlocked = false;
     *         this.d = true;
     *         this.level++;
     *         AddScore(5000);
     *         this.game.WriteToStore();
     *         if (this.level > 11) { ... }
     *     }
     *
     * PLACEMENT MATCHES THE SOURCE. Java awards the bonus at e.java:318, which is
     * after this.level++ at e.java:317 and before the `this.level > 11` test at
     * e.java:320. Native's two correspondences are the calls immediately above and
     * below this comment:
     *
     *   - bounce_app_flow_complete_level() above is the level++ (app_flow.c:697). It
     *     has already run, so flow->level_id has advanced and available_level_count
     *     has been clamped, which is what makes the test below decidable at all;
     *   - the bounce_app_flow_level_is_final() test below is the `this.level > 11`.
     *
     * So this call occupies the same position in the mutation order as e.java:318.
     * Java's intermediate WriteToStore() at e.java:319 is the one elided statement:
     * native has no persistence, which is a separate deferred milestone. That is an
     * elision, not a reordering, and it is the same shape as the hoop award in
     * app_collect_hoop() for the same reason.
     *
     * IT IS DELIBERATELY NOT INSIDE THE FINAL-LEVEL BRANCH. Java awards 5000 on
     * EVERY completion, the final level included; the `this.level > 11` test at
     * e.java:320 only chooses which screen follows. Putting the award inside that
     * branch would silently turn a completion bonus into a completion-screen reward
     * and would forfeit 5000 exactly once per playthrough -- on the one level where
     * the total matters most. A final-level completion therefore awards 5000 and then
     * falls through to BOUNCE_APP_EVENT_GAME_END.
     *
     * ONCE-ONLY, inherited from the one-way state transition, not a new flag. This
     * function has exactly one call site (vertical_slice.c:2246) and opens with
     *
     *     if (game->state != BOUNCE_GAME_STATE_PLAYING) return -1;
     *
     * Every statement from bounce_app_flow_apply() downwards moves the game out of
     * PLAYING -- BOUNCE_GAME_STATE_LEVEL_COMPLETE above, and
     * BOUNCE_GAME_STATE_GAME_END in the final-level arm below -- so a second
     * completion cannot reach this line even on a later frame. That guard is the
     * native counterpart of Java's `if (this.e)` at e.java:313, whose this.e is
     * cleared at e.java:314 by this same block. No new flag and no new state were
     * added.
     *
     * The result is deliberately discarded, following the convention the hoop award
     * established in app_collect_hoop(): bounce_game_add_score() returns -1 only for
     * a NULL game, and game was already proven non-NULL by the guard at the top of
     * this function, so the call cannot fail and no error path is invented.
     */
    (void)bounce_game_add_score(game, 5000);
    /*
     * STEP 12X-P4-B -- the final-level branch, e.java:320-322.
     *
     *   if (this.level > 11) { this.game.TODO_ShowInstructions(true); }
     *   else { this.H = false; this.game.ShowLevelComplete(); repaint(); }
     *
     * Placed AFTER bounce_app_flow_complete_level(), which is what makes the
     * test meaningful: that call is the native WriteToStore() at e.java:319, and
     * it has already advanced flow->level_id to 12 and clamped
     * available_level_count to 11. Only now is "the level I just completed was
     * the last playable one" a decidable question. Testing before it would read
     * 11 > 10 and wrongly report a mid-game level as final.
     *
     * ORDER PRESERVED. Java arms d at :316 and increments at :317 BEFORE this
     * test at :320, and so does this: deferred_init is armed and the level is
     * advanced above, and only then is the branch taken.
     *
     * THE LATCH IS CLEARED HERE, WHICH DEVIATES FROM JAVA'S LITERAL ORDER, and
     * the deviation is deliberate. Java leaves d armed because its only consumer
     * is Tick(), and ShowGameEnd stops the game timer at BounceGame.java:178, so
     * no tick ever runs and the latch is inert by construction. Native's latch
     * is not inert by construction: the Step 12X-G-P3 branch at the head of
     * bounce_game_tick_dispatch() releases the run and then attempts
     * bounce_game_load_level_entry(flow->level_id), which for level_id 12 is
     * refused by the FIRST..LAST guard, returning -1 into the frame loop's error
     * funnel. Clearing the latch makes "Level 12 is never initialised" a
     * structural property of this branch rather than a property of the tick
     * gate happening to stay closed, and nothing observable differs: in both
     * implementations no further level is loaded.
     */
    if (bounce_app_flow_level_is_final(&game->flow)) {
        game->deferred_init = false;
        if (bounce_app_flow_apply(
                &game->flow,
                BOUNCE_APP_EVENT_GAME_END
            ) != 0)
            return -1;
        game->state = BOUNCE_GAME_STATE_GAME_END;
    }
    return 0;
}

/*
 * STEP 12X-P4-C-D-B -- the native counterpart of e.java:153-156.
 *
 * Java is `this.score += paramInt;` plus `this.y = true;`. Only the mutation is
 * reproduced: `y` is the HUD dirty flag, and the HUD renders no score yet, so
 * setting it here would be a side effect with no reader.
 *
 * There is deliberately no validation, no clamp, no saturation, no event and no
 * persistence, and no gameplay caller exists yet. The hoop, crystal, crystal ball
 * and level-completion integrations are separate milestones.
 */
int bounce_game_add_score(BounceGame *game, int32_t amount)
{
    if (game == NULL)
        return -1;
    game->score = java_int32_add(game->score, amount);
    return 0;
}

int bounce_game_reset_score(BounceGame *game)
{
    if (game == NULL)
        return -1;
    game->score = 0;
    return 0;
}

int bounce_game_continue_level(BounceGame *game)
{
    int continued_level_id;
    int continued_lives;

    if (game == NULL
        || game->flow.state != BOUNCE_APP_STATE_LEVEL_COMPLETE)
        return -1;
    /*
     * Only a level that is actually waiting to be started may be continued.
     * bounce_game_mark_level_complete() is the single writer of deferred_init,
     * and it also advances flow->level_id, so this pairing cannot be forged by
     * any other route. A refused CONTINUE -- for example the level-11 case,
     * where flow->level_id is 12 and no level file exists -- leaves the
     * completed run, its resources and its latch completely untouched, because
     * the flow transition is attempted first and can still reject.
     */
    if (!game->deferred_init)
        return -1;
    /*
     * STEP P6 -- the completed run's remaining lives are read HERE, before the
     * release below zeroes them.
     *
     * Java has no equivalent read because it has no equivalent write to undo:
     * `lives` is written in exactly four places in the whole source (f.java:219
     * death, f.java:612 1-up, e.java:84 new game / level select, e.java:95 Record 3
     * resume) and NONE of them is on the post-completion path. The completion block
     * (e.java:313-327), ShowLevelComplete() (:199-213), the CONTINUE branch
     * (:263-266) and InitializeGame() (e.java:115-124) all leave the field alone, so
     * the value the death or 1-up write last produced is simply still there when the
     * next level starts.
     *
     * Native has to be told, because bounce_game_release_level() clears the field to
     * 0 (game.c:299) and bounce_app_flow_begin_gameplay_entry() then records the
     * constant BOUNCE_APP_ENTRY_INITIAL_LIVES, which stage 5 applies to game->lives
     * (game.c:761). Capturing the value first is what lets the carrier be corrected
     * below. Read before the release, never restored onto game->lives afterwards.
     */
    continued_lives = game->lives;
    continued_level_id = bounce_app_flow_continue_level(&game->flow);
    if (continued_level_id < 0)
        return -1;
    /*
     * THE FINISHED RUN IS RELEASED, AND THIS IS THE LoadLevelId OVERWRITE.
     * Java's LoadLevelId has no ownership guard: it assigns this.level and the
     * previous level's state is simply gone, so the new level's state array is
     * in place by the time the old one is dropped. The native loader refuses
     * while a runtime tile copy is installed (game.c:400), because replacing
     * only the level would split ownership of that copy -- so releasing the
     * completed run first is the same order of effects, not an extra step.
     *
     * bounce_game_release_level() is the SINGLE existing release mechanism; it
     * is the one app_end_canonical_gameplay() uses for the Step 12X-G-R4
     * terminal release and the one the Step 12X-G-P3 deferred branch uses, and
     * it is NULL-safe, so nothing is double-freed. It deliberately does NOT
     * clear deferred_init: the latch stays armed until
     * bounce_game_load_level_entry() clears it as part of a real load, which is
     * exactly the b.java:213 clear.
     */
    bounce_game_release_level(game);
    /*
     * The existing gameplay-entry record, then the existing staging driver in
     * the frame loop. bounce_app_flow_begin_gameplay_entry() moves
     * ACTION_BOUNDARY -> GAMEPLAY_ENTRY, and app_update_production_game() runs
     * app_begin_canonical_gameplay() unconditionally on every frame, which then
     * performs load -> player -> runtime -> camera -> activate -> timer with no
     * change at all. Nothing of that chain is duplicated or reordered here.
     */
    if (bounce_app_flow_begin_gameplay_entry(&game->flow) != 0)
        return -1;
    /*
     * STEP P6 -- re-apply the captured lives into the EXISTING entry carrier, after
     * the call that overwrote it with the constant. This is the same placement and
     * the same carrier that bounce_app_flow_begin_resume_entry() uses for the R1/R2
     * Record 3 restore (app_flow.c:995-1002), for the same reason: stage 5 reads
     * gameplay_entry.initial_lives at game.c:761, so correcting the carrier keeps the
     * carrier and the live field consistent by construction instead of patching
     * game->lives after activation.
     *
     * Nothing else changes. The level is still the already-advanced flow->level_id,
     * no second increment occurs, the release above already happened for the loader's
     * ownership guard, and the score is untouched because the New Game reset flag is
     * not set on this route.
     */
    if (bounce_app_flow_set_entry_initial_lives(&game->flow, continued_lives) != 0)
        return -1;
    /*
     * Mirror the flow into the gameplay state. BOUNCE_APP_STATE_LEVEL_COMPLETE
     * is an explicit no-op case in bounce_game_sync_ui_state(), so nothing else
     * moves this field until the staging chain reaches
     * bounce_game_activate_staged_level().
     */
    game->state = BOUNCE_GAME_STATE_ACTION_BOUNDARY;
    return continued_level_id;
}

int bounce_game_mark_game_over(BounceGame *game)
{
    if (game == NULL || game->state != BOUNCE_GAME_STATE_DEAD)
        return -1;

    game->game_over = true;
    game->state = BOUNCE_GAME_STATE_GAME_OVER;
    return 0;
}

void bounce_game_shutdown(BounceGame *game)
{
    if (game == NULL)
        return;

    bounce_game_release_level(game);
    bounce_app_flow_init(&game->flow);
    bounce_input_reset(&game->input);
    game->state = BOUNCE_GAME_STATE_SPLASH;
    game->level_id = BOUNCE_GAME_LEVEL_ID_UNKNOWN;
    game->next_player_tick_ms = 0;
}
