#ifndef BOUNCE_NATIVE_LEVEL_H
#define BOUNCE_NATIVE_LEVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structural representation of the verified Nokia Bounce level binary.
 *
 * Header bytes 0..7 are preserved in their original order:
 * initial_X, initial_Y, isBigBall, exit_X, exit_Y, HoopsTotal, width, height.
 * The tile region is width * height row-major bytes. The final thorn_count
 * byte and its following 8 * thorn_count bytes are retained as opaque data;
 * the exact size is 8 + width * height + 1 + 8 * thorn_count. This module
 * intentionally does not decode thorn fields or coordinates.
 */

typedef struct BounceLevel BounceLevel;
typedef struct BounceRuntimeLevelTiles BounceRuntimeLevelTiles;

/*
 * Dynamic-thorn animation state, defined in dyn_thorns.h. Only an opaque
 * forward declaration is needed here because BounceRuntimeLevelTiles is itself
 * opaque to this header.
 */
struct BounceDynThorns;
/*
 * BounceVec2S itself is defined in dyn_thorns.h, which includes this header, so
 * only an incomplete type can be named here. The restore seam below takes
 * pointers to it and never dereferences one in this header's scope.
 */
struct BounceVec2S;

/*
 * Data-only record of the fields whose initialization is directly established
 * by the Java f constructor and its regular/big-ball helper. This is not a
 * native Player object and owns no level, sprite, owner, or collision data.
 */
/* Records only the verified regular/big helper branch; it owns no sprite. */
typedef enum BouncePlayerBallMode {
    BOUNCE_PLAYER_BALL_MODE_REGULAR = 0,
    BOUNCE_PLAYER_BALL_MODE_BIG = 1
} BouncePlayerBallMode;

typedef struct BouncePlayerConstructorState {
    int TODO_unkX;
    int TODO_unkY;
    int l;
    int o;
    int t;
    bool m;
    bool v;
    bool u;
    int q;
    int TODO_somePowerUp1;
    int powerUpGravity;
    int TODO_somePowerUp3;
    int C;
    int z;
    int w;
    int ballSize;
    int p;
    BouncePlayerBallMode current_ball_mode;
    bool j; /* Java field initializer is true; semantic role is UNKNOWN. */
} BouncePlayerConstructorState;

/*
 * Parse raw level bytes into a newly allocated structural level object.
 * The object copies its tile and thorn payload bytes and does not retain data.
 * The parser performs no file or resource I/O. Returns NULL for invalid,
 * truncated, trailing, overflowing, or allocation-failing input.
 */
BounceLevel *bounce_level_parse(
    const uint8_t *data,
    size_t size
);

/* Safe with NULL; releases all level-owned storage. */
void bounce_level_destroy(BounceLevel *level);

/* Header accessors return 0 for a NULL level. */
uint8_t bounce_level_initial_x(const BounceLevel *level);
uint8_t bounce_level_initial_y(const BounceLevel *level);
uint8_t bounce_level_is_big_ball(const BounceLevel *level);
uint8_t bounce_level_exit_x(const BounceLevel *level);
uint8_t bounce_level_exit_y(const BounceLevel *level);
uint8_t bounce_level_hoops_total(const BounceLevel *level);
uint32_t bounce_level_width(const BounceLevel *level);
uint32_t bounce_level_height(const BounceLevel *level);

/*
 * Derive only the new-game f-constructor inputs from this level:
 * initial tile X/Y become initial pixel X/Y using 12-pixel tiles and offset 6;
 * isBigBall == 0 derives ball size 12, otherwise 16. This does not construct
 * a player, perform collision probing, or restore resume/persisted state.
 * Output pointers are reset to zero before validation and all are required.
 * Returns -1 for a NULL level or output pointer.
 */
int bounce_level_derive_initial_player_input(
    const BounceLevel *level,
    int *player_x_out,
    int *player_y_out,
    int *ball_size_out
);

/*
 * Initialize only the data-only f-constructor state. The source calls
 * n.CreateTiles(this) before selecting the ball mode; no native equivalent of
 * that sprite/tile dependency is invoked here. The source's big-ball
 * collision probe is also not invoked or emulated, so TODO_unkX/TODO_unkY
 * remain the supplied constructor inputs even for the big-ball branch. l/o
 * are constructor defaults only; e's later paramInt4/paramInt5 assignments
 * are separate.
 * Returns -1 for a NULL state output, otherwise 0.
 */
int bounce_player_constructor_state_init(
    int param_x,
    int param_y,
    int ball_size,
    BouncePlayerConstructorState *state_out
);

/*
 * Apply only the two post-constructor assignments made by e.a(...):
 * aq.l = paramInt4 and aq.o = paramInt5. The caller supplies the values.
 * This does not perform camera/E-buffer setup, tile-coordinate assignment,
 * resume restoration, or any other player update. Returns -1 for NULL state.
 */
int bounce_player_post_constructor_apply(
    BouncePlayerConstructorState *state,
    int param4,
    int param5
);

/*
 * Apply only the source-verified f.a(int tileX, int tileY) assignments:
 * d = tileX, c = tileY, b = this.ballSize. The constructor-state record is
 * read only; outputs are separate data-only values. No coordinate conversion,
 * owner access, LevelTiles mutation, camera/E-buffer work, or resume logic is
 * performed. All output pointers are required and reset to zero on failure.
 */
int bounce_player_tile_coordinate_apply(
    const BouncePlayerConstructorState *state,
    int tile_x,
    int tile_y,
    int *d_out,
    int *c_out,
    int *b_out
);

/*
 * Compose only the verified new-game data sequence:
 * level initial header -> constructor pixel/ball inputs -> constructor data
 * defaults -> l/o = 0/0 -> d/c/b tile-coordinate assignment.
 *
 * The caller supplies an already parsed BounceLevel. This helper does not
 * load a file, own a player or level, touch LevelTiles, invoke sprite setup,
 * probe big-ball collisions, reset camera/E-buffer state, or perform resume,
 * gameplay, persistence, or rendering. For the big-ball branch it preserves
 * the supplied constructor coordinates; the unresolved Java collision probe
 * is intentionally outside this data-only composition.
 *
 * On failure, state_out and coordinate outputs are reset to zero when their
 * pointers are non-NULL. All output pointers are required for success.
 */
int bounce_level_initialize_new_game_player_data(
    const BounceLevel *level,
    BouncePlayerConstructorState *state_out,
    int *d_out,
    int *c_out,
    int *b_out
);

/* Structural storage queries return 0 for a NULL level. */
size_t bounce_level_tile_count(const BounceLevel *level);
uint32_t bounce_level_thorn_count(const BounceLevel *level);

/*
 * Copy one raw row-major tile byte to tile_out. Returns -1 for NULL arguments
 * or out-of-range coordinates. No gameplay meaning is attached to the byte.
 */
int bounce_level_get_tile(
    const BounceLevel *level,
    int x,
    int y,
    uint8_t *tile_out
);

/*
 * Expose only the opaque thorn payload, excluding its count byte. The payload
 * pointer is borrowed from the level and remains valid until level destruction.
 * For thorn_count == 0, payload_out receives NULL and length_out receives 0.
 * Returns -1 for NULL level or output arguments.
 */
int bounce_level_get_thorn_payload(
    const BounceLevel *level,
    const uint8_t **payload_out,
    size_t *length_out
);

/*
 * Isolated mutable runtime tile-value boundary. The corrected Java audit
 * establishes short values in the 0..255 domain for raw loads and the audited
 * runtime mutations. A uint16_t cell represents that complete established
 * domain without treating 0x80 as a sign bit. It is not a claim that every
 * theoretical Java short value is represented. The object copies structural
 * BounceLevel tile bytes and owns no level, scene, player, renderer, or thorn
 * state. The caller owns the returned object and must destroy it.
 */
BounceRuntimeLevelTiles *bounce_runtime_level_tiles_create(
    const BounceLevel *level
);
void bounce_runtime_level_tiles_destroy(
    BounceRuntimeLevelTiles *runtime
);
uint32_t bounce_runtime_level_tiles_width(
    const BounceRuntimeLevelTiles *runtime
);
uint32_t bounce_runtime_level_tiles_height(
    const BounceRuntimeLevelTiles *runtime
);

/*
 * Borrowed read-only view of the runtime copy's dynamic-thorn animation state,
 * as reconstructed from b.java. Returns NULL for a NULL runtime. The returned
 * pointer stays valid until the runtime object is destroyed and must not be
 * freed by the caller. See dyn_thorns.h.
 */
const struct BounceDynThorns *bounce_runtime_level_tiles_dyn_thorns(
    const BounceRuntimeLevelTiles *runtime
);

/*
 * G-R3-R1-D -- install saved dyn-thorn animation offsets into a runtime copy's
 * OWNED records, the native equivalent of e.AddScore() (e.java:482-492).
 *
 * This exists as a function rather than by making
 * bounce_runtime_level_tiles_dyn_thorns() return a mutable pointer. The read-only
 * view is borrowed by the collision query and by renderers, and widening its
 * constness would grant every one of those callers write access to the animation
 * state. A named mutator keeps the write capability explicit and confined to the
 * one transition that performs it.
 *
 * The runtime must already own its records -- stage 3 of the canonical entry
 * chain creates them, and this must not be called before that. See
 * bounce_dyn_thorns_restore_offsets() in dyn_thorns.h for exactly what is and is
 * not touched.
 *
 * Returns 0 on success, -1 for a NULL runtime, or when the supplied count
 * exceeds the records the runtime owns.
 */
int bounce_runtime_level_tiles_restore_dyn_thorns(
    BounceRuntimeLevelTiles *runtime,
    uint32_t count,
    const struct BounceVec2S *w_values,
    const struct BounceVec2S *ae_values
);

/*
 * Advance the runtime copy's dynamic-thorn animation by exactly one step, the
 * native equivalent of the source's guarded call at e.java:282-283:
 *
 *     if (this.DynThornsCount != 0) UpdateDynThorns();
 *
 * The mutable owner stays this runtime object; BounceLevel is never read for
 * payload decoding here and is never modified. No second thorn array, second
 * runtime level, per-tick copy, or per-tick allocation is created.
 *
 * Returns 1 when the update actually ran (thorn count != 0), 0 when the source
 * guard skipped it (thorn count == 0), and -1 for a NULL runtime or a failed
 * update.
 */
int bounce_runtime_level_tiles_update_dyn_thorns(
    BounceRuntimeLevelTiles *runtime
);

/*
 * Read or write one mutable runtime cell using LevelTiles[y][x] coordinates.
 * Reads reset value_out to zero before validation. Invalid coordinates and
 * NULL arguments return -1; valid operations return 0. No clamping is
 * performed. Writes affect only the runtime object.
 */
int bounce_runtime_level_tiles_get(
    const BounceRuntimeLevelTiles *runtime,
    int x,
    int y,
    uint16_t *value_out
);
int bounce_runtime_level_tiles_set(
    BounceRuntimeLevelTiles *runtime,
    int x,
    int y,
    uint16_t value
);

/*
 * Exact audited value expressions. int32_t results model the Java int used
 * after short-to-int promotion; these helpers are not a general bitfield
 * framework and do not implement collision or persistence records.
 */
uint16_t bounce_level_tile_value_from_raw(
    uint8_t raw_value
);
int32_t bounce_level_tile_value_water(
    uint16_t value
);
int32_t bounce_level_tile_value_without_water(
    uint16_t value
);
int32_t bounce_level_tile_value_without_dirty(
    uint16_t value
);
int32_t bounce_level_tile_value_persistence_normalize(
    uint16_t value
);
int32_t bounce_level_tile_value_persistence_without_water(
    uint16_t value
);
uint16_t bounce_level_tile_value_set_dirty(
    uint16_t value
);
uint16_t bounce_level_tile_value_clear_dirty(
    uint16_t value
);
uint16_t bounce_level_tile_value_reconstruct(
    uint16_t old_value,
    uint16_t literal
);
int bounce_level_tile_value_has_dirty(
    uint16_t value
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_LEVEL_H */
