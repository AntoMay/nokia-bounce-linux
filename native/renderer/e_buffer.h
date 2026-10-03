#ifndef BOUNCE_RENDERER_E_BUFFER_H
#define BOUNCE_RENDERER_E_BUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fixed logical gameplay offscreen buffer: 156x96 pixels.
 *
 * This is distinct from BounceSurface, which remains the 128x128 logical
 * presentation canvas. Pixels are opaque uint32_t logical tokens initialized
 * to zero. The caller owns this buffer and destroys it when finished.
 *
 * Coordinates are zero-based, top-left logical pixels. Invalid coordinates
 * fail explicitly; this API defines no Java ME or platform pixel semantics.
 */

#define BOUNCE_E_BUFFER_WIDTH 156u
#define BOUNCE_E_BUFFER_HEIGHT 96u

typedef struct BounceEBuffer BounceEBuffer;

/* Returns NULL when storage allocation fails. */
BounceEBuffer *bounce_e_buffer_create(void);

/* Safe with NULL. */
void bounce_e_buffer_destroy(BounceEBuffer *buffer);

/* Return 0 for NULL, otherwise the fixed E-buffer dimensions. */
uint32_t bounce_e_buffer_width(const BounceEBuffer *buffer);
uint32_t bounce_e_buffer_height(const BounceEBuffer *buffer);

/* Fills every E-buffer pixel; also serves as the clear operation. */
int bounce_e_buffer_fill(BounceEBuffer *buffer, uint32_t pixel);

/* Returns 0 on success and -1 for NULL buffer or invalid coordinates. */
int bounce_e_buffer_set_pixel(
    BounceEBuffer *buffer,
    int x,
    int y,
    uint32_t pixel
);

/* Returns 0 on success and -1 for NULL arguments or invalid coordinates. */
int bounce_e_buffer_get_pixel(
    const BounceEBuffer *buffer,
    int x,
    int y,
    uint32_t *pixel_out
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_E_BUFFER_H */
