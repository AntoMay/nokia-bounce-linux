#include "visual_assets.h"

#include "../assets/asset.h"
#include "../renderer/image.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    BOUNCE_ATLAS_TILE_SIZE = 12,
    BOUNCE_ATLAS_COLUMNS = 4,
    BOUNCE_ATLAS_ROWS = 6,
    BOUNCE_SPRITE_COUNT = 67,
    BOUNCE_BALL_HALF_SIZE = 6,
    /*
     * b.java SpriteIDs: BALL = 47, POPPED_BALL = 48, BIG_BALL = 49. All three
     * are named here as of STEP 14B-11. The previous revision of this comment
     * read "Only the two live-ball slots are named here; the popped slot is
     * deliberately not reachable from this layer because e.java:214-215 selects
     * it from a separate z == 2 arm, which is outside this boundary."
     *
     * That was accurate about the assets layer and incomplete as a justification:
     * the selection is not made here, it is made by the caller, which supplies
     * the death state. STEP 14B-11 adds that parameter, so the popped slot is
     * now reachable from this layer. The original wording is preserved above
     * because earlier reconciliation sections quote it.
     */
    BOUNCE_VISUAL_SPRITE_BALL = 47,
    BOUNCE_VISUAL_SPRITE_POPPED_BALL = 48,
    BOUNCE_VISUAL_SPRITE_BIG_BALL = 49,
    /* f.java:169 sets ballSize = 16 with p = 8; f.java:210 sets 12 with 6. */
    BOUNCE_VISUAL_BIG_BALL_SIZE = 16
};

typedef enum BounceVisualTransform {
    BOUNCE_VISUAL_TRANSFORM_NONE = 0,
    BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL,
    BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL,
    BOUNCE_VISUAL_TRANSFORM_FLIP_BOTH,
    BOUNCE_VISUAL_TRANSFORM_ROTATE_90,
    BOUNCE_VISUAL_TRANSFORM_ROTATE_180,
    BOUNCE_VISUAL_TRANSFORM_ROTATE_270
} BounceVisualTransform;

struct BounceVisualAssets {
    BounceImage *atlas;
    BounceImage *sprites[BOUNCE_SPRITE_COUNT];
    BounceImage *hud_ball;
    BounceImage *hud_hoop;
    BounceImage *deflater[4];
    BounceImage *pumper[4];
    BounceImage *gravity[4];
    BounceImage *jump[4];
    BounceImage *splash;
    BounceImage *nokiagames;
    BounceImage *icon;
};

static const int tile_front_sprites[16] = {
    35, 36, 17, 19, 43, 44, 25, 27,
    31, 32, 13, 15, 39, 40, 21, 23
};

static const int tile_back_sprites[16] = {
    33, 34, 18, 20, 41, 42, 26, 28,
    29, 30, 14, 16, 37, 38, 22, 24
};

static uint32_t visual_argb(
    uint32_t alpha,
    uint32_t red,
    uint32_t green,
    uint32_t blue
)
{
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static int visual_path(
    const char *root,
    const char *relative,
    char *out,
    size_t out_size
)
{
    int written;

    if (root == NULL || relative == NULL || out == NULL || out_size == 0u)
        return -1;
    written = snprintf(out, out_size, "%s/%s", root, relative);
    if (written < 0 || (size_t)written >= out_size)
        return -1;
    return 0;
}

static int visual_fill(
    BounceImage *image,
    uint32_t pixel
)
{
    uint32_t y;

    if (image == NULL)
        return -1;
    for (y = 0u; y < bounce_image_height(image); ++y) {
        uint32_t x;
        for (x = 0u; x < bounce_image_width(image); ++x) {
            if (bounce_image_set_pixel(image, (int)x, (int)y, pixel) != 0)
                return -1;
        }
    }
    return 0;
}

static int visual_fill_rect(
    BounceImage *image,
    int x,
    int y,
    int width,
    int height,
    uint32_t pixel
)
{
    int py;

    if (image == NULL || x < 0 || y < 0 || width < 0 || height < 0)
        return -1;
    for (py = y; py < y + height; ++py) {
        int px;
        for (px = x; px < x + width; ++px) {
            if (bounce_image_set_pixel(image, px, py, pixel) != 0)
                return -1;
        }
    }
    return 0;
}

static void visual_source_coordinate(
    BounceVisualTransform transform,
    int destination_x,
    int destination_y,
    int width,
    int height,
    int *source_x_out,
    int *source_y_out
)
{
    switch (transform) {
        case BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL:
            *source_x_out = width - 1 - destination_x;
            *source_y_out = destination_y;
            break;
        case BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL:
            *source_x_out = destination_x;
            *source_y_out = height - 1 - destination_y;
            break;
        case BOUNCE_VISUAL_TRANSFORM_FLIP_BOTH:
            *source_x_out = width - 1 - destination_x;
            *source_y_out = height - 1 - destination_y;
            break;
        case BOUNCE_VISUAL_TRANSFORM_ROTATE_90:
            *source_x_out = destination_y;
            *source_y_out = height - 1 - destination_x;
            break;
        case BOUNCE_VISUAL_TRANSFORM_ROTATE_180:
            *source_x_out = width - 1 - destination_x;
            *source_y_out = height - 1 - destination_y;
            break;
        case BOUNCE_VISUAL_TRANSFORM_ROTATE_270:
            *source_x_out = width - 1 - destination_y;
            *source_y_out = destination_x;
            break;
        case BOUNCE_VISUAL_TRANSFORM_NONE:
        default:
            *source_x_out = destination_x;
            *source_y_out = destination_y;
            break;
    }
}

static BounceImage *visual_tile_from_atlas(
    const BounceImage *atlas,
    int atlas_x,
    int atlas_y,
    uint32_t background,
    BounceVisualTransform transform
)
{
    BounceImage *image;
    int x;
    int y;
    int source_left = atlas_x * BOUNCE_ATLAS_TILE_SIZE;
    int source_top = atlas_y * BOUNCE_ATLAS_TILE_SIZE;

    if (atlas == NULL
        || atlas_x < 0
        || atlas_x >= BOUNCE_ATLAS_COLUMNS
        || atlas_y < 0
        || atlas_y >= BOUNCE_ATLAS_ROWS)
        return NULL;
    image = bounce_image_create(12u, 12u);
    if (image == NULL || visual_fill(image, background) != 0) {
        bounce_image_destroy(image);
        return NULL;
    }
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            int source_x;
            int source_y;
            uint32_t pixel;
            visual_source_coordinate(
                transform,
                x,
                y,
                12,
                12,
                &source_x,
                &source_y
            );
            if (bounce_image_get_pixel(
                    atlas,
                    source_left + source_x,
                    source_top + source_y,
                    &pixel
                ) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(image, x, y, pixel) != 0)
                goto failure;
        }
    }
    return image;

failure:
    bounce_image_destroy(image);
    return NULL;
}

static BounceImage *visual_transform_image(
    const BounceImage *source,
    BounceVisualTransform transform
)
{
    BounceImage *image;
    int width;
    int height;
    int x;
    int y;

    if (source == NULL)
        return NULL;
    width = (int)bounce_image_width(source);
    height = (int)bounce_image_height(source);
    image = bounce_image_create((uint32_t)width, (uint32_t)height);
    if (image == NULL)
        return NULL;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            int source_x;
            int source_y;
            uint32_t pixel;
            visual_source_coordinate(
                transform,
                x,
                y,
                width,
                height,
                &source_x,
                &source_y
            );
            if (bounce_image_get_pixel(
                    source,
                    source_x,
                    source_y,
                    &pixel
                ) != 0
                || bounce_image_set_pixel(image, x, y, pixel) != 0) {
                bounce_image_destroy(image);
                return NULL;
            }
        }
    }
    return image;
}

static BounceImage *visual_mirror16_ball(const BounceImage *atlas)
{
    BounceImage *image;
    int x;
    int y;

    image = bounce_image_create(16u, 16u);
    if (image == NULL || visual_fill(image, 0u) != 0)
        goto failure;
    for (y = 0; y < 16; ++y) {
        for (x = 0; x < 16; ++x) {
            int source_x;
            int source_y;
            uint32_t pixel;
            if (x < 8 && y < 8) {
                source_x = x + 4;
                source_y = y + 4;
            } else if (x >= 8 && y < 8) {
                source_x = 11 - (x - 8);
                source_y = y + 4;
            } else if (x < 8 && y >= 8) {
                source_x = x + 4;
                source_y = 11 - (y - 8);
            } else {
                source_x = 11 - (x - 8);
                source_y = 11 - (y - 8);
            }
            if (bounce_image_get_pixel(
                    atlas,
                    3 * BOUNCE_ATLAS_TILE_SIZE + source_x,
                    source_y,
                    &pixel
                ) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(image, x, y, pixel) != 0)
                goto failure;
        }
    }
    return image;

failure:
    bounce_image_destroy(image);
    return NULL;
}

static BounceImage *visual_exit_tile(const BounceImage *atlas)
{
    BounceImage *image;
    int x;
    int y;
    const uint32_t sky = visual_argb(255u, 176u, 224u, 240u);
    const uint32_t outer = visual_argb(255u, 252u, 157u, 158u);
    const uint32_t middle = visual_argb(255u, 227u, 58u, 63u);
    const uint32_t inner = visual_argb(255u, 194u, 132u, 142u);

    image = bounce_image_create(24u, 48u);
    if (image == NULL || visual_fill(image, sky) != 0)
        goto failure;
    if (visual_fill_rect(image, 4, 0, 16, 48, outer) != 0
        || visual_fill_rect(image, 6, 0, 10, 48, middle) != 0
        || visual_fill_rect(image, 10, 0, 4, 48, inner) != 0)
        goto failure;
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(
                    atlas,
                    2 * BOUNCE_ATLAS_TILE_SIZE + x,
                    3 * BOUNCE_ATLAS_TILE_SIZE + y,
                    &pixel
                ) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(image, x, y, pixel) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(
                    image,
                    23 - x,
                    y,
                    pixel
                ) != 0)
                goto failure;
        }
    }
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(
                    atlas,
                    2 * BOUNCE_ATLAS_TILE_SIZE + x,
                    3 * BOUNCE_ATLAS_TILE_SIZE + y,
                    &pixel
                ) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(
                    image,
                    x,
                    23 - y,
                    pixel
                ) != 0)
                goto failure;
            if (((pixel >> 24) & 0xffu) != 0u
                && bounce_image_set_pixel(
                    image,
                    23 - x,
                    23 - y,
                    pixel
                ) != 0)
                goto failure;
        }
    }
    return image;

failure:
    bounce_image_destroy(image);
    return NULL;
}

static int visual_set_tile(
    BounceVisualAssets *assets,
    int sprite_index,
    int atlas_x,
    int atlas_y,
    uint32_t background,
    BounceVisualTransform transform
)
{
    BounceImage *image;

    if (assets == NULL
        || sprite_index < 0
        || sprite_index >= BOUNCE_SPRITE_COUNT)
        return -1;
    image = visual_tile_from_atlas(
        assets->atlas,
        atlas_x,
        atlas_y,
        background,
        transform
    );
    if (image == NULL)
        return -1;
    bounce_image_destroy(assets->sprites[sprite_index]);
    assets->sprites[sprite_index] = image;
    return 0;
}

static int visual_set_transformed(
    BounceVisualAssets *assets,
    int sprite_index,
    int source_index,
    BounceVisualTransform transform
)
{
    BounceImage *image;

    if (assets == NULL
        || sprite_index < 0
        || sprite_index >= BOUNCE_SPRITE_COUNT
        || source_index < 0
        || source_index >= BOUNCE_SPRITE_COUNT
        || assets->sprites[source_index] == NULL)
        return -1;
    image = visual_transform_image(
        assets->sprites[source_index],
        transform
    );
    if (image == NULL)
        return -1;
    bounce_image_destroy(assets->sprites[sprite_index]);
    assets->sprites[sprite_index] = image;
    return 0;
}

static int visual_build_directional_assets(BounceVisualAssets *assets)
{
    static const BounceVisualTransform transforms[4] = {
        BOUNCE_VISUAL_TRANSFORM_NONE,
        BOUNCE_VISUAL_TRANSFORM_ROTATE_270,
        BOUNCE_VISUAL_TRANSFORM_ROTATE_180,
        BOUNCE_VISUAL_TRANSFORM_ROTATE_90
    };
    BounceImage *base;
    int index;

    base = assets->sprites[50];
    for (index = 0; index < 4; ++index) {
        assets->deflater[index] = visual_transform_image(base, transforms[index]);
        if (assets->deflater[index] == NULL)
            return -1;
    }
    base = assets->sprites[51];
    for (index = 0; index < 4; ++index) {
        assets->pumper[index] = visual_transform_image(base, transforms[index]);
        if (assets->pumper[index] == NULL)
            return -1;
    }
    base = assets->sprites[52];
    for (index = 0; index < 4; ++index) {
        assets->gravity[index] = visual_transform_image(base, transforms[index]);
        if (assets->gravity[index] == NULL)
            return -1;
    }
    base = assets->sprites[54];
    for (index = 0; index < 4; ++index) {
        assets->jump[index] = visual_transform_image(base, transforms[index]);
        if (assets->jump[index] == NULL)
            return -1;
    }
    return 0;
}

static int visual_build_sprites(BounceVisualAssets *assets)
{
    const uint32_t transparent = 0u;
    const uint32_t sky = visual_argb(255u, 176u, 224u, 240u);
    const uint32_t water = visual_argb(255u, 16u, 96u, 176u);
    int index;

    if (visual_set_tile(assets, 0, 1, 0, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 1, 1, 2, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 2, 0, 3, sky,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 3, 2,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 4, 2,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_90) != 0
        || visual_set_transformed(assets, 5, 2,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_tile(assets, 6, 0, 3, water,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 7, 6,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 8, 6,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_90) != 0
        || visual_set_transformed(assets, 9, 6,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_tile(assets, 10, 0, 4, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 11, 3, 4, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0)
        return -1;

    assets->sprites[12] = visual_exit_tile(assets->atlas);
    if (assets->sprites[12] == NULL)
        return -1;
    if (visual_set_tile(assets, 14, 0, 5, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 13, 14,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 15, 13,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 16, 14,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_tile(assets, 18, 1, 5, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 17, 18,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 19, 17,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 20, 18,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_tile(assets, 22, 2, 5, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 21, 22,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 23, 21,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 24, 22,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_tile(assets, 26, 3, 5, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 25, 26,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 27, 25,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 28, 26,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0)
        return -1;

    if (visual_set_transformed(assets, 29, 14,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_transformed(assets, 30, 29,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 31, 29,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 32, 30,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 33, 18,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_transformed(assets, 34, 33,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 35, 33,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 36, 34,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 37, 22,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_transformed(assets, 38, 37,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 39, 37,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 40, 38,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 41, 26,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_transformed(assets, 42, 41,
            BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL) != 0
        || visual_set_transformed(assets, 43, 41,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0
        || visual_set_transformed(assets, 44, 42,
            BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL) != 0)
        return -1;

    if (visual_set_tile(assets, 45, 3, 3, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 46, 1, 3, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 47, 2, 0, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 48, 0, 1, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0)
        return -1;
    assets->sprites[49] = visual_mirror16_ball(assets->atlas);
    if (assets->sprites[49] == NULL
        || visual_set_tile(assets, 50, 3, 1, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 51, 2, 4, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 52, 3, 2, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 53, 1, 1, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 54, 2, 2, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_tile(assets, 55, 0, 0, sky,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 56, 55,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_90) != 0
        || visual_set_transformed(assets, 57, 55,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_180) != 0
        || visual_set_transformed(assets, 58, 55,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_tile(assets, 59, 0, 0, water,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 60, 59,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_90) != 0
        || visual_set_transformed(assets, 61, 59,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_180) != 0
        || visual_set_transformed(assets, 62, 59,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0
        || visual_set_tile(assets, 63, 0, 2, transparent,
            BOUNCE_VISUAL_TRANSFORM_NONE) != 0
        || visual_set_transformed(assets, 64, 63,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_90) != 0
        || visual_set_transformed(assets, 65, 63,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_180) != 0
        || visual_set_transformed(assets, 66, 63,
            BOUNCE_VISUAL_TRANSFORM_ROTATE_270) != 0)
        return -1;

    assets->hud_ball = visual_tile_from_atlas(
        assets->atlas,
        2,
        1,
        transparent,
        BOUNCE_VISUAL_TRANSFORM_NONE
    );
    assets->hud_hoop = visual_tile_from_atlas(
        assets->atlas,
        1,
        4,
        transparent,
        BOUNCE_VISUAL_TRANSFORM_NONE
    );
    if (assets->hud_ball == NULL || assets->hud_hoop == NULL)
        return -1;
    for (index = 0; index < 16; ++index) {
        if (tile_front_sprites[index] < 0
            || tile_front_sprites[index] >= BOUNCE_SPRITE_COUNT
            || tile_back_sprites[index] < 0
            || tile_back_sprites[index] >= BOUNCE_SPRITE_COUNT)
            return -1;
    }
    return visual_build_directional_assets(assets);
}

static BounceImage *visual_decode(
    const char *path
)
{
    BounceImage *image = NULL;
    if (bounce_asset_decode_file(path, &image) != BOUNCE_ASSET_DECODE_OK)
        return NULL;
    return image;
}

BounceVisualAssets *bounce_visual_assets_create(const char *resource_root)
{
    BounceVisualAssets *assets;
    char path[512];

    if (resource_root == NULL || resource_root[0] == '\0')
        return NULL;
    assets = (BounceVisualAssets *)calloc(1, sizeof *assets);
    if (assets == NULL)
        return NULL;
    if (visual_path(resource_root, "icons/objects_nm.png", path, sizeof path) != 0)
        goto failure;
    assets->atlas = visual_decode(path);
    if (visual_path(resource_root, "icons/bouncesplash.png", path, sizeof path) != 0)
        goto failure;
    assets->splash = visual_decode(path);
    if (visual_path(resource_root, "icons/nokiagames.png", path, sizeof path) != 0)
        goto failure;
    assets->nokiagames = visual_decode(path);
    if (visual_path(resource_root, "icons/icon.png", path, sizeof path) != 0)
        goto failure;
    assets->icon = visual_decode(path);
    if (assets->atlas == NULL
        || assets->splash == NULL
        || assets->nokiagames == NULL
        || assets->icon == NULL
        || visual_build_sprites(assets) != 0)
        goto failure;
    return assets;

failure:
    bounce_visual_assets_destroy(assets);
    return NULL;
}

void bounce_visual_assets_destroy(BounceVisualAssets *assets)
{
    int index;

    if (assets == NULL)
        return;
    for (index = 0; index < BOUNCE_SPRITE_COUNT; ++index)
        bounce_image_destroy(assets->sprites[index]);
    bounce_image_destroy(assets->hud_ball);
    bounce_image_destroy(assets->hud_hoop);
    for (index = 0; index < 4; ++index) {
        bounce_image_destroy(assets->deflater[index]);
        bounce_image_destroy(assets->pumper[index]);
        bounce_image_destroy(assets->gravity[index]);
        bounce_image_destroy(assets->jump[index]);
    }
    bounce_image_destroy(assets->splash);
    bounce_image_destroy(assets->nokiagames);
    bounce_image_destroy(assets->icon);
    bounce_image_destroy(assets->atlas);
    free(assets);
}

static int visual_draw_sprite(
    BounceRenderer *renderer,
    const BounceImage *sprite,
    int destination_x,
    int destination_y
)
{
    if (renderer == NULL || sprite == NULL)
        return -1;
    return bounce_renderer_blit_region_transparent(
        renderer,
        sprite,
        0,
        0,
        (int)bounce_image_width(sprite),
        (int)bounce_image_height(sprite),
        destination_x,
        destination_y
    );
}

static int visual_surface_has_nonblack(const BounceSurface *surface)
{
    uint32_t y;

    if (surface == NULL)
        return 0;
    for (y = 0u; y < bounce_surface_height(surface); ++y) {
        uint32_t x;
        for (x = 0u; x < bounce_surface_width(surface); ++x) {
            uint32_t pixel;
            if (bounce_surface_get_pixel(
                    surface,
                    (int)x,
                    (int)y,
                    &pixel
                ) != 0)
                return 0;
            if (((pixel >> 24) & 0xffu) != 0u
                && (pixel & 0x00ffffffu) != 0u)
                return 1;
        }
    }
    return 0;
}

int bounce_visual_assets_verify(const BounceVisualAssets *assets)
{
    BounceSurface *surface;
    BounceRenderer *renderer;
    uint32_t pixel;

    if (assets == NULL
        || assets->atlas == NULL
        || assets->splash == NULL
        || assets->nokiagames == NULL
        || assets->icon == NULL
        || bounce_image_width(assets->atlas) != 48u
        || bounce_image_height(assets->atlas) != 72u
        || bounce_image_width(assets->splash) != 128u
        || bounce_image_height(assets->splash) != 128u
        || bounce_image_width(assets->nokiagames) != 104u
        || bounce_image_height(assets->nokiagames) != 16u
        || bounce_image_width(assets->icon) != 16u
        || bounce_image_height(assets->icon) != 16u)
        return -1;
    if (bounce_image_get_pixel(assets->atlas, 0, 0, &pixel) != 0
        || ((pixel >> 24) & 0xffu) != 0u)
        return -1;

    surface = bounce_surface_create();
    if (surface == NULL)
        return -1;
    renderer = bounce_renderer_create(surface);
    if (renderer == NULL) {
        bounce_renderer_destroy(renderer);
        bounce_surface_destroy(surface);
        return -1;
    }
    if (bounce_surface_fill(surface, UINT32_C(0xff000000)) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_visual_assets_draw_tile(
            renderer,
            assets,
            1u,
            0,
            0
        ) != 0
        || !visual_surface_has_nonblack(surface)
        || bounce_visual_assets_draw_tile(
            renderer,
            assets,
            UINT16_C(0x40),
            16,
            0
        ) != 0
        || bounce_surface_get_pixel(surface, 17, 1, &pixel) != 0
        || pixel != visual_argb(255u, 16u, 96u, 176u)
        || bounce_visual_assets_draw_regular_ball(
            renderer,
            assets,
            32,
            6
        ) != 0
        || bounce_surface_fill(surface, UINT32_C(0xff000000)) != 0
        || bounce_visual_assets_draw_splash(renderer, assets) != 0
        || !visual_surface_has_nonblack(surface)) {
        bounce_renderer_destroy(renderer);
        bounce_surface_destroy(surface);
        return -1;
    }
    bounce_renderer_destroy(renderer);
    bounce_surface_destroy(surface);
    return 0;
}

int bounce_visual_assets_draw_tile(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    uint16_t tile_value,
    int destination_x,
    int destination_y
)
{
    uint32_t base_tile = (uint32_t)tile_value;
    bool water = (base_tile & 0x40u) != 0u;
    uint32_t background;
    int front;
    int back;

    if (renderer == NULL || assets == NULL)
        return -1;
    base_tile &= ~0x40u;
    base_tile &= ~0x80u;
    background = water
        ? visual_argb(255u, 16u, 96u, 176u)
        : visual_argb(255u, 176u, 224u, 240u);
    if (bounce_renderer_fill_rect(
            renderer,
            destination_x,
            destination_y,
            12,
            12,
            background
        ) != 0)
        return -1;

    if (base_tile >= 13u && base_tile <= 28u) {
        int offset = (int)base_tile - 13;
        front = tile_front_sprites[offset];
        back = tile_back_sprites[offset];
        if (visual_draw_sprite(
                renderer,
                assets->sprites[front],
                destination_x,
                destination_y
            ) != 0
            || visual_draw_sprite(
                renderer,
                assets->sprites[back],
                destination_x,
                destination_y
            ) != 0)
            return -1;
        return 0;
    }

    switch (base_tile) {
        case 0:
            return 0;
        case 1:
            return visual_draw_sprite(
                renderer, assets->sprites[0], destination_x, destination_y
            );
        case 2:
            return visual_draw_sprite(
                renderer, assets->sprites[1], destination_x, destination_y
            );
        case 3:
        case 4:
        case 5:
        case 6: {
            /* STEP 14B-07. b.java:418-445 selects one of two sprites per
             * orientation, on the isInWater bit that :866 already decoded.
             *
             * The tile -> sprite correspondence is NOT linear in the tile id,
             * and must not be computed as 2 + (base_tile - 3):
             *
             *   TileIDs  (TileIDs.java:7-10)   UP=3  RIGHT=4  DOWN=5  LEFT=6
             *   SpriteIDs (b.java:45-53)      UP=2  DOWN=3   LEFT=4   RIGHT=5
             *
             * The two enumerations are in different orders, so the previous
             * `2 + (int)base_tile - 3` drew sprite 3 (DOWN) for tile 4
             * (RIGHT), 4 (LEFT) for tile 5 (DOWN) and 5 (RIGHT) for tile 6
             * (LEFT).  Only tile 3 was correct.  The correspondence is
             * therefore tabulated, mirroring the four Java case blocks:
             *
             *   tile 3 THORNS_UP    ->  2 THORNS_UP    /  6 THORNS_WATER_UP
             *   tile 4 THORNS_RIGHT ->  5 THORNS_RIGHT /  9 THORNS_WATER_RIGHT
             *   tile 5 THORNS_DOWN  ->  3 THORNS_DOWN  /  7 THORNS_WATER_DOWN
             *   tile 6 THORNS_LEFT  ->  4 THORNS_LEFT  /  8 THORNS_WATER_LEFT
             *
             * Column 1 is the water variant, which native already builds at
             * :515-522 and previously never read.  Nothing else changes: the
             * background fill above still uses :866, and the collision arm for
             * ids 3-6 in collision_query.c is untouched.
             */
            static const int thorn_sprite[4][2] = {
                { 2, 6 },
                { 5, 9 },
                { 3, 7 },
                { 4, 8 }
            };
            int index = (int)base_tile - 3;

            return visual_draw_sprite(
                renderer,
                assets->sprites[thorn_sprite[index][water ? 1 : 0]],
                destination_x,
                destination_y
            );
        }
        case 7:
            return visual_draw_sprite(
                renderer, assets->sprites[10], destination_x, destination_y
            );
        case 8:
            return visual_draw_sprite(
                renderer, assets->sprites[11], destination_x, destination_y
            );
        case 12:
            return visual_draw_sprite(
                renderer, assets->sprites[12], destination_x, destination_y
            );
        case 29:
            return visual_draw_sprite(
                renderer, assets->sprites[45], destination_x, destination_y
            );
        case 30:
            return visual_draw_sprite(
                renderer,
                assets->sprites[water ? 61 : 57],
                destination_x,
                destination_y
            );
        case 31:
            return visual_draw_sprite(
                renderer,
                assets->sprites[water ? 60 : 56],
                destination_x,
                destination_y
            );
        case 32:
            return visual_draw_sprite(
                renderer,
                assets->sprites[water ? 59 : 55],
                destination_x,
                destination_y
            );
        case 33:
            return visual_draw_sprite(
                renderer,
                assets->sprites[water ? 62 : 58],
                destination_x,
                destination_y
            );
        case 34:
            return visual_draw_sprite(
                renderer, assets->sprites[65], destination_x, destination_y
            );
        case 35:
            return visual_draw_sprite(
                renderer, assets->sprites[64], destination_x, destination_y
            );
        case 36:
            return visual_draw_sprite(
                renderer, assets->sprites[63], destination_x, destination_y
            );
        case 37:
            return visual_draw_sprite(
                renderer, assets->sprites[66], destination_x, destination_y
            );
        case 38:
            return visual_draw_sprite(
                renderer, assets->sprites[53], destination_x, destination_y
            );
        case 39:
        case 40:
        case 41:
        case 42:
            return visual_draw_sprite(
                renderer,
                assets->deflater[(int)base_tile - 39],
                destination_x,
                destination_y
            );
        case 43:
        case 44:
        case 45:
        case 46:
            return visual_draw_sprite(
                renderer,
                assets->pumper[(int)base_tile - 43],
                destination_x,
                destination_y
            );
        case 47:
        case 48:
        case 49:
        case 50:
            return visual_draw_sprite(
                renderer,
                assets->gravity[(int)base_tile - 47],
                destination_x,
                destination_y
            );
        case 51:
        case 52:
        case 53:
        case 54:
            return visual_draw_sprite(
                renderer,
                assets->jump[(int)base_tile - 51],
                destination_x,
                destination_y
            );
        default:
            /* Animated exit/dynamic-thorn state is intentionally not guessed. */
            return 0;
    }
}

int bounce_visual_assets_draw_regular_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return visual_draw_sprite(
        renderer,
        assets->sprites[47],
        screen_x - BOUNCE_BALL_HALF_SIZE,
        screen_y - BOUNCE_BALL_HALF_SIZE
    );
}

int bounce_visual_assets_draw_player_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int ball_size,
    int half_size,
    int death_state,
    int screen_x,
    int screen_y
)
{
    /*
     * f.java:169-171 (UseBigBall) and f.java:210-212 (UseRegularBall) assign
     * spriteCurrentBall in the same statement groups that assign ballSize and
     * p, and f.java is the only writer of spriteCurrentBall. The live sprite is
     * therefore a pure function of ballSize, and this reproduces that mapping
     * rather than storing a second copy of the mode in native state:
     *
     *   ballSize == 16 -> SpriteIDs.BIG_BALL (49), the 16x16 Mirror16x16Tile
     *   ballSize == 12 -> SpriteIDs.BALL    (47), the 12x12 atlas tile
     *
     * e.java:217 draws at (x - p, y - p), so the destination offset is the
     * caller's live p: 8 for the big ball, 6 for the regular one. half_size is
     * taken from the caller rather than from a constant so the source
     * expression is preserved exactly; a regular ball still yields 6, which is
     * what BOUNCE_BALL_HALF_SIZE encodes.
     */
    /*
     * STEP 14B-11, GAP-005. e.java:214-215:
     *
     *   if (this.aq.z == 2)
     *       drawImage(this.aq.spritePoppedBall, i - 6 + paramInt, j - 6, 20);
     *   else
     *       drawImage(this.aq.spriteCurrentBall, i - this.aq.p + paramInt, j - this.aq.p, 20);
     *
     * z == 2 is the death state (f.java:218 sets it in KillBall; :661 clears it
     * to 1 when the q countdown ends). While it holds, the source draws
     * spritePoppedBall, SpriteIDs.POPPED_BALL = 48 (b.java:94), built from
     * atlas cell (0,1) at b.java:753 and attached at b.java:778.
     *
     * The offset is part of the same branch and is NOT the caller's p. The
     * popped arm uses the fixed literal 6, matching the 12x12 popped sprite
     * (b.java:782-787); KillBall writes neither ballSize nor p, so a Big Ball
     * that dies still has p == 8 while the source places the 12x12 popped
     * sprite at -6, not -8. Selecting sprite 48 alone would therefore be
     * geometrically wrong on a big-ball death, which is reachable on the
     * isBigBall levels 003, 005 and 011.
     *
     * The live arm is unchanged: spriteCurrentBall is still a pure function of
     * ballSize, and its offset is still the caller's live p.
     */
    int sprite_index;
    int offset;

    if (renderer == NULL || assets == NULL)
        return -1;
    if (death_state == 2) {
        sprite_index = BOUNCE_VISUAL_SPRITE_POPPED_BALL;
        offset = BOUNCE_BALL_HALF_SIZE;
    } else {
        sprite_index = ball_size == BOUNCE_VISUAL_BIG_BALL_SIZE
            ? BOUNCE_VISUAL_SPRITE_BIG_BALL
            : BOUNCE_VISUAL_SPRITE_BALL;
        offset = half_size;
    }
    return visual_draw_sprite(
        renderer,
        assets->sprites[sprite_index],
        screen_x - offset,
        screen_y - offset
    );
}

int bounce_visual_assets_draw_hud_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int destination_x,
    int destination_y
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return visual_draw_sprite(
        renderer,
        assets->hud_ball,
        destination_x,
        destination_y
    );
}

int bounce_visual_assets_draw_hud_hoop(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int destination_x,
    int destination_y
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return visual_draw_sprite(
        renderer,
        assets->hud_hoop,
        destination_x,
        destination_y
    );
}

/*
 * STEP 12X-HUD-SCORE-IMPL-B -- the original in-game SCORE, e.java:183.
 *
 * Longest possible output is prefix 7 + sign 1 + magnitude 10 + NUL 1 = 19, so 24 is a
 * static ceiling. Nothing here allocates; the formatted text lives in this one frame-local
 * buffer and is consumed before the call returns.
 */
#define VISUAL_HUD_SCORE_BUFFER 24u

/*
 * e.java:436-454, `PadZeroes(int)`, transcribed.
 *
 * It is NOT conventional seven-digit padding and is deliberately not "corrected" into one. It
 * picks a FIXED zero string by magnitude and concatenates the signed decimal, so the recovered
 * widths are 8 characters for 1-digit and 3-to-8-digit values, 9 for 2-digit values, 8 zeros
 * for 0, and prefix + "-N" for negatives. Verified by executing this ladder over the whole
 * int32 domain in STEP 12X-HUD-SCORE-FMT; the two-digit 9-character case (10 -> 000000010)
 * and the 0 -> 00000000 case are the two that a "sensible" rewrite would silently change.
 */
static int visual_hud_pad_zeroes(int32_t number, char *out)
{
    const char *prefix;
    char reversed[11];
    size_t at = 0u;
    size_t digits = 0u;
    uint32_t magnitude;

    if (out == NULL)
        return -1;

    /* e.java:438-451, the seven comparisons verbatim and in source order. */
    if (number < 100) {
        prefix = "0000000";
    } else if (number < 1000) {
        prefix = "00000";
    } else if (number < 10000) {
        prefix = "0000";
    } else if (number < 100000) {
        prefix = "000";
    } else if (number < 1000000) {
        prefix = "00";
    } else if (number < 10000000) {
        prefix = "0";
    } else {
        prefix = "";
    }

    for (; *prefix != '\0'; ++prefix)
        out[at++] = *prefix;

    /*
     * e.java:453 `return str + number;` is String.valueOf(int): a leading '-' followed by the
     * magnitude in plain decimal, with no grouping and no padding of its own. The sign is
     * emitted separately and the magnitude is taken as uint32_t, so INT32_MIN is
     * representable. Negating the int32_t itself is never done, because -INT32_MIN overflows
     * a 32-bit signed type.
     */
    if (number < 0) {
        out[at++] = '-';
        /* Unsigned wraparound, which is well defined for every input including INT32_MIN. */
        magnitude = (uint32_t)0 - (uint32_t)number;
    } else {
        magnitude = (uint32_t)number;
    }

    do {
        reversed[digits++] = (char)('0' + (int)(magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u);

    while (digits > 0u)
        out[at++] = reversed[--digits];

    out[at] = '\0';
    return (int)at;
}

/*
 * 5x7 digit shapes, one bit per pixel, most significant bit leftmost within a 5-bit row.
 * These are the same shapes the UI screens already draw at native/app/ui_shell.c:1759; that
 * copy stays where it is, because it serves the themed menu and form destinations and is
 * file-static there, so the gameplay view cannot reach it. This is the gameplay-side copy
 * required by the visual-asset HUD seam, and it is the same data rather than a new design.
 */
static const uint8_t visual_hud_digit_rows[10][7] = {
    { 0x0eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu },
    { 0x04u, 0x0cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0eu },
    { 0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1fu },
    { 0x1fu, 0x02u, 0x04u, 0x02u, 0x01u, 0x11u, 0x0eu },
    { 0x02u, 0x06u, 0x0au, 0x12u, 0x1fu, 0x02u, 0x02u },
    { 0x1fu, 0x10u, 0x1eu, 0x01u, 0x01u, 0x11u, 0x0eu },
    { 0x06u, 0x08u, 0x10u, 0x1eu, 0x11u, 0x11u, 0x0eu },
    { 0x1fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u },
    { 0x0eu, 0x11u, 0x11u, 0x0eu, 0x11u, 0x11u, 0x0eu },
    { 0x0eu, 0x11u, 0x11u, 0x0fu, 0x01u, 0x02u, 0x0cu }
};

static int visual_hud_digit(
    BounceRenderer *renderer,
    int x,
    int y,
    int digit,
    uint32_t color
)
{
    int row;
    int column;

    if (renderer == NULL || digit < 0 || digit > 9)
        return -1;
    for (row = 0; row < 7; ++row) {
        for (column = 0; column < 5; ++column) {
            if ((visual_hud_digit_rows[digit][row] & (uint8_t)(1u << (4 - column))) != 0u
                && bounce_renderer_fill_rect(renderer, x + column, y + row, 1, 1, color)
                       != 0)
                return -1;
        }
    }
    return 0;
}

/*
 * The '-' that Java's concatenation can place mid-string, so -1 renders as "0000000-1"
 * (e.java:453). No existing native glyph mechanism can draw it: the UI text font is A-Z only
 * (native/app/ui_shell.c:382, placeholder_glyph_rows[26]) and placeholder_text_index returns
 * -1 for anything else. Rather than invent a font, this is the minimum shape that represents
 * it -- the conventional 5x7 hyphen, columns 1..3 of row 3 -- drawn with the same 1x1 fill
 * primitive, so the cell and the 6px pitch are identical to the digits. Unreachable in normal
 * play, because a score stays orders of magnitude below 2^31, but it is drawn rather than
 * silently dropped.
 */
static int visual_hud_minus(
    BounceRenderer *renderer,
    int x,
    int y,
    uint32_t color
)
{
    int column;

    if (renderer == NULL)
        return -1;
    for (column = 1; column <= 3; ++column)
        if (bounce_renderer_fill_rect(renderer, x + column, y + 3, 1, 1, color) != 0)
            return -1;
    return 0;
}

int bounce_visual_assets_draw_hud_score(
    BounceRenderer *renderer,
    int destination_x,
    int destination_y,
    int32_t score,
    uint32_t color
)
{
    char text[VISUAL_HUD_SCORE_BUFFER];
    int length;
    int index;
    int pen;

    if (renderer == NULL)
        return -1;

    length = visual_hud_pad_zeroes(score, text);
    if (length < 0)
        return -1;

    /*
     * e.java:183 is a TOP|LEFT anchor at a fixed origin, so the pen starts there and is never
     * adjusted for the formatted length. The score is left-anchored, not centred, and a longer
     * value simply runs further right.
     */
    pen = destination_x;
    for (index = 0; index < length; ++index) {
        int rc;
        if (text[index] >= '0' && text[index] <= '9') {
            rc = visual_hud_digit(renderer, pen, destination_y, text[index] - '0', color);
        } else if (text[index] == '-') {
            rc = visual_hud_minus(renderer, pen, destination_y, color);
        } else {
            rc = -1;
        }
        if (rc != 0)
            return -1;
        pen += 6;
    }
    return 0;
}

int bounce_visual_assets_draw_splash(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return visual_draw_sprite(renderer, assets->splash, 0, 0);
}

/*
 * STEP 12E — Native Exit Door Rendering.
 *
 * Draws the 24x48 exit door sprite at the specified screen position.
 * Uses the existing transparent blit mechanism and clipping architecture.
 * The sprite is drawn as a single image, not as individual tiles.
 */
int bounce_visual_assets_draw_exit_door(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return visual_draw_sprite(
        renderer,
        assets->sprites[12],
        screen_x,
        screen_y
    );
}

/*
 * STEP 12R — Exact Exit Sprite Window.
 *
 * Draws a 24x24 source region from the 24x48 exit door sprite at the
 * specified screen position. The source Y offset is the door animation
 * offset (door_image_offset), reproducing the Java aa window behavior.
 */
int bounce_visual_assets_draw_exit_door_window(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y,
    int source_y
)
{
    if (renderer == NULL || assets == NULL)
        return -1;
    return bounce_renderer_blit_region_transparent(
        renderer,
        assets->sprites[12],
        0,
        source_y,
        24,
        24,
        screen_x,
        screen_y
    );
}

/*
 * STEP 12V — Dynamic Thorn Composite.
 *
 * Constructs the 24x24 spriteDynThorn from 4 transformed copies of
 * assets->sprites[46] (DYN_THORN_QUARTER), reproducing the Java composite:
 *
 *   Top-left:     Original
 *   Top-right:    FLIP_HORIZONTAL
 *   Bottom-right: ROTATE_180
 *   Bottom-left:  FLIP_VERTICAL
 */
static BounceImage *visual_dyn_thorn_composite(const BounceVisualAssets *assets)
{
    BounceImage *composite;
    BounceImage *quarter;
    BounceImage *flip_h;
    BounceImage *flip_v;
    BounceImage *rot180;
    int x;
    int y;

    if (assets == NULL || assets->sprites[46] == NULL)
        return NULL;

    quarter = assets->sprites[46];
    flip_h = visual_transform_image(quarter, BOUNCE_VISUAL_TRANSFORM_FLIP_HORIZONTAL);
    flip_v = visual_transform_image(quarter, BOUNCE_VISUAL_TRANSFORM_FLIP_VERTICAL);
    rot180 = visual_transform_image(quarter, BOUNCE_VISUAL_TRANSFORM_ROTATE_180);

    if (flip_h == NULL || flip_v == NULL || rot180 == NULL) {
        bounce_image_destroy(flip_h);
        bounce_image_destroy(flip_v);
        bounce_image_destroy(rot180);
        return NULL;
    }

    composite = bounce_image_create(24u, 24u);
    if (composite == NULL) {
        bounce_image_destroy(flip_h);
        bounce_image_destroy(flip_v);
        bounce_image_destroy(rot180);
        return NULL;
    }

    /* Top-left: original */
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(quarter, x, y, &pixel) == 0)
                bounce_image_set_pixel(composite, x, y, pixel);
        }
    }

    /* Top-right: flip horizontal */
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(flip_h, x, y, &pixel) == 0)
                bounce_image_set_pixel(composite, 12 + x, y, pixel);
        }
    }

    /* Bottom-left: flip vertical */
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(flip_v, x, y, &pixel) == 0)
                bounce_image_set_pixel(composite, x, 12 + y, pixel);
        }
    }

    /* Bottom-right: rotate 180 */
    for (y = 0; y < 12; ++y) {
        for (x = 0; x < 12; ++x) {
            uint32_t pixel;
            if (bounce_image_get_pixel(rot180, x, y, &pixel) == 0)
                bounce_image_set_pixel(composite, 12 + x, 12 + y, pixel);
        }
    }

    bounce_image_destroy(flip_h);
    bounce_image_destroy(flip_v);
    bounce_image_destroy(rot180);
    return composite;
}

/*
 * STEP 12V — Dynamic Thorn Rendering.
 *
 * Draws the 24x24 Dynamic Thorn composite at the specified screen position.
 * Uses the existing transparent blit mechanism and clipping architecture.
 */
int bounce_visual_assets_draw_dyn_thorn(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
)
{
    BounceImage *composite;
    int result;

    if (renderer == NULL || assets == NULL)
        return -1;

    composite = visual_dyn_thorn_composite(assets);
    if (composite == NULL)
        return -1;

    result = bounce_renderer_blit_region_transparent(
        renderer,
        composite,
        0,
        0,
        24,
        24,
        screen_x,
        screen_y
    );

    bounce_image_destroy(composite);
    return result;
}
