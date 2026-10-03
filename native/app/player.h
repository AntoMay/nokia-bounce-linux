#ifndef BOUNCE_NATIVE_APP_PLAYER_H
#define BOUNCE_NATIVE_APP_PLAYER_H

#include <stdbool.h>
#include <stdint.h>

#include "../level/level.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Source-shaped mutable player state used by the existing native physics
 * seam.  The field names intentionally retain the recovered Java names whose
 * meanings are not all known.  This structure owns no level, runtime tile,
 * renderer, asset, or scene object.
 */
/*
 * f.b() frame state at the pre-physics stop point.
 *
 * ORIGINAL VERIFIED, f.java:653-669, in source order:
 *   :653  int i = this.TODO_unkX;          -> i
 *   :654  int j = 0;                        -> j
 *   :655  int k = 0;                        -> k
 *   :656  byte b1 = 0;                      -> b1  (Java byte is SIGNED)
 *   :657  boolean bool1 = false;            -> bool1
 *   :658  if (this.z == 2) { ... return; }  -> z2_early_return
 *   :667  int tileX = this.TODO_unkX / 12;  -> tileX
 *   :668  int tileY = this.TODO_unkY / 12;  -> tileY
 *   :669  boolean isInWater =
 *             (this.n.LevelTiles[tileY][tileX] & 0x40) != 0;  -> is_in_water
 *
 * Every one of these is a local initialization, a field read, or the water
 * sample. The only mutations in that span (f.java:659-663) sit inside the
 * z == 2 early-return branch, which the normal path does not take. The first
 * mutation reachable in the normal path is `this.o = -10` at f.java:675, a
 * vertical-velocity write, so :669 is the last pre-physics point and this
 * record stops there.
 *
 * `this.n.LevelTiles` is surrounding game/runtime state, not player state; the
 * native equivalent is the mutable BounceRuntimeLevelTiles owned by the game.
 */
typedef struct BouncePlayerTickEntry {
    bool entered;         /* the f.b() frame was entered at all */
    bool z2_early_return; /* f.java:658-666: the z == 2 branch was taken */
    int32_t i;            /* f.java:653 */
    int32_t j;            /* f.java:654 */
    int32_t k;            /* f.java:655 */
    int8_t b1;            /* f.java:656, signed Java byte */
    bool bool1;           /* f.java:657 */
    int32_t tileX;        /* f.java:667 */
    int32_t tileY;        /* f.java:668 */
    bool is_in_water;     /* f.java:669 */
} BouncePlayerTickEntry;

typedef struct BouncePlayer {
    int32_t TODO_unkX;
    int32_t TODO_unkY;
    int32_t ballSize;
    int32_t p;
    int32_t horizontal_velocity; /* Java l */
    int32_t o;                   /* Java vertical velocity */
    int32_t t;                   /* Java gravity-related state */
    int32_t powerUpGravity;
    /*
     * STEP: f.TODO_somePowerUp1 (f.java:34), declared between powerUpGravity and
     * TODO_somePowerUp3 to keep the recovered field order.
     *
     * The tile-38 speed power-up. The source has exactly six sites:
     *   f.java:34   declaration
     *   f.java:120  constructor zero
     *   f.java:642  producer, `case 38:` in f.a() -> = 300
     *   f.java:220  cleared by KillBall()
     *   f.java:800  consumer: != 0 selects the horizontal limit 100 over 50
     *   f.java:802  the per-tick decrement
     * plus two persistence slots (BounceGame.java:377 write, e.java:108 read)
     * which are out of scope while no store exists.
     *
     * The producer is app_vertical_collision_info() in vertical_slice.c and the
     * consumer is app_step_horizontal(), mirroring the gravity and
     * TODO_somePowerUp3 power-ups. The verdict half lives in the pure
     * java_interaction_collision() switch in collision_query.c.
     */
    int32_t TODO_somePowerUp1;
    int32_t TODO_somePowerUp3;
    int32_t C;
    int32_t z;
    int32_t q;
    bool m;
    bool u; /* f.a() contact flag; not the blocked-collision discriminator */
    /*
     * STEP 14.1 A1: f.v, the rubber-wall latch (f.java:42).
     *
     * A LATCH, not a per-tick flag. The source has exactly five `this.v` sites:
     * the declaration, the constructor zero at f.java:117, the two producers at
     * f.java:444 (tile 2) and f.java:452 (tiles 34-37), and the A1 read/clear at
     * f.java:754-755. The ONLY clear is inside A1 and is reachable only when the
     * A1 guard already held, so once set the latch survives every tick, survives
     * death (KillBall, f.java:215-226, does not touch it), and is cleared only
     * by an A1 hit.
     *
     * It is therefore NOT cleared per tick and NOT cleared on respawn, which is
     * why bounce_player_reset() -- a full memset, reached only from the
     * constructor path and the player-state transition, never from the tick --
     * is the correct and sufficient initializer. See vertical_slice.c
     * app_vertical_collision_info() for the producer and app_step_vertical()
     * for the A1 consumer.
     */
    bool v;
    BouncePlayerBallMode current_ball_mode;
    bool initialized;
    /* f.b() frame state at the pre-physics stop point; see below. */
    BouncePlayerTickEntry tick_entry;
} BouncePlayer;


/*
 * The source establishes a new-game player's header-derived spawn.  Keeping
 * this record separate from the mutable player makes the respawn boundary
 * explicit without implementing an unverified respawn policy.
 */
typedef struct BouncePlayerSpawn {
    bool initialized;
    int32_t world_x;
    int32_t world_y;
    int32_t tile_x;
    int32_t tile_y;
    int32_t ball_size;
    int32_t half_size;
    BouncePlayerBallMode ball_mode;
} BouncePlayerSpawn;

/* Reset only player-owned state; safe with NULL. */
void bounce_player_reset(BouncePlayer *player);

/*
 * f.b() entry through the pre-physics stop point (f.java:652-669).
 *
 * It performs only the local initializations, the `z == 2` test, the
 * division-toward-zero tile coordinates, and the water sample, then STOPS. It
 * applies no gravity, no velocity change, no movement, no collision, and no
 * power-up, death, respawn, score, audio, or rendering effect.
 *
 * When player->z == 2 the source takes the early return at f.java:658-666
 * WITHOUT reaching :667-669, so this function likewise skips the tile
 * coordinates and the water sample and only records the branch. The mutations
 * in that branch (q--, z = 1, n.e = true) are gameplay side effects and are
 * deliberately NOT performed.
 *
 * NATIVE BOUNDARY: the source indexes this.n.LevelTiles directly and would
 * raise an uncaught ArrayIndexOutOfBoundsException for an out-of-range tile.
 * This function rejects such a coordinate instead of reproducing a crash.
 *
 * entry_out is reset before validation. Returns 0 on a recorded entry frame
 * (including the z == 2 early return), -1 for NULL arguments, a non-initialized
 * player, a NULL runtime, or an out-of-range tile.
 */
int bounce_player_tick_entry(
    const BouncePlayer *player,
    const BounceRuntimeLevelTiles *runtime,
    BouncePlayerTickEntry *entry_out
);

/*
 * Initialize the player and its spawn record from a parsed level header.
 * The conversion is the existing verified initial_X/initial_Y/isBigBall
 * sequence; no level or runtime-tile ownership is transferred.
 */
int bounce_player_initialize_from_level(
    const BounceLevel *level,
    BouncePlayer *player_out,
    BouncePlayerSpawn *spawn_out
);

/*
 * STEP 38-P -- the hazard half of f.KillBall() inside the big-ball growth walk.
 *
 * WHY A CALLBACK. In the original the walk's verdict helper is
 * f.TODO_CheckBallCollision (f.java:154-166), which calls f.a() (f.java:164) for
 * every candidate tile, and f.a() is the FULL per-tile dispatcher. Two of its
 * cases call KillBall():
 *
 *   f.java:474-478  case THORNS_UP/RIGHT/DOWN/LEFT (3-6) -> b() -> KillBall()
 *   f.java:463-473  case 10 -> TestPointInsideDynThorns + moving-box -> KillBall()
 *
 * This module owns BouncePlayer and has no knowledge of BounceApp, so it cannot
 * perform an operation whose payload is app-owned state (lives, the HUD dirty
 * flag, the audio event). Reporting the hazard OUT through a callback, supplied
 * by the caller that does own BounceApp, keeps the ownership exactly as it was:
 * this direction (app -> player) already exists and is unchanged; the reverse
 * edge is not introduced.
 *
 * This is the same one-indirection shape as BouncePlayerFreeSpaceProbe, for the
 * same reason: the algorithm exists once and the owner supplies the effect.
 *
 * CALLED AT MOST ONCE PER CANDIDATE, and only for the two hazard cases above. The
 * caller passes the existing app-level kill operation; this module neither
 * duplicates it nor decrements lives.
 *
 * STEP 38-Q EXTENDS THIS CONTRACT, without changing its shape. The walk also
 * reaches the three CONSUMED pickup classes of f.a() -- the crystal (7), the hoop
 * family (13-16 and 21-24) and the extra life (29) -- and reports them through the
 * SAME callback. No second callback and no extra parameter is needed: the reported
 * tile id discriminates the two classes disjointly, since {3,4,5,6,10} and
 * {7,13,14,15,16,21,22,23,24,29} do not overlap, so the owner selects the operation
 * from tile_id. The 38-P hazard semantics above are unchanged.
 *
 * The ids that set a state field but write no tile -- 9, 47-50, 51-54, 38 -- are
 * NOT reported, because they are not consumed and stay collidable.
 *
 * A NULL callback is legal and makes the walk hazard- and pickup-inert, which is
 * what the constructor-boundary form bounce_player_use_big_ball_level() uses: at
 * e.java:127 the original runs the probe inside `new f(...)`, where no app-level
 * death or collection state exists to write.
 */
typedef void (*BouncePlayerHazardCallback)(
    void *context,
    int32_t tile_x,
    int32_t tile_y,
    uint32_t tile_id
);

/*
 * f.UseBigBall() (f.java:168-207): set the big-ball size, then walk the ball
 * with the original six-direction escalating byte-offset probe until every
 * candidate is blocked. Uses the pure collision query only; it performs no
 * physics, collision response, camera, timer, or persistence work and does not
 * select a sprite asset. The original has no iteration cap, and neither does
 * this: a fully enclosed ball does not terminate.
 *
 * STEP 38-P: on_hazard/hazard_context carry the two KillBall hazard cases of
 * f.a() that the walk reaches through TODO_CheckBallCollision. Pass NULL/NULL to
 * suppress them, which is what a caller with no app-owned death state does.
 */
int bounce_player_use_big_ball(
    BouncePlayer *player,
    const BounceRuntimeLevelTiles *runtime,
    BouncePlayerHazardCallback on_hazard,
    void *hazard_context
);

/*
 * The same f.UseBigBall() algorithm, evaluated against the pristine BounceLevel
 * tile bytes. This is the constructor-boundary form: at e.java:127 the original
 * runs the probe inside `new f(...)`, where LevelTiles has just been written by
 * LoadLevelId and no runtime copy exists yet.
 *
 * The walk's accept/reject decision is still the pure
 * bounce_collision_query_level(), so the six-candidate geometry, the byte-
 * wrapped offset, and the absence of an iteration cap are unchanged. Layered
 * outside that pure verdict, the probe now also reproduces the A1 collision
 * state the source produces on the way through, because in the original the
 * verdict helper is f.TODO_CheckBallCollision -> f.a() and f.a() mutates the
 * player:
 *
 *   u   f.java:437, :440, :448, :454, :460
 *   v   f.java:444 (tile 2 blocking), :452 (tiles 34-37 blocking)
 *   l/o f.java:361 -> f.java:234-270, under the f.java:360 !m guard
 *
 * plus the deflater transition at f.java:618-621, which is required for
 * correctness rather than for its own sake: it changes p from 8 to 6 mid-search
 * and f.java:155-158 reads f.p, so every later candidate is tested with the
 * regular half size.
 *
 * DEFERRED, deliberately: hoops, crystals, the extra life, the gravity/jump/
 * speed power-ups, KillBall, level completion, TODO_somePowerUp1, audio, and
 * every LevelTiles rewrite. The level is never written; the short-lived tile
 * view this uses is created and destroyed here and is never retained.
 */
int bounce_player_use_big_ball_level(
    BouncePlayer *player,
    const BounceLevel *level
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_PLAYER_H */
