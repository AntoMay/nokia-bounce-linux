#include "image.h"
#include "image_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct BounceImage {
    size_t width;
    size_t height;
    uint32_t *pixels;
};

BounceImage *bounce_image_create(uint32_t width, uint32_t height)
{
    size_t width_s;
    size_t height_s;
    size_t pixel_count;
    BounceImage *image;

    if (width == 0u || height == 0u)
        return NULL;

    width_s = (size_t)width;
    height_s = (size_t)height;
    if ((uint32_t)width_s != width || (uint32_t)height_s != height)
        return NULL;

    if (height_s > SIZE_MAX / width_s)
        return NULL;

    pixel_count = width_s * height_s;
    if (pixel_count > SIZE_MAX / sizeof(uint32_t))
        return NULL;

    image = (BounceImage *)calloc(1, sizeof *image);
    if (image == NULL)
        return NULL;

    image->pixels = (uint32_t *)calloc(pixel_count, sizeof *image->pixels);
    if (image->pixels == NULL) {
        free(image);
        return NULL;
    }

    image->width = width_s;
    image->height = height_s;
    return image;
}

void bounce_image_destroy(BounceImage *image)
{
    if (image == NULL)
        return;

    free(image->pixels);
    free(image);
}

static int bounce_image_valid_coordinate(
    const BounceImage *image,
    int x,
    int y
)
{
    return image != NULL
        && image->pixels != NULL
        && x >= 0
        && y >= 0
        && (uintmax_t)x < (uintmax_t)image->width
        && (uintmax_t)y < (uintmax_t)image->height;
}

int bounce_image_get_pixel_internal(
    const BounceImage *image,
    size_t x,
    size_t y,
    uint32_t *pixel_out
)
{
    if (image == NULL
        || image->pixels == NULL
        || pixel_out == NULL
        || x >= image->width
        || y >= image->height)
        return -1;

    *pixel_out = image->pixels[y * image->width + x];
    return 0;
}

int bounce_image_set_pixel(
    BounceImage *image,
    int x,
    int y,
    uint32_t pixel
)
{
    if (!bounce_image_valid_coordinate(image, x, y))
        return -1;

    image->pixels[(size_t)y * image->width + (size_t)x] = pixel;
    return 0;
}

int bounce_image_get_pixel(
    const BounceImage *image,
    int x,
    int y,
    uint32_t *pixel_out
)
{
    if (!bounce_image_valid_coordinate(image, x, y) || pixel_out == NULL)
        return -1;

    return bounce_image_get_pixel_internal(
        image,
        (size_t)x,
        (size_t)y,
        pixel_out
    );
}

uint32_t bounce_image_width(const BounceImage *image)
{
    return image == NULL ? 0u : (uint32_t)image->width;
}

uint32_t bounce_image_height(const BounceImage *image)
{
    return image == NULL ? 0u : (uint32_t)image->height;
}
