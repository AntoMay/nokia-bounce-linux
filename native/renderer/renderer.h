#ifndef BOUNCE_RENDERER_RENDERER_H
#define BOUNCE_RENDERER_RENDERER_H

#include "image.h"
#include "surface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal logical drawing state for a BounceSurface.
 *
 * The renderer borrows the surface; the caller owns the surface and must keep
 * it alive until bounce_renderer_destroy() returns. Coordinates are top-left
 * logical pixels in the 128x128 surface. This layer has no scaling, platform
 * presentation, asset loading, or game-state ownership.
 */

typedef struct BounceRenderer BounceRenderer;

/* Returns NULL for a NULL surface or allocation failure. */
BounceRenderer *bounce_renderer_create(BounceSurface *surface);

/* Safe with NULL; does not destroy the borrowed surface. */
void bounce_renderer_destroy(BounceRenderer *renderer);

/*
 * Install a clip rectangle. The rectangle must be non-empty and wholly within
 * the 128x128 surface. Invalid rectangles return -1 and leave the current
 * clip unchanged; no implicit clipping of public state is performed.
 */
int bounce_renderer_set_clip(
    BounceRenderer *renderer,
    int x,
    int y,
    int width,
    int height
);

/* Restore the full 128x128 surface clip. */
int bounce_renderer_reset_clip(BounceRenderer *renderer);

/*
 * Fill a logical rectangle, writing only the intersection with the active
 * clip and surface. Non-positive width or height is a deterministic no-op.
 * A rectangle wholly outside the clip is also a no-op.
 */
int bounce_renderer_fill_rect(
    BounceRenderer *renderer,
    int x,
    int y,
    int width,
    int height,
    uint32_t pixel
);

/*
 * Blit an opaque logical image at the requested top-left destination.
 * The image is borrowed and must remain alive for this call. Only the
 * intersection with the active clip and 128x128 surface is copied; no
 * transparency or alpha blending is performed.
 */
int bounce_renderer_blit(
    BounceRenderer *renderer,
    const BounceImage *image,
    int dst_x,
    int dst_y
);

/*
 * Blit a source rectangle using the decoded 0xAARRGGBB pixel convention.
 * The transparent form skips alpha-zero pixels and alpha-composites partial
 * alpha over the existing surface. It is kept separate from the historical
 * opaque blit so existing procedural-token behavior is unchanged.
 */
int bounce_renderer_blit_region_transparent(
    BounceRenderer *renderer,
    const BounceImage *image,
    int src_x,
    int src_y,
    int src_width,
    int src_height,
    int dst_x,
    int dst_y
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_RENDERER_H */
