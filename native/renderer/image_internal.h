#ifndef BOUNCE_RENDERER_IMAGE_INTERNAL_H
#define BOUNCE_RENDERER_IMAGE_INTERNAL_H

#include "image.h"

#include <stddef.h>

/* Private size-indexed accessor used by the logical blit implementation. */
int bounce_image_get_pixel_internal(
    const BounceImage *image,
    size_t x,
    size_t y,
    uint32_t *pixel_out
);

#endif /* BOUNCE_RENDERER_IMAGE_INTERNAL_H */
