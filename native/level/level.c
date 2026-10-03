#include "level.h"

#include "dyn_thorns.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    BOUNCE_LEVEL_TILE_SIZE = 12,
    BOUNCE_LEVEL_INITIAL_CENTER_OFFSET = 6,
    BOUNCE_LEVEL_REGULAR_BALL_SIZE = 12,
    BOUNCE_LEVEL_BIG_BALL_SIZE = 16
};

struct BounceLevel {
    uint8_t initial_X;
    uint8_t initial_Y;
    uint8_t isBigBall;
    uint8_t exit_X;
    uint8_t exit_Y;
    uint8_t HoopsTotal;
    size_t width;
    size_t height;
    uint8_t *tiles;
    size_t tile_count;
    uint8_t thorn_count;
    uint8_t *thorn_payload;
    size_t thorn_payload_length;
};

struct BounceRuntimeLevelTiles {
    size_t width;
    size_t height;
    size_t tile_count;
    uint16_t *tiles;
    /*
     * Dynamic-thorn animation state, owned here because b.java advances w and
     * ae once per e.Tick() (e.java:282-283) and the native runtime copy is the
     * equivalent of the original's single mutable LevelTiles array. BounceLevel
     * keeps the payload opaque and immutable.
     */
    struct BounceDynThorns dyn_thorns;
};

static int bounce_level_add_size(size_t left, size_t right, size_t *result)
{
    if (right > SIZE_MAX - left)
        return -1;

    *result = left + right;
    return 0;
}

static int bounce_level_mul_size(size_t left, size_t right, size_t *result)
{
    if (left != 0u && right > SIZE_MAX / left)
        return -1;

    *result = left * right;
    return 0;
}

BounceLevel *bounce_level_parse(const uint8_t *data, size_t size)
{
    BounceLevel *level;
    size_t width;
    size_t height;
    size_t tile_count;
    size_t tile_offset;
    size_t tail_count_offset;
    size_t remaining_bytes;
    size_t thorn_payload_length;
    size_t expected_size;
    uint8_t thorn_count;
    uint8_t *tiles;
    uint8_t *thorn_payload = NULL;

    if (data == NULL || size < 8u)
        return NULL;

    width = (size_t)data[6];
    height = (size_t)data[7];
    if (width == 0u || height == 0u)
        return NULL;
    if (height > SIZE_MAX / width)
        return NULL;
    tile_count = width * height;

    if (bounce_level_add_size(8u, tile_count, &tile_offset) != 0
        || bounce_level_add_size(tile_offset, 1u, &tail_count_offset) != 0
        || size < tail_count_offset)
        return NULL;

    remaining_bytes = size - tail_count_offset;
    thorn_count = data[tile_offset];
    if (bounce_level_mul_size(
            (size_t)thorn_count,
            8u,
            &thorn_payload_length
        ) != 0)
        return NULL;

    if (remaining_bytes != thorn_payload_length
        || bounce_level_add_size(
            tail_count_offset,
            thorn_payload_length,
            &expected_size
        ) != 0
        || size != expected_size)
        return NULL;

    tiles = (uint8_t *)malloc(tile_count);
    if (tiles == NULL)
        return NULL;
    memcpy(tiles, data + 8u, tile_count);

    if (thorn_payload_length != 0u) {
        thorn_payload = (uint8_t *)malloc(thorn_payload_length);
        if (thorn_payload == NULL) {
            free(tiles);
            return NULL;
        }
        memcpy(
            thorn_payload,
            data + tail_count_offset,
            thorn_payload_length
        );
    }

    level = (BounceLevel *)calloc(1, sizeof *level);
    if (level == NULL) {
        free(thorn_payload);
        free(tiles);
        return NULL;
    }

    level->initial_X = data[0];
    level->initial_Y = data[1];
    level->isBigBall = data[2];
    level->exit_X = data[3];
    level->exit_Y = data[4];
    level->HoopsTotal = data[5];
    level->width = width;
    level->height = height;
    level->tiles = tiles;
    level->tile_count = tile_count;
    level->thorn_count = thorn_count;
    level->thorn_payload = thorn_payload;
    level->thorn_payload_length = thorn_payload_length;
    return level;
}

void bounce_level_destroy(BounceLevel *level)
{
    if (level == NULL)
        return;

    free(level->thorn_payload);
    free(level->tiles);
    free(level);
}

uint8_t bounce_level_initial_x(const BounceLevel *level)
{
    return level == NULL ? 0u : level->initial_X;
}

uint8_t bounce_level_initial_y(const BounceLevel *level)
{
    return level == NULL ? 0u : level->initial_Y;
}

uint8_t bounce_level_is_big_ball(const BounceLevel *level)
{
    return level == NULL ? 0u : level->isBigBall;
}

uint8_t bounce_level_exit_x(const BounceLevel *level)
{
    return level == NULL ? 0u : level->exit_X;
}

uint8_t bounce_level_exit_y(const BounceLevel *level)
{
    return level == NULL ? 0u : level->exit_Y;
}

uint8_t bounce_level_hoops_total(const BounceLevel *level)
{
    return level == NULL ? 0u : level->HoopsTotal;
}

uint32_t bounce_level_width(const BounceLevel *level)
{
    return level == NULL ? 0u : (uint32_t)level->width;
}

uint32_t bounce_level_height(const BounceLevel *level)
{
    return level == NULL ? 0u : (uint32_t)level->height;
}

int bounce_level_derive_initial_player_input(
    const BounceLevel *level,
    int *player_x_out,
    int *player_y_out,
    int *ball_size_out
)
{
    if (player_x_out != NULL)
        *player_x_out = 0;
    if (player_y_out != NULL)
        *player_y_out = 0;
    if (ball_size_out != NULL)
        *ball_size_out = 0;

    if (level == NULL
        || player_x_out == NULL
        || player_y_out == NULL
        || ball_size_out == NULL)
        return -1;

    *player_x_out = (int)level->initial_X
        * BOUNCE_LEVEL_TILE_SIZE
        + BOUNCE_LEVEL_INITIAL_CENTER_OFFSET;
    *player_y_out = (int)level->initial_Y
        * BOUNCE_LEVEL_TILE_SIZE
        + BOUNCE_LEVEL_INITIAL_CENTER_OFFSET;
    *ball_size_out = level->isBigBall == 0u
        ? BOUNCE_LEVEL_REGULAR_BALL_SIZE
        : BOUNCE_LEVEL_BIG_BALL_SIZE;
    return 0;
}

int bounce_player_constructor_state_init(
    int param_x,
    int param_y,
    int ball_size,
    BouncePlayerConstructorState *state_out
)
{
    if (state_out == NULL)
        return -1;

    state_out->TODO_unkX = param_x;
    state_out->TODO_unkY = param_y;
    state_out->l = 0;
    state_out->o = 0;
    state_out->t = 0;
    state_out->m = false;
    state_out->v = false;
    state_out->u = false;
    state_out->q = 0;
    state_out->TODO_somePowerUp1 = 0;
    state_out->powerUpGravity = 0;
    state_out->TODO_somePowerUp3 = 0;
    state_out->C = 0;
    state_out->z = 0;
    state_out->w = 0;
    state_out->j = true;

    if (ball_size == BOUNCE_LEVEL_REGULAR_BALL_SIZE) {
        state_out->ballSize = BOUNCE_LEVEL_REGULAR_BALL_SIZE;
        state_out->p = 6;
        state_out->current_ball_mode =
            BOUNCE_PLAYER_BALL_MODE_REGULAR;
    } else {
        state_out->ballSize = BOUNCE_LEVEL_BIG_BALL_SIZE;
        state_out->p = 8;
        state_out->current_ball_mode =
            BOUNCE_PLAYER_BALL_MODE_BIG;
    }

    return 0;
}

int bounce_player_post_constructor_apply(
    BouncePlayerConstructorState *state,
    int param4,
    int param5
)
{
    if (state == NULL)
        return -1;

    state->l = param4;
    state->o = param5;
    return 0;
}

int bounce_player_tile_coordinate_apply(
    const BouncePlayerConstructorState *state,
    int tile_x,
    int tile_y,
    int *d_out,
    int *c_out,
    int *b_out
)
{
    if (d_out != NULL)
        *d_out = 0;
    if (c_out != NULL)
        *c_out = 0;
    if (b_out != NULL)
        *b_out = 0;

    if (state == NULL || d_out == NULL || c_out == NULL || b_out == NULL)
        return -1;

    *d_out = tile_x;
    *c_out = tile_y;
    *b_out = state->ballSize;
    return 0;
}

int bounce_level_initialize_new_game_player_data(
    const BounceLevel *level,
    BouncePlayerConstructorState *state_out,
    int *d_out,
    int *c_out,
    int *b_out
)
{
    BouncePlayerConstructorState state;
    int player_x;
    int player_y;
    int ball_size;
    int d;
    int c;
    int b;

    if (state_out != NULL)
        memset(state_out, 0, sizeof *state_out);
    if (d_out != NULL)
        *d_out = 0;
    if (c_out != NULL)
        *c_out = 0;
    if (b_out != NULL)
        *b_out = 0;

    if (level == NULL
        || state_out == NULL
        || d_out == NULL
        || c_out == NULL
        || b_out == NULL)
        return -1;

    if (bounce_level_derive_initial_player_input(
            level,
            &player_x,
            &player_y,
            &ball_size
        ) != 0)
        return -1;

    if (bounce_player_constructor_state_init(
            player_x,
            player_y,
            ball_size,
            &state
        ) != 0)
        return -1;

    if (bounce_player_post_constructor_apply(&state, 0, 0) != 0)
        return -1;

    if (bounce_player_tile_coordinate_apply(
            &state,
            (int)bounce_level_initial_x(level),
            (int)bounce_level_initial_y(level),
            &d,
            &c,
            &b
        ) != 0)
        return -1;

    *state_out = state;
    *d_out = d;
    *c_out = c;
    *b_out = b;
    return 0;
}

size_t bounce_level_tile_count(const BounceLevel *level)
{
    return level == NULL ? 0u : level->tile_count;
}

uint32_t bounce_level_thorn_count(const BounceLevel *level)
{
    return level == NULL ? 0u : (uint32_t)level->thorn_count;
}

int bounce_level_get_tile(
    const BounceLevel *level,
    int x,
    int y,
    uint8_t *tile_out
)
{
    size_t index;

    if (level == NULL
        || tile_out == NULL
        || x < 0
        || y < 0
        || (size_t)x >= level->width
        || (size_t)y >= level->height)
        return -1;

    index = (size_t)y * level->width + (size_t)x;
    *tile_out = level->tiles[index];
    return 0;
}

int bounce_level_get_thorn_payload(
    const BounceLevel *level,
    const uint8_t **payload_out,
    size_t *length_out
)
{
    if (payload_out != NULL)
        *payload_out = NULL;
    if (length_out != NULL)
        *length_out = 0u;

    if (level == NULL || payload_out == NULL || length_out == NULL)
        return -1;

    *payload_out = level->thorn_payload;
    *length_out = level->thorn_payload_length;
    return 0;
}

uint16_t bounce_level_tile_value_from_raw(uint8_t raw_value)
{
    return (uint16_t)raw_value;
}

int32_t bounce_level_tile_value_water(uint16_t value)
{
    return (int32_t)value & INT32_C(0x40);
}

int32_t bounce_level_tile_value_without_water(uint16_t value)
{
    return (int32_t)value & ~INT32_C(0x40);
}

int32_t bounce_level_tile_value_without_dirty(uint16_t value)
{
    return (int32_t)value & ~INT32_C(0x80);
}

int32_t bounce_level_tile_value_persistence_normalize(uint16_t value)
{
    return (int32_t)value & INT32_C(0xff7f);
}

int32_t bounce_level_tile_value_persistence_without_water(uint16_t value)
{
    return bounce_level_tile_value_persistence_normalize(value)
        & ~INT32_C(0x40);
}

uint16_t bounce_level_tile_value_set_dirty(uint16_t value)
{
    return (uint16_t)((int32_t)value | INT32_C(0x80));
}

uint16_t bounce_level_tile_value_clear_dirty(uint16_t value)
{
    return (uint16_t)((int32_t)value & ~INT32_C(0x80));
}

uint16_t bounce_level_tile_value_reconstruct(
    uint16_t old_value,
    uint16_t literal
)
{
    return (uint16_t)(
        (int32_t)literal
            | ((int32_t)old_value & INT32_C(0x40))
    );
}

int bounce_level_tile_value_has_dirty(uint16_t value)
{
    return ((int32_t)value & INT32_C(0x80)) != 0;
}

BounceRuntimeLevelTiles *bounce_runtime_level_tiles_create(
    const BounceLevel *level
)
{
    BounceRuntimeLevelTiles *runtime;
    size_t index;

    if (level == NULL || level->tile_count == 0u)
        return NULL;
    if (level->tile_count > SIZE_MAX / sizeof(uint16_t))
        return NULL;

    runtime = (BounceRuntimeLevelTiles *)calloc(1, sizeof *runtime);
    if (runtime == NULL)
        return NULL;

    runtime->tiles = (uint16_t *)malloc(
        level->tile_count * sizeof *runtime->tiles
    );
    if (runtime->tiles == NULL) {
        free(runtime);
        return NULL;
    }

    runtime->width = level->width;
    runtime->height = level->height;
    runtime->tile_count = level->tile_count;
    for (index = 0u; index < runtime->tile_count; ++index)
        runtime->tiles[index] = bounce_level_tile_value_from_raw(
            level->tiles[index]
        );
    if (bounce_dyn_thorns_init_from_level(&runtime->dyn_thorns, level) != 0) {
        free(runtime->tiles);
        free(runtime);
        return NULL;
    }
    return runtime;
}

void bounce_runtime_level_tiles_destroy(
    BounceRuntimeLevelTiles *runtime
)
{
    if (runtime == NULL)
        return;

    bounce_dyn_thorns_reset(&runtime->dyn_thorns);
    free(runtime->tiles);
    free(runtime);
}

uint32_t bounce_runtime_level_tiles_width(
    const BounceRuntimeLevelTiles *runtime
)
{
    return runtime == NULL ? 0u : (uint32_t)runtime->width;
}

uint32_t bounce_runtime_level_tiles_height(
    const BounceRuntimeLevelTiles *runtime
)
{
    return runtime == NULL ? 0u : (uint32_t)runtime->height;
}

const struct BounceDynThorns *bounce_runtime_level_tiles_dyn_thorns(
    const BounceRuntimeLevelTiles *runtime
)
{
    return runtime == NULL ? NULL : &runtime->dyn_thorns;
}

int bounce_runtime_level_tiles_update_dyn_thorns(
    BounceRuntimeLevelTiles *runtime
)
{
    if (runtime == NULL)
        return -1;

    /*
     * e.java:282-283 guards the call on DynThornsCount != 0. The owner is this
     * runtime object, so no payload decoding, copying, or allocation happens
     * here; BounceLevel stays pristine and the query path stays untouched.
     */
    if (runtime->dyn_thorns.count == 0u)
        return 0;
    if (bounce_dyn_thorns_update(&runtime->dyn_thorns) != 0)
        return -1;
    return 1;
}

int bounce_runtime_level_tiles_restore_dyn_thorns(
    BounceRuntimeLevelTiles *runtime,
    uint32_t count,
    const struct BounceVec2S *w_values,
    const struct BounceVec2S *ae_values
)
{
    if (runtime == NULL)
        return -1;
    /*
     * The records were created by bounce_runtime_level_tiles_create() when the
     * runtime was built, so this only installs values into storage that already
     * exists. It deliberately does not call bounce_dyn_thorns_update(): e.AddScore()
     * does not call UpdateDynThorns() either, and doing so would advance every
     * thorn one step past the restored animation phase.
     */
    return bounce_dyn_thorns_restore_offsets(
        &runtime->dyn_thorns,
        count,
        w_values,
        ae_values
    );
}

int bounce_runtime_level_tiles_get(
    const BounceRuntimeLevelTiles *runtime,
    int x,
    int y,
    uint16_t *value_out
)
{
    size_t index;

    if (value_out != NULL)
        *value_out = 0u;
    if (runtime == NULL
        || value_out == NULL
        || x < 0
        || y < 0
        || (size_t)x >= runtime->width
        || (size_t)y >= runtime->height)
        return -1;

    index = (size_t)y * runtime->width + (size_t)x;
    *value_out = runtime->tiles[index];
    return 0;
}

int bounce_runtime_level_tiles_set(
    BounceRuntimeLevelTiles *runtime,
    int x,
    int y,
    uint16_t value
)
{
    size_t index;

    if (runtime == NULL
        || x < 0
        || y < 0
        || (size_t)x >= runtime->width
        || (size_t)y >= runtime->height)
        return -1;

    index = (size_t)y * runtime->width + (size_t)x;
    runtime->tiles[index] = value;
    return 0;
}
