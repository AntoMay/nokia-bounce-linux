#include "renderer.h"
#include "image_internal.h"

#include <stdint.h>
#include <stdlib.h>

struct BounceRenderer {
    BounceSurface *surface;
    int clip_x;
    int clip_y;
    int clip_width;
    int clip_height;
};

static int64_t max_i64(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

static int64_t min_i64(int64_t a, int64_t b)
{
    return a < b ? a : b;
}

static int bounce_renderer_has_surface(const BounceRenderer *renderer)
{
    return renderer != NULL && renderer->surface != NULL;
}

BounceRenderer *bounce_renderer_create(BounceSurface *surface)
{
    BounceRenderer *renderer;

    if (surface == NULL)
        return NULL;

    renderer = (BounceRenderer *)calloc(1, sizeof(BounceRenderer));
    if (renderer == NULL)
        return NULL;

    renderer->surface = surface;
    renderer->clip_x = 0;
    renderer->clip_y = 0;
    renderer->clip_width = (int)BOUNCE_SURFACE_WIDTH;
    renderer->clip_height = (int)BOUNCE_SURFACE_HEIGHT;
    return renderer;
}

void bounce_renderer_destroy(BounceRenderer *renderer)
{
    free(renderer);
}

int bounce_renderer_set_clip(
    BounceRenderer *renderer,
    int x,
    int y,
    int width,
    int height
)
{
    int64_t right;
    int64_t bottom;

    if (!bounce_renderer_has_surface(renderer)
        || width <= 0
        || height <= 0
        || x < 0
        || y < 0)
        return -1;

    right = (int64_t)x + (int64_t)width;
    bottom = (int64_t)y + (int64_t)height;
    if (right > (int64_t)BOUNCE_SURFACE_WIDTH
        || bottom > (int64_t)BOUNCE_SURFACE_HEIGHT)
        return -1;

    renderer->clip_x = x;
    renderer->clip_y = y;
    renderer->clip_width = width;
    renderer->clip_height = height;
    return 0;
}

int bounce_renderer_reset_clip(BounceRenderer *renderer)
{
    if (!bounce_renderer_has_surface(renderer))
        return -1;

    renderer->clip_x = 0;
    renderer->clip_y = 0;
    renderer->clip_width = (int)BOUNCE_SURFACE_WIDTH;
    renderer->clip_height = (int)BOUNCE_SURFACE_HEIGHT;
    return 0;
}

int bounce_renderer_fill_rect(
    BounceRenderer *renderer,
    int x,
    int y,
    int width,
    int height,
    uint32_t pixel
)
{
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t py;
    int64_t px;

    if (!bounce_renderer_has_surface(renderer))
        return -1;
    if (width <= 0 || height <= 0)
        return 0;

    left = max_i64((int64_t)x, (int64_t)renderer->clip_x);
    top = max_i64((int64_t)y, (int64_t)renderer->clip_y);
    right = min_i64(
        (int64_t)x + (int64_t)width,
        (int64_t)renderer->clip_x + (int64_t)renderer->clip_width
    );
    bottom = min_i64(
        (int64_t)y + (int64_t)height,
        (int64_t)renderer->clip_y + (int64_t)renderer->clip_height
    );

    if (left >= right || top >= bottom)
        return 0;

    for (py = top; py < bottom; ++py) {
        for (px = left; px < right; ++px) {
            (void)bounce_surface_set_pixel(
                renderer->surface,
                (int)px,
                (int)py,
                pixel
            );
        }
    }

    return 0;
}

int bounce_renderer_blit(
    BounceRenderer *renderer,
    const BounceImage *image,
    int dst_x,
    int dst_y
)
{
    int64_t image_width;
    int64_t image_height;
    int64_t image_right;
    int64_t image_bottom;
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t px;
    int64_t py;

    if (!bounce_renderer_has_surface(renderer) || image == NULL)
        return -1;

    image_width = (int64_t)bounce_image_width(image);
    image_height = (int64_t)bounce_image_height(image);
    if (image_width <= 0 || image_height <= 0)
        return -1;

    image_right = (int64_t)dst_x + image_width;
    image_bottom = (int64_t)dst_y + image_height;

    left = max_i64((int64_t)dst_x, 0);
    top = max_i64((int64_t)dst_y, 0);
    left = max_i64(left, (int64_t)renderer->clip_x);
    top = max_i64(top, (int64_t)renderer->clip_y);
    right = min_i64(image_right, (int64_t)BOUNCE_SURFACE_WIDTH);
    bottom = min_i64(image_bottom, (int64_t)BOUNCE_SURFACE_HEIGHT);
    right = min_i64(
        right,
        (int64_t)renderer->clip_x + (int64_t)renderer->clip_width
    );
    bottom = min_i64(
        bottom,
        (int64_t)renderer->clip_y + (int64_t)renderer->clip_height
    );

    if (left >= right || top >= bottom)
        return 0;

    for (py = top; py < bottom; ++py) {
        for (px = left; px < right; ++px) {
            int64_t source_x = px - (int64_t)dst_x;
            int64_t source_y = py - (int64_t)dst_y;
            uint32_t pixel;

            if (source_x < 0
                || source_y < 0
                || source_x >= image_width
                || source_y >= image_height)
                return -1;

            if (bounce_image_get_pixel_internal(
                    image,
                    (size_t)source_x,
                    (size_t)source_y,
                    &pixel
                ) != 0)
                return -1;

            if (bounce_surface_set_pixel(
                    renderer->surface,
                    (int)px,
                    (int)py,
                    pixel
                ) != 0)
                return -1;
        }
    }

    return 0;
}

static uint32_t bounce_renderer_alpha_over_opaque(
    uint32_t source,
    uint32_t destination
)
{
    uint32_t source_alpha = (source >> 24) & 0xffu;
    uint32_t inverse_alpha = 255u - source_alpha;
    uint32_t source_red = (source >> 16) & 0xffu;
    uint32_t source_green = (source >> 8) & 0xffu;
    uint32_t source_blue = source & 0xffu;
    uint32_t destination_red = (destination >> 16) & 0xffu;
    uint32_t destination_green = (destination >> 8) & 0xffu;
    uint32_t destination_blue = destination & 0xffu;
    uint64_t red;
    uint64_t green;
    uint64_t blue;

    if (source_alpha == 0u)
        return destination;
    if (source_alpha == 255u)
        return source;

    red = (uint64_t)source_red * source_alpha
        + (uint64_t)destination_red * inverse_alpha + 127u;
    green = (uint64_t)source_green * source_alpha
        + (uint64_t)destination_green * inverse_alpha + 127u;
    blue = (uint64_t)source_blue * source_alpha
        + (uint64_t)destination_blue * inverse_alpha + 127u;
    return UINT32_C(0xff000000)
        | ((uint32_t)(red / 255u) << 16)
        | ((uint32_t)(green / 255u) << 8)
        | (uint32_t)(blue / 255u);
}

int bounce_renderer_blit_region_transparent(
    BounceRenderer *renderer,
    const BounceImage *image,
    int src_x,
    int src_y,
    int src_width,
    int src_height,
    int dst_x,
    int dst_y
)
{
    int64_t image_width;
    int64_t image_height;
    int64_t source_right;
    int64_t source_bottom;
    int64_t image_right;
    int64_t image_bottom;
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t py;
    int64_t px;

    if (!bounce_renderer_has_surface(renderer)
        || image == NULL
        || src_width <= 0
        || src_height <= 0
        || src_x < 0
        || src_y < 0)
        return -1;
    image_width = (int64_t)bounce_image_width(image);
    image_height = (int64_t)bounce_image_height(image);
    source_right = (int64_t)src_x + (int64_t)src_width;
    source_bottom = (int64_t)src_y + (int64_t)src_height;
    if (source_right > image_width || source_bottom > image_height)
        return -1;

    image_right = (int64_t)dst_x + (int64_t)src_width;
    image_bottom = (int64_t)dst_y + (int64_t)src_height;
    left = max_i64((int64_t)dst_x, 0);
    top = max_i64((int64_t)dst_y, 0);
    left = max_i64(left, (int64_t)renderer->clip_x);
    top = max_i64(top, (int64_t)renderer->clip_y);
    right = min_i64(image_right, (int64_t)BOUNCE_SURFACE_WIDTH);
    bottom = min_i64(image_bottom, (int64_t)BOUNCE_SURFACE_HEIGHT);
    right = min_i64(
        right,
        (int64_t)renderer->clip_x + (int64_t)renderer->clip_width
    );
    bottom = min_i64(
        bottom,
        (int64_t)renderer->clip_y + (int64_t)renderer->clip_height
    );
    if (left >= right || top >= bottom)
        return 0;

    for (py = top; py < bottom; ++py) {
        for (px = left; px < right; ++px) {
            int64_t source_x = (int64_t)src_x + px - (int64_t)dst_x;
            int64_t source_y = (int64_t)src_y + py - (int64_t)dst_y;
            uint32_t source_pixel;
            uint32_t destination_pixel;

            if (source_x < 0
                || source_y < 0
                || source_x >= image_width
                || source_y >= image_height
                || bounce_image_get_pixel_internal(
                    image,
                    (size_t)source_x,
                    (size_t)source_y,
                    &source_pixel
                ) != 0
                || bounce_surface_get_pixel(
                    renderer->surface,
                    (int)px,
                    (int)py,
                    &destination_pixel
                ) != 0)
                return -1;
            if (((source_pixel >> 24) & 0xffu) == 0u)
                continue;
            if (bounce_surface_set_pixel(
                    renderer->surface,
                    (int)px,
                    (int)py,
                    bounce_renderer_alpha_over_opaque(
                        source_pixel,
                        destination_pixel
                    )
                ) != 0)
                return -1;
        }
    }
    return 0;
}
