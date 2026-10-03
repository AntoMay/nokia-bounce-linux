#include "player.h"

#include <stdint.h>
#include <string.h>

#include "collision_query.h"

enum {
    BOUNCE_PLAYER_TILE_SIZE = 12,
    /* f.java:210-211 UseRegularBall() and f.java:169-170 UseBigBall().  The
     * probe prologue below writes the two big-ball values as literals, matching
     * the source; these names exist for the new constructor side-effect code. */
    BOUNCE_PLAYER_REGULAR_BALL_SIZE = 12,
    BOUNCE_PLAYER_REGULAR_HALF_SIZE = 6,
    BOUNCE_PLAYER_BIG_BALL_SIZE = 16,
    BOUNCE_PLAYER_BIG_HALF_SIZE = 8
};

void bounce_player_reset(BouncePlayer *player)
{
    if (player == NULL)
        return;
    memset(player, 0, sizeof *player);
}

int bounce_player_tick_entry(
    const BouncePlayer *player,
    const BounceRuntimeLevelTiles *runtime,
    BouncePlayerTickEntry *entry_out
)
{
    BouncePlayerTickEntry entry;
    uint16_t tile_value;

    if (entry_out != NULL)
        memset(entry_out, 0, sizeof *entry_out);
    if (player == NULL || runtime == NULL || entry_out == NULL)
        return -1;
    if (!player->initialized)
        return -1;

    memset(&entry, 0, sizeof entry);

    /*
     * f.java:653-657, verbatim. `i` is a copy of the player's world X; j, k,
     * b1, and bool1 are frame locals. b1 is a Java `byte`, therefore signed.
     */
    entry.i = player->TODO_unkX;
    entry.j = 0;
    entry.k = 0;
    entry.b1 = 0;
    entry.bool1 = false;
    entry.entered = true;

    /*
     * f.java:658-666. z == 2 is tested FIRST, before the water sample and before
     * any gravity-constant assignment, and the branch returns without reaching
     * :667. The branch's own mutations (q-- at :659, z = 1 at :661, and the
     * surrounding-game n.e = true at :663) are death/respawn and game-over side
     * effects, so only the branch selection is recorded.
     */
    if (player->z == 2) {
        entry.z2_early_return = true;
        *entry_out = entry;
        return 0;
    }

    /*
     * f.java:667-668. Java int division truncates toward zero, which is exactly
     * what C signed division does, including for negative coordinates.
     */
    entry.tileX = player->TODO_unkX / BOUNCE_PLAYER_TILE_SIZE;
    entry.tileY = player->TODO_unkY / BOUNCE_PLAYER_TILE_SIZE;

    /*
     * f.java:669. This is the LAST pre-physics operation: it reads the
     * surrounding game's LevelTiles and writes only the local isInWater.
     *
     * NATIVE BOUNDARY: the source would raise an uncaught
     * ArrayIndexOutOfBoundsException here for an out-of-range coordinate; this
     * rejects the call instead of reproducing a crash.
     */
    if (bounce_runtime_level_tiles_get(
            runtime, (int)entry.tileX, (int)entry.tileY, &tile_value) != 0)
        return -1;
    entry.is_in_water = (tile_value & 0x40u) != 0u;

    /*
     * PRE-PHYSICS STOP POINT.
     *
     * Execution stops here, immediately before f.java:670. The water/ball-size
     * block that follows assigns the gravity constants k and j, and contains the
     * first mutation reachable in the normal path: `this.o = -10` at
     * f.java:675, a vertical-velocity write. Gravity, velocity changes,
     * movement, collision, power-ups, and every later operation are therefore
     * NOT executed.
     */
    *entry_out = entry;
    return 0;
}

int bounce_player_initialize_from_level(
    const BounceLevel *level,
    BouncePlayer *player_out,
    BouncePlayerSpawn *spawn_out
)
{
    BouncePlayerConstructorState constructor_state;
    int d;
    int c;
    int b;

    if (player_out != NULL)
        bounce_player_reset(player_out);
    if (spawn_out != NULL)
        memset(spawn_out, 0, sizeof *spawn_out);

    if (level == NULL || player_out == NULL || spawn_out == NULL)
        return -1;

    if (bounce_level_initialize_new_game_player_data(
            level,
            &constructor_state,
            &d,
            &c,
            &b
        ) != 0)
        return -1;

    player_out->TODO_unkX = constructor_state.TODO_unkX;
    player_out->TODO_unkY = constructor_state.TODO_unkY;
    player_out->ballSize = constructor_state.ballSize;
    player_out->p = constructor_state.p;
    player_out->horizontal_velocity = constructor_state.l;
    player_out->o = constructor_state.o;
    player_out->t = constructor_state.t;
    player_out->powerUpGravity = constructor_state.powerUpGravity;
    player_out->TODO_somePowerUp3 = constructor_state.TODO_somePowerUp3;
    player_out->C = constructor_state.C;
    player_out->z = constructor_state.z;
    player_out->q = constructor_state.q;
    player_out->m = constructor_state.m;
    player_out->u = constructor_state.u;
    /* f.java:117 initialises f.v in the same constructor block as f.u at
     * :118.  The probe that can raise it runs after this copy, against the
     * player, but the boundary is completed here so the constructor state is
     * carried in full and the field cannot be silently dropped. */
    player_out->v = constructor_state.v;
    player_out->current_ball_mode = constructor_state.current_ball_mode;
    player_out->initialized = true;

    spawn_out->initialized = true;
    spawn_out->world_x = constructor_state.TODO_unkX;
    spawn_out->world_y = constructor_state.TODO_unkY;
    spawn_out->tile_x = d;
    spawn_out->tile_y = c;
    spawn_out->ball_size = b;
    spawn_out->half_size = constructor_state.p;
    spawn_out->ball_mode = constructor_state.current_ball_mode;
    return 0;
}

/* Free-space probe source: one indirection so the six-direction algorithm
 * below exists exactly once and serves both tile owners. */
typedef bool (*BouncePlayerFreeSpaceProbe)(
    int32_t x,
    int32_t y,
    int32_t half_size,
    const void *context
);

static bool player_probe_runtime(
    int32_t x,
    int32_t y,
    int32_t half_size,
    const void *context
)
{
    return bounce_collision_query(
        x,
        y,
        half_size,
        (const BounceRuntimeLevelTiles *)context
    );
}

/*
 * The constructor probe's own context.
 *
 * `level` is the pristine BounceLevel, which is what f.java:431 reads.  `view`
 * is a short-lived runtime tile copy of exactly those bytes, needed only because
 * bounce_collision_tile_overlaps() -- the already-verified single-tile
 * predicate -- takes a runtime owner.  It is created and destroyed inside
 * bounce_player_use_big_ball_level, is never retained, and is never written:
 * bounce_level_tile_value_from_raw() is the identity (level.c) and the
 * dynamic-thorn records come from the same bounce_level_dyn_thorn_record()
 * decode, so the view and the level agree tile for tile.  It exists so the
 * identification pass below can reuse the verified geometry rather than
 * duplicate the masks, the dispatch, and the traversal.
 */
typedef struct PlayerConstructorProbeContext {
    BouncePlayer *player;
    const BounceLevel *level;
    const BounceRuntimeLevelTiles *view;
} PlayerConstructorProbeContext;

static int32_t java_int32_negate(int32_t value)
{
    uint32_t bits = UINT32_C(0) - (uint32_t)value;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_shift_right_one(int32_t value)
{
    uint32_t bits = ((uint32_t)value >> 1)
        | ((uint32_t)value & UINT32_C(0x80000000));
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

/*
 * f.java:234-270 b(int tileId), the l/o rewriter that f.java:361 reaches from
 * c() under the !m guard.
 *
 * This is the same transcription as app_apply_slope_response() in
 * vertical_slice.c, which serves the verified per-tick vertical and horizontal
 * paths.  It is deliberately repeated here instead of shared: the constructor
 * addition must not be able to alter the already-verified A1 collision code in
 * any way, and this keeps that guarantee structural rather than reviewed.
 */
static void player_apply_slope_response(
    BouncePlayer *player,
    uint32_t tile_id
)
{
    int32_t previous_horizontal;
    int32_t previous_vertical;

    if (player == NULL)
        return;
    previous_horizontal = player->horizontal_velocity;
    previous_vertical = player->o;
    switch (tile_id) {
        case 30:
            player->horizontal_velocity =
                player->horizontal_velocity < previous_vertical
                ? player->horizontal_velocity
                : java_int32_negate(java_int32_shift_right_one(previous_vertical));
            player->o = java_int32_negate(previous_horizontal);
            break;
        case 31:
            player->horizontal_velocity =
                player->horizontal_velocity
                    > java_int32_negate(previous_vertical)
                ? player->horizontal_velocity
                : java_int32_shift_right_one(previous_vertical);
            player->o = previous_horizontal;
            break;
        case 32:
            player->horizontal_velocity =
                player->horizontal_velocity > previous_vertical
                ? player->horizontal_velocity
                : java_int32_negate(java_int32_shift_right_one(previous_vertical));
            player->o = java_int32_negate(previous_horizontal);
            break;
        case 33:
            player->horizontal_velocity =
                java_int32_negate(player->horizontal_velocity) > previous_vertical
                ? player->horizontal_velocity
                : java_int32_shift_right_one(previous_vertical);
            player->o = previous_horizontal;
            break;
        case 34:
            player->horizontal_velocity =
                player->horizontal_velocity < previous_vertical
                ? player->horizontal_velocity
                : java_int32_negate(previous_vertical);
            player->o = java_int32_negate(previous_horizontal);
            break;
        case 35:
            player->horizontal_velocity =
                player->horizontal_velocity
                    > java_int32_negate(previous_vertical)
                ? player->horizontal_velocity
                : previous_vertical;
            player->o = previous_horizontal;
            break;
        case 36:
            player->horizontal_velocity =
                player->horizontal_velocity > previous_vertical
                ? player->horizontal_velocity
                : java_int32_negate(previous_vertical);
            player->o = java_int32_negate(previous_horizontal);
            break;
        case 37:
            player->horizontal_velocity =
                java_int32_negate(player->horizontal_velocity)
                    > previous_vertical
                ? player->horizontal_velocity
                : previous_vertical;
            player->o = previous_horizontal;
            break;
        default:
            break;
    }
}

/*
 * f.java:154-166 TODO_CheckBallCollision, reduced to the A1-relevant subset of
 * f.java:423-470 and applied in the same X-outer / Y-inner order.
 *
 * The source helper mutates the player and returns a verdict, so the walk's
 * accept/reject decision and the player state are produced together.  Here the
 * verdict stays with the unchanged pure query in player_probe_constructor()
 * and this pass only writes state, so the two can never disagree about where
 * the walk stops: bounce_collision_tile_overlaps() and
 * bounce_collision_query_level() resolve the same tile through the same
 * tile_has_static_collision() with the same masks and the same thorn records.
 *
 * REPRODUCED, the A1 collision state only:
 *   f.java:437  tile 1 blocking            -> u
 *   f.java:440  tile 1 mask miss           -> u
 *   f.java:444  tile 2 blocking            -> v, and NOT u (:446 breaks first)
 *   f.java:448  tile 2 mask miss           -> u
 *   f.java:452  tiles 34-37 blocking       -> slope response, v, u
 *   f.java:460  tiles 30-33 blocking       -> slope response, u
 *   f.java:618-621 tiles 39-42             -> deflate
 *
 * NOT REPRODUCED, deliberately, per the deferral list: hoops, crystals, the
 * extra life, the gravity/jump/speed power-ups, KillBall, level completion,
 * TODO_somePowerUp1, audio, and every LevelTiles rewrite.  Two consequences
 * are worth stating plainly:
 *
 *   - f.java:427-428 short-circuits every tile once z == 2.  KillBall is not
 *     reproduced, so z cannot become 2 here; the guard is kept because it is
 *     the source's own and because this function's result is not used as a
 *     verdict.
 *   - Interaction tiles that the source blocks WITHOUT any geometry test --
 *     tile 38 at f.java:641-645, tiles 51-54 at :636-640, and the
 *     gravity power-up at :630-635 -- are not treated as blocking here, which
 *     matches the pure query this walk has always used and keeps the existing
 *     walk geometry unchanged.  Their side effects are deferred, so the only
 *     cost is that the walk passes through them, exactly as before.
 */
static void player_constructor_apply_collision_state(
    BouncePlayer *player,
    const BounceRuntimeLevelTiles *view,
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    BouncePlayerHazardCallback on_hazard,
    void *hazard_context
)
{
    int32_t first_tile_x;
    int32_t first_tile_y;
    int32_t end_tile_x;
    int32_t end_tile_y;
    uint32_t width;
    uint32_t height;
    int32_t tile_x;
    int32_t tile_y;

    if (player == NULL || view == NULL)
        return;

    width = bounce_runtime_level_tiles_width(view);
    height = bounce_runtime_level_tiles_height(view);
    /* f.java:155-158. C signed division truncates toward zero, as Java's
     * int division does, so the plain operators are the transcription. */
    first_tile_x = (player_x - half_size) / BOUNCE_PLAYER_TILE_SIZE;
    first_tile_y = (player_y - half_size) / BOUNCE_PLAYER_TILE_SIZE;
    end_tile_x = ((player_x - 1 + half_size) / BOUNCE_PLAYER_TILE_SIZE) + 1;
    end_tile_y = ((player_y - 1 + half_size) / BOUNCE_PLAYER_TILE_SIZE) + 1;

    for (tile_x = first_tile_x; tile_x < end_tile_x; tile_x++) {
        /* f.java:425-426: an out-of-range coordinate returns false before the
         * switch, so it produces no state change either. */
        if (tile_x < 0 || (uint32_t)tile_x >= width)
            return;
        for (tile_y = first_tile_y; tile_y < end_tile_y; tile_y++) {
            uint16_t value;
            uint32_t tile_id;
            bool wall_tile;

            if (tile_y < 0 || (uint32_t)tile_y >= height)
                return;
            if (bounce_runtime_level_tiles_get(
                    view, (int)tile_x, (int)tile_y, &value) != 0)
                return;
            /* f.java:430-431. */
            tile_id = (uint32_t)value;
            tile_id &= ~UINT32_C(0x40);
            tile_id &= ~UINT32_C(0x80);
            wall_tile = tile_id == 1u || tile_id == 2u;

            if (bounce_collision_tile_overlaps(
                    player_x, player_y, half_size, tile_x, tile_y, view)) {
                if (tile_id >= 30u && tile_id <= 37u) {
                    /* f.java:450-461. c() runs the l/o rewrite first, inside its
                     * own !m guard, and only on the blocking path. */
                    if (!player->m)
                        player_apply_slope_response(player, tile_id);
                    if (tile_id >= 34u)
                        player->v = true;
                    player->u = true;
                } else if (tile_id >= 3u && tile_id <= 6u) {
                    /*
                     * STEP 38-P. f.java:474-478, the four STATIC thorns:
                     *   case TileIDs.THORNS_UP, _RIGHT, _DOWN, _LEFT -> {
                     *       if (b(x, y, tileY, tileX, tileId)) {
                     *           bool = false;
                     *           KillBall();
                     *
                     * The b() test is NOT restated here: the enclosing
                     * bounce_collision_tile_overlaps() at :402 already dispatched
                     * this tile through java_interaction_collision()'s 3/4/5/6
                     * shape cases, which are exactly f.b() (collision_query.c).
                     * Entering this arm therefore means the source's own condition
                     * held, the same argument the two existing app-level call
                     * sites make at vertical_slice.c:3092-3099.
                     *
                     * PLACEMENT. This is the `else` of the slope arm, so slopes
                     * (30-37) keep priority, and within the arm it precedes the
                     * wall and deflater writes, mirroring the source's own switch
                     * order: 3-6 and 10 sit at f.java:474 and :463, both BEFORE
                     * the deflater at :618 and the ordinary wall cases at :434-449.
                     * Nothing below is reached for a hazard because the loop
                     * returns at :437 either way.
                     *
                     * REPORTED, NOT PERFORMED. f.KillBall() writes lives, the HUD
                     * dirty flag and an audio event, all of which are BounceApp
                     * state this module does not own and cannot see. The hazard is
                     * therefore handed to the owner, which runs the single
                     * existing app_kill_ball(). No lives decrement is duplicated
                     * here.
                     */
                    if (on_hazard != NULL) {
                        on_hazard(
                            hazard_context,
                            tile_x,
                            tile_y,
                            tile_id
                        );
                    }
                } else if (tile_id == 10u) {
                    /*
                     * STEP 38-P. f.java:463-473, the DYNAMIC thorn:
                     *   case 10 -> {
                     *       k = this.n.TestPointInsideDynThorns(tileX, tileY);
                     *       if (k != -1) {
                     *           <the moving box from DynThornsBottomLeft[k], w[k]>
                     *           if (<9-arg box overlap>) {
                     *               bool = false;
                     *               KillBall();
                     *
                     * BOTH conditions are already evaluated by the enclosing
                     * test. java_interaction_collision()'s `case 10` calls
                     * java_dyn_thorn_overlaps(), which performs the record lookup
                     * AND the moving-box overlap, and bounce_collision_tile_overlaps()
                     * builds that thorns source from the runtime
                     * (collision_query.c:811-850). So a raw tile id of 10 is NOT
                     * sufficient on its own: an inactive or non-overlapping
                     * dynamic thorn returns false and never reaches this arm. No
                     * second classification is invented.
                     */
                    if (on_hazard != NULL) {
                        on_hazard(
                            hazard_context,
                            tile_x,
                            tile_y,
                            tile_id
                        );
                    }
                } else if (tile_id == 7u
                    || (tile_id >= 13u && tile_id <= 16u)
                    || (tile_id >= 21u && tile_id <= 24u)
                    || tile_id == 29u) {
                    /*
                     * STEP 38-Q. The three CONSUMED pickup classes, reachable from
                     * this walk exactly as they are from ordinary play.
                     *
                     * In the original the walk's verdict helper is
                     * f.TODO_CheckBallCollision (f.java:154-166), which calls f.a()
                     * (f.java:164) for every candidate tile, and f.a() is the FULL
                     * dispatcher. Three of its cases consume the tile:
                     *
                     *   f.java:480-486  case TileIDs.CRYSTAL (7)
                     *   f.java:487-580  the hoop family 13-16 and 21-24
                     *   f.java:609-618  case 29, the extra-life power up
                     *
                     * each ending in a LevelTiles write. So a walk candidate
                     * landing on one of them consumes it, and not reporting it here
                     * is what STEP 38-Q identified as the remaining divergence.
                     *
                     * ONLY consumed ids appear in this condition. The ids that set
                     * a state field but write no tile -- 9, 47-50, 51-54 and 38 --
                     * are deliberately ABSENT: they are not consumed, they stay
                     * collidable on every later probe, and reporting them would
                     * fabricate a consumption f.a() never performs. 39-42 (the
                     * deflater) and 43-46 (the pumper) are absent for the same
                     * reason and keep their own existing arms below.
                     *
                     * The predicate is the tile id ALONE, matching the source: none
                     * of these three cases is gated on a further b()/a() test
                     * beyond the overlap this block already established, and none
                     * assigns bool = false. The enclosing
                     * bounce_collision_tile_overlaps() at :402 is the same gate the
                     * ordinary path uses, so both routes agree on WHEN.
                     *
                     * REPORTED, NOT PERFORMED, for the same ownership reason as the
                     * hazard arms: consumption writes score, lives, checkpoint and
                     * audio state and mutates LevelTiles, none of which this module
                     * owns, and the `view` it holds is const. The owner runs the
                     * existing consumption logic; nothing is duplicated here.
                     *
                     * ORDERING. This sits inside the same overlap block as the 38-P
                     * hazard arms, so the effect fires at the same per-candidate
                     * evaluation boundary rather than after the whole six-candidate
                     * walk, and the function returns immediately afterwards, so at
                     * most one effect is produced per candidate.
                     *
                     * STEP 38-R CORRECTION to the paragraphs above. They described
                     * this arm as covering "the hoop family 13-16 and 21-24", which
                     * is right, but they also listed 9 among the ids "deliberately
                     * ABSENT" on the grounds that it writes no tile. That reasoning is
                     * correct for CONSUMPTION and wrong as a reachability statement:
                     * f.a()'s `case 9` (f.java:599-608) sets `this.n.e` when
                     * `this.n.M`, and e.java:313 later consumes `e` as the
                     * level-completion trigger. Tile 9 is not consumed, yet the walk
                     * does reach it. The arm below adds 22 and 9 for that reason, and
                     * neither consumes anything.
                     */
                    if (on_hazard != NULL) {
                        on_hazard(
                            hazard_context,
                            tile_x,
                            tile_y,
                            tile_id
                        );
                    }
                } else if (tile_id == 22u
                    || tile_id == 9u) {
                    /*
                     * STEP 38-R -- the two arms Java's walk reaches that neither
                     * STEP 38-P nor STEP 38-Q covered. Both are REPORTED, neither is
                     * consumed, and they are distinct mechanisms grouped only because
                     * they share the report.
                     *
                     * TILE 22, the NON-BLOCKING hoop. f.java:561-568:
                     *     case 22 -> {
                     *         if (b(x, y, tileY, tileX, tileId)) {
                     *             OnTouchHoop();
                     *             this.n.LevelTiles[tileY][tileX]     = 0x9A | isInWater;
                     *             this.n.LevelTiles[tileY - 1][tileX] = 0x99 | isInWater;
                     *             sound = this.n.soundUp;
                     *
                     * The seven ids 13,14,15,16,21,23,24 sit at f.java:487-546 and
                     * :569-580, and each assigns `bool = false; break;` BEFORE calling
                     * OnTouchHoop(). Tile 22 assigns NO bool at all, so the original
                     * walk CONTINUES past this tile. That distinction is precisely why
                     * native keeps it at its own site (vertical_slice.c:3833) rather
                     * than in the blocking gate, and it is preserved here: this arm
                     * reports and does nothing else, introduces no blocking verdict,
                     * and the enclosing block's existing return still ends the scan for
                     * this candidate exactly as for every other tile. Tile 22 is NOT
                     * made equivalent to the blocking hoop ids.
                     *
                     * TILE 9, the exit door. f.java:599-608:
                     *     case 9 -> {
                     *         if (b(x, y, tileY, tileX, tileId)) {
                     *             if (this.n.M) { this.n.e = true; sound = ...; break; }
                     *             bool = false;
                     *
                     * Like tile 22 this writes no tile, so it is not consumed; what
                     * it does is set the completion trigger `e`, which e.java:313
                     * consumes. The door-open gate is the owner's business and is
                     * applied there, exactly as the ordinary tile-9 site at
                     * vertical_slice.c:3169 does.
                     *
                     * REPORTED, NOT PERFORMED, for the ownership reason above: the
                     * hoop rewrite and the completion trigger are both app-owned.
                     */
                    if (on_hazard != NULL) {
                        on_hazard(
                            hazard_context,
                            tile_x,
                            tile_y,
                            tile_id
                        );
                    }
                } else {
                    if (tile_id == 1u)
                        player->u = true;
                    if (tile_id == 2u)
                        player->v = true;
                    if (tile_id >= 39u && tile_id <= 42u
                        && player->ballSize
                            == BOUNCE_PLAYER_BIG_BALL_SIZE) {
                        /* f.java:618-621 calling f.java:209-213. The source
                         * guards on ballSize == 16, so the search deflates at
                         * most once; p changes mid-walk and every later
                         * candidate then uses the regular half size, which is
                         * why the walk passes player->p rather than a literal. */
                        player->ballSize = BOUNCE_PLAYER_REGULAR_BALL_SIZE;
                        player->p = BOUNCE_PLAYER_REGULAR_HALF_SIZE;
                        player->current_ball_mode =
                            BOUNCE_PLAYER_BALL_MODE_REGULAR;
                    }
                    if (tile_id >= 43u && tile_id <= 46u) {
                        /*
                         * RE-ENTRANT PUMPER -- f.java:623-629, the PUMPER_UP /
                         * _RIGHT / _DOWN / _LEFT case:
                         *
                         *   623  case TileIDs.PUMPER_UP, _RIGHT, _DOWN, _LEFT -> {
                         *   624      if (b(x, y, tileY, tileX, tileId)) {
                         *   625          bool = false;
                         *   626          if (this.ballSize == 12)
                         *   627              UseBigBall();
                         *   628      }
                         *   629  }
                         *
                         * RE-ENTRANCE IS THE POINT, and it is genuine recursion in
                         * the source: the nested UseBigBall() at :627 is invoked
                         * from inside a(), i.e. BEFORE a() returns. The `bool =
                         * false` on :625 only propagates as a()'s return value; it
                         * does not prevent the call on the next line.
                         *
                         * ORDERING, matching :625 then :627. The blocking verdict
                         * is already established: the enclosing
                         * bounce_collision_tile_overlaps() at :402 is this arm's
                         * b() gate (b() is the shape test, collision_query.c), and
                         * the enclosing block's existing `return` at :616 is
                         * a()'s `bool = false`. So the verdict is set first and
                         * the growth runs synchronously to completion before that
                         * return, exactly as the source orders them. Nothing is
                         * deferred or queued: the walk that :627 performs finishes
                         * inside this call.
                         *
                         * WHY THIS LIVES HERE AND NOT IN THE APP MODULE. In Java
                         * UseBigBall() and a() are methods of the same object f, so
                         * the nested call is pure self-recursion with no ownership
                         * transfer. bounce_player_use_big_ball() is likewise
                         * player-owned -- it is defined below in this file and
                         * declared in player.h -- so this arm is a same-file call
                         * and introduces no compile dependency on
                         * vertical_slice.c, no second growth implementation, and no
                         * new callback. The app-side pumper arm at
                         * vertical_slice.c:3769 calls the same entry and is left
                         * exactly as it was.
                         *
                         * RECURSION BOUND IS THE SOURCE'S OWN. The guard on
                         * ballSize == 12 is f.java:626, and
                         * bounce_player_use_big_ball_probe() sets ballSize = 16
                         * before its own walk, so a second pumper hit inside the
                         * nested walk does not re-enter -- identical to the source.
                         * The f.java:618-621 deflate above sets ballSize back to 12
                         * and therefore re-arms it, again as the source does. No
                         * recursion counter is added because the source has none.
                         *
                         * LIVENESS IS THE OTHER HALF. The nested walk writes
                         * player->TODO_unkX/Y before returning; the live per-
                         * candidate re-reads in bounce_player_use_big_ball_probe()
                         * are what let this walk's next candidate observe that
                         * result instead of a stale pre-walk copy. See
                         * RE-ENTRANT-PUMPER-TODO-LIVENESS-AUDIT.md.
                         *
                         * ERROR HANDLING. bounce_player_use_big_ball() returns -1
                         * only for a NULL player, NULL runtime or an uninitialised
                         * player. The walk is entered only from a live player, so
                         * that cannot occur here; the return is deliberately not
                         * branched on, matching how the sibling deflater arm above
                         * handles its own state writes.
                         */
                        if (player->ballSize == BOUNCE_PLAYER_REGULAR_BALL_SIZE)
                            (void)bounce_player_use_big_ball(
                                player,
                                view,
                                on_hazard,
                                hazard_context
                            );
                    }
                }
                return;
            }
            /* f.java:440 and f.java:448: both ordinary wall cases assign u
             * when their mask misses, and f.java:442-449 is reached without
             * ever testing the tile's identity again. */
            if (wall_tile)
                player->u = true;
        }
    }
}

static bool player_probe_constructor(
    int32_t x,
    int32_t y,
    int32_t half_size,
    const void *context
)
{
    const PlayerConstructorProbeContext *probe_context = context;
    bool fits;

    if (probe_context == NULL
        || probe_context->player == NULL
        || probe_context->level == NULL
        || probe_context->view == NULL)
        return false;

    /* The verdict is the unchanged pure level query, so the six-candidate walk
     * keeps exactly the geometry it had before this side-effecting pass
     * existed. */
    fits = bounce_collision_query_level(
        x, y, half_size, probe_context->level);
    /* STEP 38-P: the constructor-boundary form is hazard-inert. At e.java:127 the
     * original runs the probe inside `new f(...)`, where there is no app-level
     * death state to write, so no callback is supplied and the two hazard arms
     * are simply not taken. */
    player_constructor_apply_collision_state(
        probe_context->player,
        probe_context->view,
        x,
        y,
        half_size,
        NULL,
        NULL
    );
    return fits;
}

/* STEP 14B-02 context for the production f.UseBigBall() resolver.
 *
 * f.java:175-206 drives the six-direction resolver through
 * TODO_CheckBallCollision() (f.java:154-166), and that method is NOT a pure
 * query: its inner a(x, y, tileY, tileX) at f.java:161 dispatches into the
 * same per-tile walk the constructor already uses, so every probe may mutate
 * the player before the resolver reads the verdict.
 *
 * The verdict itself stays pure, exactly as it always has. Only the side
 * effects are added, and they are the ones player_constructor_apply_collision_state()
 * already implements and which an earlier milestone verified: the u and v
 * producers of f.java:437/440/444/448/452/460 and the Deflater p-shrink of
 * f.java:618-621. Reusing that function is deliberate -- it is the same walk,
 * the same tile order (f.java:159-163), the same f.java:155-158 window, the
 * same f.java:430-431 masking, and it already reads player->p live so a
 * Deflater struck mid-walk widens the window for later candidates, which is
 * the defect class STEP 13G-U closed on the main path.
 *
 * No level pointer is needed: the production caller already owns a runtime tile
 * view, so the verdict is taken with bounce_collision_query() -- the same pure
 * function the pre-change resolver used, through player_probe_runtime() below,
 * which is left untouched and still called.
 */
typedef struct PlayerUseBigBallProbeContext {
    BouncePlayer *player;
    const BounceRuntimeLevelTiles *view;
    /* STEP 38-P: the reported hazard and its owner, for the two KillBall cases
     * of f.a(). NULL for the constructor-boundary form. */
    BouncePlayerHazardCallback on_hazard;
    void *hazard_context;
} PlayerUseBigBallProbeContext;

/* STEP 14B-02: the production resolver probe. Order matters and mirrors
 * player_probe_constructor() at pc:440 -- verdict first, then the walk -- so
 * the resolver's candidate acceptance cannot be perturbed by the side effects
 * it is being given. */
static bool player_probe_use_big_ball(
    int32_t x,
    int32_t y,
    int32_t half_size,
    const void *context
)
{
    const PlayerUseBigBallProbeContext *probe_context = context;
    bool fits;

    if (probe_context == NULL
        || probe_context->player == NULL
        || probe_context->view == NULL)
        return false;

    /* The verdict is the unchanged pure runtime query, so the six-candidate
     * walk keeps exactly the geometry it had before this side-effecting pass
     * existed. */
    fits = player_probe_runtime(x, y, half_size, probe_context->view);
    /* STEP 38-P: the hazard the owner supplied, forwarded. This is the production
     * pumper walk, so the callback is the app-level kill operation. */
    player_constructor_apply_collision_state(
        probe_context->player,
        probe_context->view,
        x,
        y,
        half_size,
        probe_context->on_hazard,
        probe_context->hazard_context
    );
    return fits;
}

static int bounce_player_use_big_ball_probe(
    BouncePlayer *player,
    BouncePlayerFreeSpaceProbe probe,
    const void *context
)
{
    int32_t x;
    int32_t y;
    int32_t offset;

    if (player == NULL
        || probe == NULL
        || context == NULL
        || !player->initialized)
        return -1;

    /* f.UseBigBall(): ballSize 16, p 8, big-ball sprite. No native sprite
     * asset is selected here; sprite selection is a presentation concern. */
    player->ballSize = 16;
    player->p = 8;
    player->current_ball_mode = BOUNCE_PLAYER_BALL_MODE_BIG;

    /* RE-ENTRANT PUMPER -- the walk no longer snapshots the coordinates here.
     *
     * f.UseBigBall() reads this.TODO_unkX / this.TODO_unkY as fields inside each
     * of its six candidate expressions (f.java:177, :181, :186, :191, :195,
     * :200); there is no equivalent of the `x = player->TODO_unkX` pair that stood
     * here. Each candidate now re-reads the live fields just before its probe, so
     * the initialisation that used to live at this point is unnecessary and a
     * retained copy would be dead. `offset` is still declared and still local,
     * because the source's loop variable is a local too (f.java:175). */

    /*
     * f.UseBigBall() (f.java:175-206), transcribed with its structure intact:
     * a signed byte offset starting at 1, six candidates in the original
     * order, the first free candidate accepted, and a byte-wrapped offset that
     * keeps the original's unbounded termination behavior.
     *
     * STEP 43B -- the two loop exits were transposed here. f.java:175 is a
     * CONDITIONED loop, so the flag decides which path continues:
     *
     *   f.java:176   TODO_collidesWithLevel = true at the top of every pass.
     *   f.java:179   a candidate fits -> the coordinate is mutated and
     *   :184/:189   `continue` runs the update expression offset++, after which
     *   :193/:198   the f.java:175 condition !true is false. The walk therefore
     *   :203        ENDS on the first free candidate.
     *   f.java:205   TODO_collidesWithLevel = false is reached only when all six
     *                candidates at this offset are blocked; !false is then true,
     *                so the walk CONTINUES at the next byte offset.
     *
     * STEP 43A observed the reverse: at the pumper the offset-1 pass was fully
     * blocked and this loop exited with displacement (+0, +0), leaving the grown
     * ball inside the pumper tile it had just fired, even though offset 2 held a
     * free up-right candidate. Only the exit/continue assignment is corrected
     * below; the candidate order, the byte wrap, player->p, the probe, and the
     * unbounded termination behavior are unchanged.
     *
     * No iteration cap and no early exit is added, because the original has
     * none; a ball that is fully enclosed therefore never terminates here
     * either.
     *
     * The candidate coordinates use the byte offset, while the footprint size is
     * f.p, read live inside TODO_CheckBallCollision at f.java:155-158. Passing
     * player->p rather than a literal is what lets the f.java:620-621 deflate
     * change the half size for every later candidate instead of only changing
     * the recorded state.
     */
    for (offset = 1;; offset = (int32_t)(int8_t)((uint8_t)offset + 1u)) {
        /*
         * RE-ENTRANT PUMPER -- live candidate coordinates, f.java:177-203.
         *
         * The source has no positional locals. Every one of the six candidate
         * expressions reads this.TODO_unkX / this.TODO_unkY as a FIELD at the
         * moment that candidate is evaluated:
         *
         *   f.java:177  TODO_CheckBallCollision(this.TODO_unkX, this.TODO_unkY - offset)
         *   f.java:181  ... (this.TODO_unkX - offset, this.TODO_unkY - offset)
         *   :186        ... (this.TODO_unkX + offset, this.TODO_unkY - offset)
         *   :191        ... (this.TODO_unkX, this.TODO_unkY + offset)
         *   :195        ... (this.TODO_unkX - offset, this.TODO_unkY + offset)
         *   :200        ... (this.TODO_unkX + offset, this.TODO_unkY + offset)
         *
         * and each accepted candidate writes the field on the following line
         * (:178, :182-183, :187-188, :192, :196-197, :201-202). The next
         * candidate therefore observes the coordinate the previous one accepted.
         *
         * That per-candidate re-read is what makes the nested UseBigBall() of
         * f.java:627 correct: the nested walk mutates this.TODO_unkX/Y before it
         * returns, and this walk's NEXT candidate must see that result. The
         * earlier local snapshot could not -- it both mis-based the nested walk
         * on the pre-walk position and then clobbered the nested result at the
         * single write-back. See RE-ENTRANT-PUMPER-TODO-LIVENESS-AUDIT.md.
         *
         * ORDINARY WALKS ARE UNCHANGED. Nothing reachable from this walk writes
         * TODO_unkX/Y except an accepted candidate: player_probe_runtime() is a
         * pure tile read, player_constructor_apply_collision_state() performs no
         * position write, app_pumper_walk_hazard() performs none, and
         * app_kill_ball() writes only q / z / the power-up fields. With no nested
         * UseBigBall() the field and the removed locals were provably identical
         * at every instant, so this is the same walk with one read moved from
         * loop entry to each candidate.
         *
         * x and y are retained solely as this function's working registers for
         * the accepted displacement; they are refreshed from the live fields
         * before every probe so a nested walk's result is never lost.
         */
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x, y - offset, player->p, context)) {
            y -= offset;
            player->TODO_unkY = y;
            break;
        }
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x - offset, y - offset, player->p, context)) {
            x -= offset;
            y -= offset;
            player->TODO_unkX = x;
            player->TODO_unkY = y;
            break;
        }
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x + offset, y - offset, player->p, context)) {
            x += offset;
            y -= offset;
            player->TODO_unkX = x;
            player->TODO_unkY = y;
            break;
        }
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x, y + offset, player->p, context)) {
            y += offset;
            player->TODO_unkY = y;
            break;
        }
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x - offset, y + offset, player->p, context)) {
            x -= offset;
            y += offset;
            player->TODO_unkX = x;
            player->TODO_unkY = y;
            break;
        }
        x = player->TODO_unkX;
        y = player->TODO_unkY;
        if (probe(x + offset, y + offset, player->p, context)) {
            x += offset;
            y += offset;
            player->TODO_unkX = x;
            player->TODO_unkY = y;
            break;
        }
        /* Every candidate at this offset is blocked: f.java:205 clears the flag,
         * so the f.java:175 condition stays true and the walk continues at the
         * next byte offset rather than ending here. */
    }

    return 0;
}

/* STEP 14B-02: f.UseBigBall() (f.java:168-207) is reached from the object walk
 * at f.java:627, so the resolver's own probes are the same side-effecting
 * f.java:154-166 walk. Before this change the production resolver used the pure
 * player_probe_runtime() alone and therefore dropped the u, v and p mutations
 * that Java performs while it is growing the ball.
 *
 * The verdict is still player_probe_runtime() (player_probe_use_big_ball calls
 * it), so the resolver's geometry, candidate order, offset wrap and termination
 * are all unchanged. Only the side effects are new. */
int bounce_player_use_big_ball(
    BouncePlayer *player,
    const BounceRuntimeLevelTiles *runtime,
    BouncePlayerHazardCallback on_hazard,
    void *hazard_context
)
{
    PlayerUseBigBallProbeContext probe_context;

    if (player == NULL || runtime == NULL)
        return -1;

    probe_context.player = player;
    probe_context.view = runtime;
    probe_context.on_hazard = on_hazard;
    probe_context.hazard_context = hazard_context;
    return bounce_player_use_big_ball_probe(
        player,
        player_probe_use_big_ball,
        &probe_context
    );
}

int bounce_player_use_big_ball_level(
    BouncePlayer *player,
    const BounceLevel *level
)
{
    PlayerConstructorProbeContext probe_context;
    BounceRuntimeLevelTiles *view;
    int status;

    if (player == NULL || level == NULL)
        return -1;

    view = bounce_runtime_level_tiles_create(level);
    if (view == NULL)
        return -1;

    probe_context.player = player;
    probe_context.level = level;
    probe_context.view = view;
    status = bounce_player_use_big_ball_probe(
        player,
        player_probe_constructor,
        &probe_context
    );
    bounce_runtime_level_tiles_destroy(view);
    return status;
}
