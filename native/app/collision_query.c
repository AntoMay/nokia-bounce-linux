#include "collision_query.h"

#include "../level/dyn_thorns.h"

#include <stdint.h>
#include <string.h>

enum {
    BOUNCE_COLLISION_TILE_SIZE = 12,
    BOUNCE_REGULAR_HALF_SIZE = 6,
    BOUNCE_BIG_HALF_SIZE = 8,
    BOUNCE_WATER_BIT = 0x40,
    BOUNCE_DIRTY_BIT = 0x80
};

/* These are the recovered f.java BALL_COLLISION entries, in row order. */
static const uint8_t bounce_regular_collision[12][12] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0}
};

/* These are the recovered f.java BIG_BALL_COLLISION entries, in row order. */
static const uint8_t bounce_big_ball_collision[16][16] = {
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}
};

/* These are the recovered f.java SLOPE_COLLISION entries, in row order. */
static const uint8_t bounce_slope_collision[12][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

static int32_t java_int32_add(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left + (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_subtract(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left - (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_multiply(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left * (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_divide_by_tile_size(int32_t value)
{
    /* C23/C99 signed integer division truncates toward zero, as Java int does. */
    return value / BOUNCE_COLLISION_TILE_SIZE;
}

static int32_t java_int32_absolute(int32_t value)
{
    return value < 0 ? java_int32_subtract(0, value) : value;
}

static bool java_int32_less_or_equal(int32_t left, int32_t right)
{
    return left <= right;
}

/*
 * f.b(...) + f.a(int x8): the static-thorn inclusive rectangle test
 * (f.java:369-385 and f.java:858-860).
 *
 * The original builds the ball box as
 *
 *     [x - p, x + p - 1] x [y - p, y + p - 1]
 *
 * and the thorn box as [rect_left, rect_right - 1] x [rect_top, rect_bottom - 1],
 * then applies the inclusive test
 *
 *     (x - p <= rect_right) && (y - p <= rect_bottom)
 *  && (rect_left <= x + p - 1) && (rect_top <= y + p - 1)
 *
 * The trailing "-1" on the ball box is load-bearing: the original treats exact
 * edge contact as free. A symmetric [x - p, x + p] box produced a one-pixel
 * false-collision band; see reverse/COLLISION-QUERY-FIDELITY-AUDIT.md (D1).
 * The subtraction is evaluated as (x + p) - 1, matching Java's left-to-right
 * order for `x + p - 1`.
 */
static bool java_thorn_rect_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t rect_left,
    int32_t rect_top,
    int32_t rect_right,
    int32_t rect_bottom
)
{
    int32_t ball_left = java_int32_subtract(player_x, half_size);
    int32_t ball_top = java_int32_subtract(player_y, half_size);
    int32_t ball_right = java_int32_add(
        java_int32_add(player_x, half_size),
        -1
    );
    int32_t ball_bottom = java_int32_add(
        java_int32_add(player_y, half_size),
        -1
    );

    return java_int32_less_or_equal(ball_left, rect_right)
        && java_int32_less_or_equal(ball_top, rect_bottom)
        && java_int32_less_or_equal(rect_left, ball_right)
        && java_int32_less_or_equal(rect_top, ball_bottom);
}

static const uint8_t *ball_collision_mask(int32_t half_size)
{
    if (half_size == BOUNCE_REGULAR_HALF_SIZE)
        return &bounce_regular_collision[0][0];
    if (half_size == BOUNCE_BIG_HALF_SIZE)
        return &bounce_big_ball_collision[0][0];
    return NULL;
}

static int32_t ball_collision_width(int32_t half_size)
{
    return half_size == BOUNCE_REGULAR_HALF_SIZE ? 12 : 16;
}

static bool wall_mask_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y
)
{
    const uint8_t *mask;
    int32_t mask_width;
    int32_t ball_size;
    int32_t tile_left;
    int32_t tile_top;
    int32_t x_offset;
    int32_t y_offset;
    int32_t x_start;
    int32_t x_end;
    int32_t y_start;
    int32_t y_end;
    int32_t x;
    int32_t y;

    mask = ball_collision_mask(half_size);
    if (mask == NULL)
        return false;
    mask_width = ball_collision_width(half_size);
    ball_size = java_int32_add(half_size, half_size);
    tile_left = java_int32_multiply(tile_x, BOUNCE_COLLISION_TILE_SIZE);
    tile_top = java_int32_multiply(tile_y, BOUNCE_COLLISION_TILE_SIZE);
    x_offset = java_int32_subtract(
        java_int32_subtract(player_x, half_size),
        tile_left
    );
    y_offset = java_int32_subtract(
        java_int32_subtract(player_y, half_size),
        tile_top
    );

    if (x_offset >= 0) {
        x_start = x_offset;
        x_end = BOUNCE_COLLISION_TILE_SIZE;
    } else {
        x_start = 0;
        x_end = java_int32_add(ball_size, x_offset);
    }
    if (y_offset >= 0) {
        y_start = y_offset;
        y_end = BOUNCE_COLLISION_TILE_SIZE;
    } else {
        y_start = 0;
        y_end = java_int32_add(ball_size, y_offset);
    }
    if (x_end > BOUNCE_COLLISION_TILE_SIZE)
        x_end = BOUNCE_COLLISION_TILE_SIZE;
    if (y_end > BOUNCE_COLLISION_TILE_SIZE)
        y_end = BOUNCE_COLLISION_TILE_SIZE;

    /* The source traversal is X outer and Y inner. */
    for (x = x_start; x < x_end; x = java_int32_add(x, 1)) {
        for (y = y_start; y < y_end; y = java_int32_add(y, 1)) {
            int32_t mask_x = java_int32_subtract(x, x_offset);
            int32_t mask_y = java_int32_subtract(y, y_offset);

            if (mask_x >= 0
                && mask_x < mask_width
                && mask_y >= 0
                && mask_y < mask_width
                && mask[(size_t)mask_y * (size_t)mask_width
                    + (size_t)mask_x] != 0u)
                return true;
        }
    }
    return false;
}

/*
 * f.b(...) (f.java:369-385): the shape rectangle test shared by static thorns,
 * tile 9, the hoop tiles, and the pumper tiles. The inset switch below is the
 * original's full case list, not a thorns-only subset.
 */
static bool java_shape_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    int32_t tile_id
)
{
    int32_t left = java_int32_multiply(tile_x, BOUNCE_COLLISION_TILE_SIZE);
    int32_t top = java_int32_multiply(tile_y, BOUNCE_COLLISION_TILE_SIZE);
    int32_t right = java_int32_add(left, BOUNCE_COLLISION_TILE_SIZE);
    int32_t bottom = java_int32_add(top, BOUNCE_COLLISION_TILE_SIZE);

    switch (tile_id) {
        case 3: case 5: case 9: case 13: case 14: case 17: case 18:
        case 21: case 22: case 43: case 45:
            left = java_int32_add(left, 4);
            right = java_int32_subtract(right, 4);
            break;
        case 4: case 6: case 15: case 16: case 19: case 20: case 23:
        case 24: case 44: case 46:
            top = java_int32_add(top, 4);
            bottom = java_int32_subtract(bottom, 4);
            break;
        default:
            break;
    }

    return java_thorn_rect_overlaps(
        player_x,
        player_y,
        half_size,
        left,
        top,
        java_int32_subtract(right, 1),
        java_int32_subtract(bottom, 1)
    );
}

/*
 * f.a(int,int,int,int,int) (f.java:387-421): the second hoop/pumper shape test.
 *
 * Note the ball box: this helper passes x + p and y + p (NOT x + p - 1) to the
 * 8-argument inclusive AABB at f.java:420, unlike f.b(...) at f.java:384. The
 * two source functions therefore use different ball boxes, and both are kept
 * exactly as the original has them.
 */
static bool java_hoop_shape_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    int32_t tile_id
)
{
    int32_t i = java_int32_multiply(tile_x, BOUNCE_COLLISION_TILE_SIZE);
    int32_t j = java_int32_multiply(tile_y, BOUNCE_COLLISION_TILE_SIZE);
    int32_t k = java_int32_add(i, BOUNCE_COLLISION_TILE_SIZE);
    int32_t m = java_int32_add(j, BOUNCE_COLLISION_TILE_SIZE);

    switch (tile_id) {
        case 15: case 19: case 23: case 27:
            j = java_int32_add(j, 6);
            m = java_int32_subtract(m, 6);
            k = java_int32_subtract(k, 11);
            break;
        case 16: case 20: case 24: case 28:
            j = java_int32_add(j, 6);
            m = java_int32_subtract(m, 6);
            i = java_int32_add(i, 11);
            break;
        case 13: case 17:
            i = java_int32_add(i, 6);
            k = java_int32_subtract(k, 6);
            m = java_int32_subtract(m, 11);
            break;
        case 21: case 25:
            m = j;
            j = java_int32_subtract(j, 1);
            i = java_int32_add(i, 6);
            k = java_int32_subtract(k, 6);
            break;
        case 14: case 18: case 22: case 26:
            i = java_int32_add(i, 6);
            k = java_int32_subtract(k, 6);
            j = java_int32_add(j, 11);
            break;
        default:
            break;
    }

    return java_int32_less_or_equal(
            java_int32_subtract(player_x, half_size),
            k
        )
        && java_int32_less_or_equal(
            java_int32_subtract(player_y, half_size),
            m
        )
        && java_int32_less_or_equal(
            i,
            java_int32_add(player_x, half_size)
        )
        && java_int32_less_or_equal(
            j,
            java_int32_add(player_y, half_size)
        );
}

static bool slope_mask_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    int32_t tile_id
)
{
    const uint8_t *mask;
    int32_t mask_width;
    int32_t ball_size;
    int32_t tile_left;
    int32_t tile_top;
    int32_t x_offset;
    int32_t y_offset;
    int32_t x_start;
    int32_t x_end;
    int32_t y_start;
    int32_t y_end;
    int32_t slope_edge_x = 0;
    int32_t slope_edge_y = 0;
    int32_t x;
    int32_t y;

    mask = ball_collision_mask(half_size);
    if (mask == NULL)
        return false;
    mask_width = ball_collision_width(half_size);
    ball_size = java_int32_add(half_size, half_size);
    tile_left = java_int32_multiply(tile_x, BOUNCE_COLLISION_TILE_SIZE);
    tile_top = java_int32_multiply(tile_y, BOUNCE_COLLISION_TILE_SIZE);
    x_offset = java_int32_subtract(
        java_int32_subtract(player_x, half_size),
        tile_left
    );
    y_offset = java_int32_subtract(
        java_int32_subtract(player_y, half_size),
        tile_top
    );

    if (tile_id == 30 || tile_id == 34) {
        slope_edge_y = 11;
        slope_edge_x = 11;
    } else if (tile_id == 31 || tile_id == 35) {
        slope_edge_y = 11;
    } else if (tile_id == 33 || tile_id == 37) {
        slope_edge_x = 11;
    }

    if (x_offset >= 0) {
        x_start = x_offset;
        x_end = BOUNCE_COLLISION_TILE_SIZE;
    } else {
        x_start = 0;
        x_end = java_int32_add(ball_size, x_offset);
    }
    if (y_offset >= 0) {
        y_start = y_offset;
        y_end = BOUNCE_COLLISION_TILE_SIZE;
    } else {
        y_start = 0;
        y_end = java_int32_add(ball_size, y_offset);
    }
    if (x_end > BOUNCE_COLLISION_TILE_SIZE)
        x_end = BOUNCE_COLLISION_TILE_SIZE;
    if (y_end > BOUNCE_COLLISION_TILE_SIZE)
        y_end = BOUNCE_COLLISION_TILE_SIZE;

    /* The source traversal is X outer and Y inner. */
    for (x = x_start; x < x_end; x = java_int32_add(x, 1)) {
        for (y = y_start; y < y_end; y = java_int32_add(y, 1)) {
            int32_t slope_x = java_int32_absolute(
                java_int32_subtract(x, slope_edge_x)
            );
            int32_t slope_y = java_int32_absolute(
                java_int32_subtract(y, slope_edge_y)
            );
            int32_t mask_x = java_int32_subtract(x, x_offset);
            int32_t mask_y = java_int32_subtract(y, y_offset);

            if (slope_x >= 0
                && slope_x < BOUNCE_COLLISION_TILE_SIZE
                && slope_y >= 0
                && slope_y < BOUNCE_COLLISION_TILE_SIZE
                && mask_x >= 0
                && mask_x < mask_width
                && mask_y >= 0
                && mask_y < mask_width
                && (bounce_slope_collision[(size_t)slope_y]
                    [(size_t)slope_x]
                    & mask[(size_t)mask_y * (size_t)mask_width
                        + (size_t)mask_x]) != 0u)
                return true;
        }
    }
    return false;
}

/*
 * BOUNDED D2 PURE-QUERY POLICY.
 *
 * The original f.a(x, y, tileY, tileX) resolves the interaction tiles with
 * side effects: LevelTiles rewrites, AddScore/OnTouchHoop, power-up counters,
 * flag writes (u, v, M), sound playback, and KillBall(). This pure query keeps
 * only the FREE/BLOCKED verdict each case contributes and reproduces none of
 * the side effects. Every rule below is transcribed from the case list in
 * f.java:433-645.
 *
 * Deliberately excluded from the query (caller or gameplay-state concerns):
 *   - the dying-ball early return f.java:427-428 (this.z == 2)
 *   - the level-complete flag f.java:601 (this.n.M) on tile 9, which only
 *     *relaxes* the tile-9 block, so the geometric rule is kept
 *   - KillBall() on tiles 10 and 3-6
 *   - every LevelTiles rewrite, score, hoop, power-up and audio effect
 *
 * Only one case has a verdict that genuinely cannot be determined without
 * state this query does not own: tile 10, whose block depends on the moving
 * dynamic-thorn box. The source default (not blocked) is preserved there, and
 * the loss is recorded rather than approximated.
 */
/*
 * f.java:463-473, the tile-10 dynamic-thorn branch.
 *
 *   k = this.n.TestPointInsideDynThorns(tileX, tileY);
 *   if (k != -1) {
 *       int m = this.n.DynThornsBottomLeft[k].x * 12 + this.n.w[k].x;
 *       int n = this.n.DynThornsBottomLeft[k].y * 12 + this.n.w[k].y;
 *       if (a(x - this.p + 1, y - this.p + 1, x + this.p - 1, y + this.p - 1,
 *             m + 1, n + 1, m + 24 - 1, n + 24 - 1)) { bool = false; KillBall(); }
 *   }
 *
 * The overlap is the 8-argument f.a(...) at f.java:858-860, whose four
 * comparisons are transcribed below in the original argument order. Both boxes
 * carry a symmetric one-pixel inset; this is deliberately NOT the asymmetric D1
 * thorn box from f.java:369-385.
 *
 * Java evaluation order is preserved: (x - p) + 1, (x + p) - 1, and (m + 24) - 1
 * are each evaluated as written, through the int32 wrapping helpers.
 */
static bool java_dyn_thorn_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    const BounceDynThornsSource *thorns
)
{
    int32_t index;
    BounceDynThornRecord thorn;
    int32_t m;
    int32_t n;
    int32_t ball_left;
    int32_t ball_top;
    int32_t ball_right;
    int32_t ball_bottom;

    if (thorns == NULL)
        return false;

    /* b.java:464-465 */
    index = bounce_dyn_thorns_test_point_inside(thorns, tile_x, tile_y);
    if (index < 0)
        return false;
    if (!thorns->read_record(thorns->context, (uint32_t)index, &thorn))
        return false;

    /* b.java:466-467: tile corner in pixels plus the animated pixel offset. */
    m = java_int32_add(
            java_int32_multiply((int32_t)thorn.bottom_left.x, 12),
            (int32_t)thorn.w.x
        );
    n = java_int32_add(
            java_int32_multiply((int32_t)thorn.bottom_left.y, 12),
            (int32_t)thorn.w.y
        );

    ball_left = java_int32_add(
        java_int32_subtract(player_x, half_size), 1);
    ball_top = java_int32_add(
        java_int32_subtract(player_y, half_size), 1);
    ball_right = java_int32_subtract(
        java_int32_add(player_x, half_size), 1);
    ball_bottom = java_int32_subtract(
        java_int32_add(player_y, half_size), 1);

    /* f.java:859: (p1 <= p7) && (p2 <= p8) && (p5 <= p3) && (p6 <= p4) */
    return java_int32_less_or_equal(
            ball_left,
            java_int32_subtract(java_int32_add(m, 24), 1)
        )
        && java_int32_less_or_equal(
            ball_top,
            java_int32_subtract(java_int32_add(n, 24), 1)
        )
        && java_int32_less_or_equal(
            java_int32_add(m, 1),
            ball_right
        )
        && java_int32_less_or_equal(
            java_int32_add(n, 1),
            ball_bottom
        );
}

static bool java_interaction_collision(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    int32_t tile_id,
    const BounceDynThornsSource *thorns
)
{
    bool big_ball = half_size == BOUNCE_BIG_HALF_SIZE;
    bool shape;
    bool hoop;

    switch (tile_id) {
        /*
         * f.java:463-473. The original sets bool = false only when BOTH
         *   (a) TestPointInsideDynThorns(tileX, tileY) != -1, and
         *   (b) the ball box [x-p+1, x+p-1] x [y-p+1, y+p-1] overlaps the
         *       *moving* thorn box derived from DynThornsBottomLeft[k] and
         *       the animated offset w[k].
         *
         * Both conditions are now evaluated. The dynamic-thorn state is a
         * borrowed const source: the query reads w but never advances it and
         * never mutates LevelTiles, the level, or any gameplay state. The
         * original's KillBall() at f.java:470 is still never called; only the
         * bool = false verdict is returned.
         *
         * When the caller supplies no source, TestPointInsideDynThorns has no
         * records to scan and returns -1, so the tile does not block. That is
         * the source result for a level with no dynamic thorns, not a
         * replacement for the rule above.
         */
        case 10:
            return java_dyn_thorn_overlaps(
                player_x,
                player_y,
                half_size,
                tile_x,
                tile_y,
                thorns
            );
        /* f.java:618-622 - 39,40,41,42 set bool = false unconditionally. */
        case 39:
        case 40:
        case 41:
        case 42:
            return true;
        /* f.java:595-598 - case 18: b(...) && ballSize == 16. */
        case 18:
            return big_ball
                && java_shape_overlaps(
                    player_x, player_y, half_size, tile_x, tile_y, tile_id
                );
        /* f.java:591-594 - case 25,27,28: second test only, no b() gate. */
        case 25:
        case 27:
        case 28:
            return java_hoop_shape_overlaps(
                player_x, player_y, half_size, tile_x, tile_y, tile_id
            );
        /*
         * f.java:630-635 - POWER_UP_GRAVITY_UP / _RIGHT / _DOWN / _LEFT
         * (TileIDs.java:19-22) set bool = false at f.java:634 with no geometry
         * test, so the tile blocks. This is a pure VERDICT: the pickup's state
         * writes (powerUpGravity, m) belong to the caller, which is
         * app_vertical_collision_info() in vertical_slice.c. Nothing in this
         * module reads or writes player, level, or audio state.
         */
        case 47:
        case 48:
        case 49:
        case 50:
            return true;
        /*
         * f.java:636-640 - case 51, 52, 53, 54. These are the unnamed
         * interaction tiles drawn from the jump-power-up sprite at b.java:567-573;
         * b.java:97-100 calls 51-54 PUMPER / POWER_UP_GRAVITY / POWER_UP_SPEED /
         * POWER_UP_JUMP, but those are SPRITE indices, not tile names, and the
         * sprite must not be used to infer gameplay.
         *
         * The source runs three statements, none of which is a geometry test:
         *
         *   f.java:637  this.TODO_somePowerUp3 = 300;
         *   f.java:638  sound = this.n.soundPickup;   (NOT IMPLEMENTED)
         *   f.java:639  bool = false;
         *
         * bool = false is the BLOCKING verdict, so the tile blocks wherever the
         * 12x12 cell falls inside the ball's footprint, exactly as for 47-50
         * above. There is no b(...) / c(...) gate, no ballSize test, and no
         * m write, so the verdict is unconditional and size-independent.
         *
         * As with 47-50, the TODO_somePowerUp3 assignment is NOT made here: this
         * module stays pure, and the write belongs to the caller,
         * app_vertical_collision_info() in vertical_slice.c. The 0x40 water bit
         * and the 0x80 dirty bit are already stripped above, so a flagged tile
         * dispatches here identically -- matching f.java:430-431.
         */
        case 51:
        case 52:
        case 53:
        case 54:
            return true;
        /*
         * f.java:641-645 - case 38, the tile-38 speed power-up. The source runs:
         *
         *   f.java:642  this.TODO_somePowerUp1 = 300;
         *   f.java:643  sound = this.n.soundPickup;   (NOT IMPLEMENTED)
         *   f.java:644  bool = false;
         *
         * bool = false is the BLOCKING verdict, so the tile blocks wherever the
         * 12x12 cell falls inside the ball's footprint, exactly as for 47-50 and
         * 51-54 above. There is no b(...) / c(...) gate and no ballSize test, so
         * the verdict is unconditional and size-independent.
         *
         * The TODO_somePowerUp1 assignment is NOT made here: this module stays
         * pure, and the write belongs to the caller, app_vertical_collision_info()
         * in vertical_slice.c. This case was previously grouped with 7 and 29 as a
         * never-changing tile and was recorded as an open CONFLICT against the
         * source; the field and its consumers now exist, so the verdict is
         * corrected here.
         */
        case 38:
            return true;
        /*
         * f.java:480-486, 609-617 - tiles that never change the verdict.
         */
        case 7:
        case 29:
            return false;
        default:
            break;
    }

    /* Remaining interaction ids are decided by f.b(...) and/or the 5-arg test. */
    shape = java_shape_overlaps(
        player_x, player_y, half_size, tile_x, tile_y, tile_id
    );
    switch (tile_id) {
        /* f.java:561-568 - case 22 collects a hoop but never blocks. */
        case 22:
            return false;
        /* f.java:599-607 - case 9 blocks when b(...) (M flag is caller state). */
        case 9:
            return shape;
        /* f.java:623-629 - pumper: b(...) only, UseBigBall() is a side effect. */
        case 43:
        case 44:
        case 45:
        case 46:
            return shape;
        /* f.java:547-560, 569-580, 581-590 - 13,15,16,17,19,20. */
        case 13:
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
            if (!shape)
                return false;
            return big_ball
                || java_hoop_shape_overlaps(
                    player_x, player_y, half_size, tile_x, tile_y, tile_id
                );
        /* f.java:569-580 - case 14 blocks for a big ball only. */
        case 14:
            return shape && big_ball;
        /* f.java:487-546 - 21,23,24 need the second test as well. */
        case 21:
        case 23:
        case 24:
            hoop = java_hoop_shape_overlaps(
                player_x, player_y, half_size, tile_x, tile_y, tile_id
            );
            return shape && hoop;
        default:
            /* Tiles the original leaves unblocked (0 EMPTY and any other id). */
            return false;
    }
}

static bool tile_has_static_collision(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_id,
    int32_t tile_x,
    int32_t tile_y,
    const BounceDynThornsSource *thorns
)
{
    switch (tile_id) {
        case 1:
        case 2:
            return wall_mask_overlaps(
                player_x,
                player_y,
                half_size,
                tile_x,
                tile_y
            );
        case 3:
        case 4:
        case 5:
        case 6:
            return java_shape_overlaps(
                player_x,
                player_y,
                half_size,
                tile_x,
                tile_y,
                tile_id
            );
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
            return slope_mask_overlaps(
                player_x,
                player_y,
                half_size,
                tile_x,
                tile_y,
                tile_id
            );
        default:
            return java_interaction_collision(
                player_x,
                player_y,
                half_size,
                tile_x,
                tile_y,
                tile_id,
                thorns
            );
    }
}

bool bounce_collision_tile_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    int32_t tile_x,
    int32_t tile_y,
    const BounceRuntimeLevelTiles *runtime
)
{
    uint16_t value;
    uint32_t tile_id;
    BounceDynThornsSource thorns;

    if (runtime == NULL
        || (player_half_size != BOUNCE_REGULAR_HALF_SIZE
            && player_half_size != BOUNCE_BIG_HALF_SIZE)
        || tile_x < 0
        || tile_y < 0
        || (uint32_t)tile_x >= bounce_runtime_level_tiles_width(runtime)
        || (uint32_t)tile_y >= bounce_runtime_level_tiles_height(runtime)
        || bounce_runtime_level_tiles_get(
            runtime,
            (int)tile_x,
            (int)tile_y,
            &value
        ) != 0)
        return false;
    tile_id = (uint32_t)value;
    tile_id &= ~(uint32_t)BOUNCE_WATER_BIT;
    tile_id &= ~(uint32_t)BOUNCE_DIRTY_BIT;
    if (bounce_dyn_thorns_source(
            bounce_runtime_level_tiles_dyn_thorns(runtime),
            &thorns
        ) != 0)
        return false;
    return tile_has_static_collision(
        player_x,
        player_y,
        player_half_size,
        (int32_t)tile_id,
        tile_x,
        tile_y,
        &thorns
    );
}

/*
 * PURE SHAPE OVERLAP -- API EXPOSURE ONLY, NO NEW GEOMETRY.
 *
 * This returns exactly the value java_shape_overlaps() produces, which is the
 * value f.b() returns in the source (f.java:369-385, dispatched through
 * java_shape_overlaps at :704). It is the FIRST of the two independent gates the
 * source applies to a hoop tile:
 *
 *   f.b(...)  f.java:369   pure shape overlap. For the six blocking hoop ids
 *                          this alone authorises OnTouchHoop() at f.java:493,
 *                          :507, :517, :531, :541 and :555, and the paired
 *                          consumed-tile rewrite that follows each one.
 *   f.a(...)  f.java:387   hoop-core overlap. This only ADDS the blocking
 *                          verdict `bool = false`; it never gates collection.
 *
 * The two regions are not nested. For tile 23, b() spans X [X0, X0+11] and
 * Y [Y0+4, Y0+7] while a() spans only X [X0, X0+1] and Y [Y0+6, Y0+6], so a
 * ball inside b() but outside a() scores in the source and does not block.
 *
 * Until now the blocking verdict was the only thing the module published, so
 * app_vertical_collision_info() could only reach collection from inside the
 * blocked branch and therefore required `shape && hoop`. This exposes the
 * already-computed `shape` so collection can be gated on b() alone.
 *
 * NOT A SECOND IMPLEMENTATION. There is exactly one copy of the shape AABB
 * arithmetic, java_shape_overlaps() at :252, and this calls it. No AABB
 * constant, tile coordinate, inset or comparison is restated here. The tile
 * read, the bounds validation and the 0x40 / 0x80 flag stripping are copied
 * from bounce_collision_tile_overlaps() so both entry points see an identical
 * tile_id, which is what keeps the exposed value and the blocking verdict
 * derived from the same inputs.
 *
 * NOT A DYN-THORN PROBE. java_shape_overlaps() takes no thorn source, so unlike
 * bounce_collision_tile_overlaps() this neither reads nor needs the dynamic
 * thorn state, and it says nothing about tile 10.
 *
 * bounce_collision_tile_overlaps() and bounce_collision_query() are unchanged;
 * the blocking verdict remains exactly java_interaction_collision()'s.
 */
bool bounce_collision_tile_shape_overlaps(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    int32_t tile_x,
    int32_t tile_y,
    const BounceRuntimeLevelTiles *runtime
)
{
    uint16_t value;
    uint32_t tile_id;

    if (runtime == NULL
        || (player_half_size != BOUNCE_REGULAR_HALF_SIZE
            && player_half_size != BOUNCE_BIG_HALF_SIZE)
        || tile_x < 0
        || tile_y < 0
        || (uint32_t)tile_x >= bounce_runtime_level_tiles_width(runtime)
        || (uint32_t)tile_y >= bounce_runtime_level_tiles_height(runtime)
        || bounce_runtime_level_tiles_get(
            runtime,
            (int)tile_x,
            (int)tile_y,
            &value
        ) != 0)
        return false;
    tile_id = (uint32_t)value;
    tile_id &= ~(uint32_t)BOUNCE_WATER_BIT;
    tile_id &= ~(uint32_t)BOUNCE_DIRTY_BIT;
    return java_shape_overlaps(
        player_x,
        player_y,
        player_half_size,
        tile_x,
        tile_y,
        tile_id
    );
}

/*
 * Tile-source seam.
 *
 * The verified f.TODO_CheckBallCollision traversal and every tile verdict below
 * are unchanged; only the byte source differs. Two owners are supported:
 * the mutable BounceRuntimeLevelTiles used by gameplay, and the pristine
 * BounceLevel used at the player-constructor boundary, where the original
 * f.UseBigBall() probe reads the same freshly written LevelTiles bytes.
 */
typedef bool (*BounceCollisionTileReader)(
    const void *context,
    int32_t tile_x,
    int32_t tile_y,
    uint32_t *tile_out
);

static bool collision_tile_read_runtime(
    const void *context,
    int32_t tile_x,
    int32_t tile_y,
    uint32_t *tile_out
)
{
    uint16_t value;

    if (context == NULL
        || tile_out == NULL
        || bounce_runtime_level_tiles_get(
            (const BounceRuntimeLevelTiles *)context,
            (int)tile_x,
            (int)tile_y,
            &value
        ) != 0)
        return false;
    *tile_out = (uint32_t)value;
    return true;
}

static bool collision_tile_read_level(
    const void *context,
    int32_t tile_x,
    int32_t tile_y,
    uint32_t *tile_out
)
{
    uint8_t value;

    if (context == NULL
        || tile_out == NULL
        || bounce_level_get_tile(
            (const BounceLevel *)context,
            (int)tile_x,
            (int)tile_y,
            &value
        ) != 0)
        return false;
    *tile_out = (uint32_t)value;
    return true;
}

static bool collision_tile_fits(
    int32_t player_x,
    int32_t player_y,
    int32_t half_size,
    int32_t tile_x,
    int32_t tile_y,
    uint32_t level_width,
    uint32_t level_height,
    BounceCollisionTileReader reader,
    const void *context,
    const BounceDynThornsSource *thorns
)
{
    uint32_t value;

    if (tile_x < 0
        || tile_y < 0
        || (uint32_t)tile_x >= level_width
        || (uint32_t)tile_y >= level_height
        || !reader(context, tile_x, tile_y, &value))
        return false;

    value &= ~(uint32_t)BOUNCE_WATER_BIT;
    value &= ~(uint32_t)BOUNCE_DIRTY_BIT;
    return !tile_has_static_collision(
        player_x,
        player_y,
        half_size,
        (int32_t)value,
        tile_x,
        tile_y,
        thorns
    );
}

static bool bounce_collision_query_source(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    uint32_t level_width,
    uint32_t level_height,
    BounceCollisionTileReader reader,
    const void *context,
    const BounceDynThornsSource *thorns
)
{
    int32_t first_tile_x;
    int32_t first_tile_y;
    int32_t end_tile_x;
    int32_t end_tile_y;
    int32_t tile_x;
    int32_t tile_y;

    if (reader == NULL
        || (player_half_size != BOUNCE_REGULAR_HALF_SIZE
            && player_half_size != BOUNCE_BIG_HALF_SIZE))
        return false;

    first_tile_x = java_int32_divide_by_tile_size(
        java_int32_subtract(player_x, player_half_size)
    );
    first_tile_y = java_int32_divide_by_tile_size(
        java_int32_subtract(player_y, player_half_size)
    );
    end_tile_x = java_int32_add(
        java_int32_divide_by_tile_size(
            java_int32_add(
                java_int32_subtract(player_x, 1),
                player_half_size
            )
        ),
        1
    );
    end_tile_y = java_int32_add(
        java_int32_divide_by_tile_size(
            java_int32_add(
                java_int32_subtract(player_y, 1),
                player_half_size
            )
        ),
        1
    );

    /* Preserve Java's signed loop ordering, including empty wrapped ranges. */
    for (tile_x = first_tile_x;
         tile_x < end_tile_x;
         tile_x = java_int32_add(tile_x, 1)) {
        for (tile_y = first_tile_y;
             tile_y < end_tile_y;
             tile_y = java_int32_add(tile_y, 1)) {
            if (!collision_tile_fits(
                    player_x,
                    player_y,
                    player_half_size,
                    tile_x,
                    tile_y,
                    level_width,
                    level_height,
                    reader,
                    context,
                    thorns
                ))
                return false;
        }
    }
    return true;
}

bool bounce_collision_query(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    const BounceRuntimeLevelTiles *runtime
)
{
    BounceDynThornsSource thorns;

    if (runtime == NULL)
        return false;
    /*
     * The animated w owned by the runtime copy. This is a borrowed const view:
     * the query reads it and never advances it.
     */
    if (bounce_dyn_thorns_source(
            bounce_runtime_level_tiles_dyn_thorns(runtime),
            &thorns
        ) != 0)
        return false;
    return bounce_collision_query_source(
        player_x,
        player_y,
        player_half_size,
        bounce_runtime_level_tiles_width(runtime),
        bounce_runtime_level_tiles_height(runtime),
        collision_tile_read_runtime,
        runtime,
        &thorns
    );
}

bool bounce_collision_query_level(
    int32_t player_x,
    int32_t player_y,
    int32_t player_half_size,
    const BounceLevel *level
)
{
    BounceDynThornsSource thorns;

    if (level == NULL)
        return false;
    /*
     * The INITIAL w decoded straight from the immutable payload. This is the
     * correct state at the e.java:127 constructor boundary because the only
     * UpdateDynThorns call site is e.java:282-283, inside Tick. BounceLevel is
     * neither copied nor modified.
     */
    if (bounce_level_dyn_thorns_source(level, &thorns) != 0)
        return false;
    return bounce_collision_query_source(
        player_x,
        player_y,
        player_half_size,
        bounce_level_width(level),
        bounce_level_height(level),
        collision_tile_read_level,
        level,
        &thorns
    );
}
