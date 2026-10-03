#include "pipeline.h"

#include <stdlib.h>

struct BounceRenderPipeline {
    BounceSurface *surface;
    BounceRenderer *renderer;
    BounceEBuffer *e_buffer;
    BounceRenderPipelineStageCallback callback;
    void *user_data;
};

static void bounce_render_pipeline_emit(
    BounceRenderPipeline *pipeline,
    BounceRenderPipelineStage stage,
    int clip_x,
    int clip_y,
    int clip_width,
    int clip_height,
    const BounceRenderPipelineInput *input
)
{
    BounceRenderPipelineEvent event;

    if (pipeline->callback == NULL)
        return;

    event.stage = stage;
    event.clip_x = clip_x;
    event.clip_y = clip_y;
    event.clip_width = clip_width;
    event.clip_height = clip_height;
    event.input = *input;
    pipeline->callback(&event, pipeline->user_data);
}

BounceRenderPipeline *bounce_render_pipeline_create(
    BounceSurface *surface,
    BounceRenderer *renderer,
    BounceEBuffer *e_buffer,
    BounceRenderPipelineStageCallback callback,
    void *user_data
)
{
    BounceRenderPipeline *pipeline;

    if (surface == NULL
        || renderer == NULL
        || e_buffer == NULL
        || bounce_surface_width(surface) != BOUNCE_SURFACE_WIDTH
        || bounce_surface_height(surface) != BOUNCE_SURFACE_HEIGHT
        || bounce_e_buffer_width(e_buffer) != BOUNCE_E_BUFFER_WIDTH
        || bounce_e_buffer_height(e_buffer) != BOUNCE_E_BUFFER_HEIGHT)
        return NULL;

    pipeline = (BounceRenderPipeline *)calloc(1, sizeof *pipeline);
    if (pipeline == NULL)
        return NULL;

    pipeline->surface = surface;
    pipeline->renderer = renderer;
    pipeline->e_buffer = e_buffer;
    pipeline->callback = callback;
    pipeline->user_data = user_data;
    return pipeline;
}

void bounce_render_pipeline_destroy(BounceRenderPipeline *pipeline)
{
    free(pipeline);
}

int bounce_render_pipeline_execute(
    BounceRenderPipeline *pipeline,
    const BounceRenderPipelineInput *input
)
{
    if (pipeline == NULL || input == NULL)
        return -1;

    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_GAMEPLAY_CLIP,
        0,
        0,
        128,
        96,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_DIRTY_TILES,
        0,
        0,
        128,
        96,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_E_DRAW,
        0,
        0,
        128,
        96,
        input
    );
    if (input->draw_second_e != 0) {
        bounce_render_pipeline_emit(
            pipeline,
            BOUNCE_RENDER_PIPELINE_STAGE_E_DRAW_SECOND,
            0,
            0,
            128,
            96,
            input
        );
    }
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_BALL,
        0,
        0,
        128,
        96,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_FOREGROUND_HOOP,
        0,
        0,
        128,
        96,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_FULL_CANVAS_CLIP,
        0,
        0,
        128,
        128,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_HUD,
        0,
        0,
        128,
        128,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_HUD_DIRTY_CLEAR,
        0,
        0,
        128,
        128,
        input
    );
    bounce_render_pipeline_emit(
        pipeline,
        BOUNCE_RENDER_PIPELINE_STAGE_PRESENTATION_REQUEST,
        0,
        0,
        128,
        128,
        input
    );

    return 0;
}
