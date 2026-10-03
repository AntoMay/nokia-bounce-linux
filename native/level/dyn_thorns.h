#ifndef BOUNCE_NATIVE_LEVEL_DYN_THORNS_H
#define BOUNCE_NATIVE_LEVEL_DYN_THORNS_H

#include <stdbool.h>
#include <stdint.h>

#include "level.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dynamic-thorn state recovered from b.java.
 *
 * The original keeps four per-thorn vectors. Two are immutable geometry in tile
 * units and two are mutable animation state in pixel units:
 *
 *   DynThornsBottomLeft  b.java:293, 304   tile corner, inclusive
 *   DynThornsTopRight    b.java:294, 308   tile corner, exclusive
 *   ae                   b.java:295, 312-313  pixel delta applied per update
 *   w                    b.java:296, 317-318  pixel offset, integrated by ae
 *
 * The 8-byte payload layout (bytes 1-2 / 3-4 / 5-6 / 7-8) is derived in
 * reverse/COLLISION-QUERY-FIDELITY-AUDIT.md from UpdateDynThorns' own
 * integrate/clamp/reflect structure. The decompiled b.java:299-317 reader that
 * performs ten reads is a decompilation defect and is not followed.
 *
 * This structure is the runtime owner: it is held by value inside
 * BounceRuntimeLevelTiles because b.java advances w and ae once per
 * e.Tick() (e.java:282-283). BounceLevel keeps the payload opaque and
 * immutable.
 */

/* Java Vec2S: two signed shorts. Promoted to int in expressions, as in Java. */
typedef struct BounceVec2S {
    int16_t x;
    int16_t y;
} BounceVec2S;

typedef struct BounceDynThornRecord {
    BounceVec2S bottom_left;
    BounceVec2S top_right;
    BounceVec2S ae;
    BounceVec2S w;
} BounceDynThornRecord;

/*
 * A non-owning read interface over per-thorn records. This is the one seam that
 * lets the collision query evaluate the tile-10 rule against either the owned
 * runtime animation state or the immutable level payload, without duplicating
 * the rule and without copying or mutating BounceLevel.
 *
 * read_record must fill record_out for a zero-based index below count and
 * return true, or return false.
 */
typedef struct BounceDynThornsSource {
    bool (*read_record)(
        const void *context,
        uint32_t index,
        BounceDynThornRecord *record_out
    );
    const void *context;
    uint32_t count;
} BounceDynThornsSource;

/* Owned mutable animation state. records is NULL when count is 0. */
typedef struct BounceDynThorns {
    BounceDynThornRecord *records;
    uint32_t count;
} BounceDynThorns;

/*
 * b.java:389-396 TestPointInsideDynThorns, transcribed exactly: scan records in
 * index order and return the FIRST index satisfying
 *
 *   x >= BottomLeft.x && x < TopRight.x && y >= BottomLeft.y && y < TopRight.y
 *
 * or -1 when none matches. x and y are tile coordinates. Pure: it reads the
 * source and mutates nothing.
 */
int32_t bounce_dyn_thorns_test_point_inside(
    const BounceDynThornsSource *source,
    int32_t tile_x,
    int32_t tile_y
);

/*
 * Decode one record directly from the immutable opaque level payload using the
 * recovered 8-byte layout. Returns -1 for a NULL level/output, an out-of-range
 * index, or a payload whose length is not index*8 + 8. Pure: BounceLevel is not
 * modified and no state is allocated.
 */
int bounce_level_dyn_thorn_record(
    const BounceLevel *level,
    uint32_t index,
    BounceDynThornRecord *record_out
);

/*
 * A read source over the pristine level payload. This yields the INITIAL w
 * values, which is the correct state at the e.java:127 constructor boundary
 * because no UpdateDynThorns call has occurred yet. Returns -1 for a NULL
 * level or output.
 */
int bounce_level_dyn_thorns_source(
    const BounceLevel *level,
    BounceDynThornsSource *source_out
);

/*
 * Initialize owned state from the level payload's initial values, exactly as
 * b.java:299-318 stores them. Returns -1 for a NULL output, a NULL level, or an
 * allocation failure. No clamping is performed here; the original clamps only
 * inside UpdateDynThorns.
 *
 * thorns must be fresh storage: either zero-initialized (as the calloc'd
 * BounceRuntimeLevelTiles is) or already passed to bounce_dyn_thorns_reset().
 * The struct is zeroed in place, never released, so uninitialized caller storage
 * is never freed. On failure the struct is left zeroed.
 */
int bounce_dyn_thorns_init_from_level(
    BounceDynThorns *thorns,
    const BounceLevel *level
);

/* Safe with NULL; releases all dyn-thorn-owned storage. */
void bounce_dyn_thorns_reset(BounceDynThorns *thorns);

/* A read source over the owned state. source_out is zeroed on failure. */
int bounce_dyn_thorns_source(
    const BounceDynThorns *thorns,
    BounceDynThornsSource *source_out
);

/*
 * b.java:340-363 UpdateDynThorns, restricted to the animated state it owns:
 * w += ae, clamp each axis to [0, (TopRight - BottomLeft - 2) * 12], and negate
 * ae on the axis that reached a bound. Java int32 wrapping and the narrowing
 * assignment back to short are preserved.
 *
 * The LevelTiles 0x80 dirty-marking at b.java:364-385 is deliberately NOT
 * performed: it is a renderer invalidation flag that the collision query masks
 * off before dispatching on the tile id, so it cannot affect any verdict.
 *
 * This is an explicit state transition, never called by a collision query. The
 * original advances the thorns in e.Tick after the player's collision check, so
 * the animation phase is gameplay-tick state and is not advanced here.
 * Returns -1 for NULL state, otherwise 0.
 */
int bounce_dyn_thorns_update(BounceDynThorns *thorns);

/*
 * G-R3-R1-D -- the D4-B restore seam for e.AddScore(), e.java:482-492.
 *
 * WHY A SEAM IS NEEDED AT ALL. AddScore() is four scalar assignments:
 *
 *     for (byte b1 = 0; b1 < this.game.r; b1++) {
 *         this.ae[b1].x = this.game.l[b1][0];
 *         this.ae[b1].y = this.game.l[b1][1];
 *         this.w[b1].x  = this.game.D[b1].x;
 *         this.w[b1].y  = this.game.D[b1].y;
 *     }
 *
 * Every destination field already exists here as BounceDynThornRecord.ae/.w, so
 * there is no representation gap. What was missing was any way to WRITE them:
 * bounce_dyn_thorns_update() integrates and clamps rather than assigns, and
 * bounce_dyn_thorns_reset() frees the records, so neither can install saved
 * values. The only accessor for a runtime's thorns returns const.
 *
 * WHAT THIS DOES, and nothing else. For each index below count it assigns the
 * four supplied shorts into records[index].w and records[index].ae.
 *
 * WHAT THIS DELIBERATELY DOES NOT DO, each of which is a fidelity requirement
 * rather than an omission:
 *
 *   - It does not integrate. AddScore() does not call UpdateDynThorns(), and
 *     e.java:282-283 runs that only from the tick. Calling update() here would
 *     advance every thorn by one step and corrupt the restored phase, which is
 *     the whole content of the saved state.
 *   - It does not clamp. Java assigns raw shorts; the first UpdateDynThorns()
 *     after resume performs the clamping, exactly as it would have.
 *   - It does not touch bottom_left or top_right. They are the level-derived
 *     clamp bounds read at b.java:347-348 and are level-invariant, so the
 *     restored w/ae are already inside valid bounds.
 *   - It does not change count, allocate, free, resize or recreate records.
 *     Records must already exist; this installs values into them.
 *   - It does not touch tiles or collision state. AddScore() references neither,
 *     and the dyn-thorn collision query is a linear scan with no derived cache.
 *
 * count is the number of saved records. Records at indices >= count are left
 * exactly as the level loader produced them, and a saved count larger than the
 * native record count installs only what fits -- the bounds are enforced here
 * rather than trusted, because Java has no such check and would throw
 * ArrayIndexOutOfBoundsException. Returning -1 for that case lets the caller
 * treat it as a failure instead of silently installing a partial state.
 *
 * Returns 0 on success, -1 for NULL thorns, NULL records with a non-zero count,
 * or count greater than the available record count.
 */
int bounce_dyn_thorns_restore_offsets(
    BounceDynThorns *thorns,
    uint32_t count,
    const BounceVec2S *w_values,
    const BounceVec2S *ae_values
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_LEVEL_DYN_THORNS_H */
