#ifndef BOUNCE_NATIVE_APP_COLLISION_QUERY_H
#define BOUNCE_NATIVE_APP_COLLISION_QUERY_H

#include <stdbool.h>
#include <stdint.h>

#include "../level/level.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pure, mask-backed collision-fit query for the original runtime LevelTiles.
 *
 * The coordinate traversal and regular/big ball masks are the recovered
 * f.TODO_CheckBallCollision()/f.TODO_CheckWallCollision() rules. A candidate
 * tile outside the structural map is treated exactly like the original
 * helper's out-of-range false result, so it makes the complete query return
 * false. The query does not mutate tiles or any player/game state.
 *
 * This boundary intentionally covers the verified static mask geometry
 * (ordinary/rubber walls, static thorns, and the original slope masks). It
 * does not reproduce dynamic-thorn animation state, hoop/exit/power-up
 * interactions, or any collision response/side effect from f.a().
 */
bool bounce_collision_query(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    const BounceRuntimeLevelTiles *runtime
);

/*
 * Same pure predicate and the same f.TODO_CheckBallCollision traversal, reading
 * the pristine BounceLevel tile bytes instead of a runtime tile copy. This is
 * the seam the original f.UseBigBall() constructor probe needs, because at that
 * point the original reads the LevelTiles bytes LoadLevelId has just written.
 * It is read-only: it never mutates the level and never creates a runtime copy.
 */
bool bounce_collision_query_level(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    const BounceLevel *level
);

/*
 * Test one in-bounds runtime tile with the same static mask used by the
 * complete query. This read-only boundary lets the vertical response path
 * identify the source tile kind without changing query semantics.
 */
bool bounce_collision_tile_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    int32_t tile_x,
    int32_t tile_y,
    const BounceRuntimeLevelTiles *runtime);

/*
 * Pure shape overlap for one in-bounds runtime tile: the f.b() verdict
 * (f.java:369-385), and exactly what java_shape_overlaps() computes.
 *
 * The source applies two independent gates to a hoop tile. f.b() alone
 * authorises OnTouchHoop() and the consumed-tile rewrite; f.a() only adds the
 * blocking verdict. The two regions are not nested, so collection must be
 * decidable without the blocking verdict. This exposes that first gate.
 *
 * API exposure only. It calls the single java_shape_overlaps() implementation
 * and restates no geometry, and it does not change the blocking verdict returned
 * by bounce_collision_tile_overlaps(), which is unchanged.
 */
bool bounce_collision_tile_shape_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    int32_t tile_x,
    int32_t tile_y,
    const BounceRuntimeLevelTiles *runtime);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_COLLISION_QUERY_H */
