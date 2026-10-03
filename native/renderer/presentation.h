#ifndef BOUNCE_RENDERER_PRESENTATION_H
#define BOUNCE_RENDERER_PRESENTATION_H

#include "surface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-neutral presentation boundary for the logical 128x128 surface.
 *
 * The presentation object borrows its BounceSurface. Presenting only invokes
 * the supplied callback with that exact surface pointer; it performs no pixel
 * transformation, copy, scaling, presentation, or platform operation.
 */

typedef struct BouncePresentation BouncePresentation;

/*
 * Callback receiving the borrowed logical surface. The const view prevents
 * the presentation boundary from requiring pixel mutation. The callback must
 * not assume any physical window or backend properties.
 */
typedef void (*BouncePresentationCallback)(
    const BounceSurface *surface,
    void *user_data
);

/*
 * Create a presentation boundary. The surface and callback are required.
 * Returns NULL for a NULL/invalid argument or allocation failure. The caller
 * retains ownership of the surface and must keep it alive until destroy.
 */
BouncePresentation *bounce_presentation_create(
    BounceSurface *surface,
    BouncePresentationCallback callback,
    void *user_data
);

/* Safe with NULL; does not destroy the borrowed surface. */
void bounce_presentation_destroy(BouncePresentation *presentation);

/* Return 0 for NULL, otherwise the fixed logical surface dimensions. */
uint32_t bounce_presentation_width(const BouncePresentation *presentation);
uint32_t bounce_presentation_height(const BouncePresentation *presentation);

/*
 * Cross the logical presentation boundary by invoking callback exactly once
 * with the borrowed surface. Returns -1 for a NULL presentation; otherwise
 * returns 0. No pixels are read, copied, transformed, or modified.
 */
int bounce_presentation_present(BouncePresentation *presentation);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_PRESENTATION_H */
