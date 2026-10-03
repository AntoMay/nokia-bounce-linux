#ifndef BOUNCE_RENDERER_PIPELINE_H
#define BOUNCE_RENDERER_PIPELINE_H

#include "e_buffer.h"
#include "renderer.h"
#include "surface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-independent logical boundary for the verified q() stage order.
 *
 * The pipeline observes/reports stages; it does not draw pixels, own game
 * state, calculate camera values, load assets, or request platform repaint.
 * Its surface, renderer, and E-buffer references are borrowed.
 */

typedef enum BounceRenderPipelineStage {
    BOUNCE_RENDER_PIPELINE_STAGE_GAMEPLAY_CLIP = 0,
    BOUNCE_RENDER_PIPELINE_STAGE_DIRTY_TILES,
    BOUNCE_RENDER_PIPELINE_STAGE_E_DRAW,
    BOUNCE_RENDER_PIPELINE_STAGE_E_DRAW_SECOND,
    BOUNCE_RENDER_PIPELINE_STAGE_BALL,
    BOUNCE_RENDER_PIPELINE_STAGE_FOREGROUND_HOOP,
    BOUNCE_RENDER_PIPELINE_STAGE_FULL_CANVAS_CLIP,
    BOUNCE_RENDER_PIPELINE_STAGE_HUD,
    BOUNCE_RENDER_PIPELINE_STAGE_HUD_DIRTY_CLEAR,
    BOUNCE_RENDER_PIPELINE_STAGE_PRESENTATION_REQUEST
} BounceRenderPipelineStage;

/*
 * Caller-supplied logical inputs. The pipeline carries these values but does
 * not derive or update camera/gameplay state. A nonzero draw_second_e value
 * is the explicit gate for the optional second E boundary; the foreground
 * and HUD flags remain metadata for later stage implementations.
 */
typedef struct BounceRenderPipelineInput {
    int v;
    int draw_second_e;
    int foreground_hoop_present;
    int hud_dirty;
} BounceRenderPipelineInput;

/*
 * Stage event metadata. Clip values describe the logical clip associated with
 * the stage; the pipeline does not mutate BounceRenderer's clip state.
 */
typedef struct BounceRenderPipelineEvent {
    BounceRenderPipelineStage stage;
    int clip_x;
    int clip_y;
    int clip_width;
    int clip_height;
    BounceRenderPipelineInput input;
} BounceRenderPipelineEvent;

/* Optional stage observer. The event pointer is valid only during the call. */
typedef void (*BounceRenderPipelineStageCallback)(
    const BounceRenderPipelineEvent *event,
    void *user_data
);

typedef struct BounceRenderPipeline BounceRenderPipeline;

/*
 * Create a pipeline borrowing surface, renderer, and E-buffer. All resources
 * are required and must remain alive until bounce_render_pipeline_destroy().
 * The renderer must be the renderer created for surface. A NULL callback is
 * allowed and makes execution a no-op with no observable side effects.
 * Returns NULL for missing/invalid resources or allocation failure.
 */
BounceRenderPipeline *bounce_render_pipeline_create(
    BounceSurface *surface,
    BounceRenderer *renderer,
    BounceEBuffer *e_buffer,
    BounceRenderPipelineStageCallback callback,
    void *user_data
);

/* Safe with NULL; does not destroy borrowed resources. */
void bounce_render_pipeline_destroy(BounceRenderPipeline *pipeline);

/*
 * Execute the established logical sequence. The optional second E stage is
 * emitted only when input->draw_second_e is nonzero. No stage performs actual
 * drawing in this boundary. Returns -1 for NULL pipeline or input.
 */
int bounce_render_pipeline_execute(
    BounceRenderPipeline *pipeline,
    const BounceRenderPipelineInput *input
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_RENDERER_PIPELINE_H */
