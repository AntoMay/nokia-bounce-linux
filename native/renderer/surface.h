#ifndef BOUNCE_RENDERER_SURFACE_H
#define BOUNCE_RENDERER_SURFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-independent logical render surface.
 *
 * The surface is intentionally 128x128, matching the verified Java logical
 * canvas.  A pixel is an opaque uint32_t token; this API does not establish
 * Java ME ARGB byte order, alpha semantics, or platform color parity.
 *
 * Coordinates are zero-based. Out-of-range coordinates fail explicitly and
 * are never silently clamped. This primitive does not implement scaling,
 * presentation, clipping, or platform display integration.
 */

#define BOUNCE_SURFACE_WIDTH 128u
#define BOUNCE_SURFACE_HEIGHT 128u

typedef struct BounceSurface BounceSurface;

/* Returns NULL when allocation fails. */
BounceSurface *bounce_surface_create(void);

/* Safe with NULL. */
void bounce_surface_destroy(BounceSurface *surface);

/* Fills every logical pixel; also serves as the clear operation. */
int bounce_surface_fill(BounceSurface *surface, uint32_t pixel);

/* Returns 0 on success and -1 for NULL surface or invalid coordinates. */
int bounce_surface_set_pixel(BounceSurface *surface, int x, int y, uint32_t pixel);

/* Returns 0 on success and -1 for NULL arguments or invalid coordinates. */
int bounce_surface_get_pixel(const BounceSurface *surface, int x, int y, uint32_t *pixel_out);

/* Return 0 for a NULL surface, otherwise the fixed logical dimensions. */
uint32_t bounce_surface_width(const BounceSurface *surface);
uint32_t bounce_surface_height(const BounceSurface *surface);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_SURFACE_H */
