#ifndef BOUNCE_RENDERER_IMAGE_H
#define BOUNCE_RENDERER_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opaque, platform-independent logical image.
 *
 * Pixels are uint32_t tokens initialized to zero. Procedural image tokens use
 * the low 24 bits as RGB; decoded PNG images use the native ARGB convention
 * 0xAARRGGBB so palette/tRNS alpha remains available to an alpha-aware blit.
 * The image owns its contiguous row-major storage;
 * the caller owns the image and must keep it alive while a renderer blits it.
 */

typedef struct BounceImage BounceImage;

/* Zero dimensions, dimension/allocation overflow, and allocation failure return NULL. */
BounceImage *bounce_image_create(uint32_t width, uint32_t height);

/* Safe with NULL. */
void bounce_image_destroy(BounceImage *image);

/* Returns 0 on success and -1 for NULL/invalid coordinates. */
int bounce_image_set_pixel(
    BounceImage *image,
    int x,
    int y,
    uint32_t pixel
);

/* Returns 0 on success and -1 for NULL arguments/invalid coordinates. */
int bounce_image_get_pixel(
    const BounceImage *image,
    int x,
    int y,
    uint32_t *pixel_out
);

/* Return 0 for NULL, otherwise the requested dimensions. */
uint32_t bounce_image_width(const BounceImage *image);
uint32_t bounce_image_height(const BounceImage *image);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_IMAGE_H */
