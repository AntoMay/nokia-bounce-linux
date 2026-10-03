#include "presentation.h"

#include <stdlib.h>

struct BouncePresentation {
    BounceSurface *surface;
    BouncePresentationCallback callback;
    void *user_data;
};

BouncePresentation *bounce_presentation_create(
    BounceSurface *surface,
    BouncePresentationCallback callback,
    void *user_data
)
{
    BouncePresentation *presentation;

    if (surface == NULL
        || callback == NULL
        || bounce_surface_width(surface) != BOUNCE_SURFACE_WIDTH
        || bounce_surface_height(surface) != BOUNCE_SURFACE_HEIGHT)
        return NULL;

    presentation = (BouncePresentation *)calloc(1, sizeof *presentation);
    if (presentation == NULL)
        return NULL;

    presentation->surface = surface;
    presentation->callback = callback;
    presentation->user_data = user_data;
    return presentation;
}

void bounce_presentation_destroy(BouncePresentation *presentation)
{
    free(presentation);
}

uint32_t bounce_presentation_width(const BouncePresentation *presentation)
{
    return presentation == NULL ? 0u : BOUNCE_SURFACE_WIDTH;
}

uint32_t bounce_presentation_height(const BouncePresentation *presentation)
{
    return presentation == NULL ? 0u : BOUNCE_SURFACE_HEIGHT;
}

int bounce_presentation_present(BouncePresentation *presentation)
{
    if (presentation == NULL
        || presentation->surface == NULL
        || presentation->callback == NULL)
        return -1;

    presentation->callback(presentation->surface, presentation->user_data);
    return 0;
}
