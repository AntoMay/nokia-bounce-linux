#include "surface.h"

#include <stdlib.h>

struct BounceSurface {
    uint32_t pixels[BOUNCE_SURFACE_WIDTH * BOUNCE_SURFACE_HEIGHT];
};

static int bounce_surface_valid_coordinate(int x, int y)
{
    return x >= 0
        && y >= 0
        && (unsigned int)x < BOUNCE_SURFACE_WIDTH
        && (unsigned int)y < BOUNCE_SURFACE_HEIGHT;
}

BounceSurface *bounce_surface_create(void)
{
    return (BounceSurface *)calloc(1, sizeof(BounceSurface));
}

void bounce_surface_destroy(BounceSurface *surface)
{
    free(surface);
}

int bounce_surface_fill(BounceSurface *surface, uint32_t pixel)
{
    unsigned int y;

    if (surface == NULL)
        return -1;

    for (y = 0; y < BOUNCE_SURFACE_HEIGHT; ++y) {
        unsigned int x;
        for (x = 0; x < BOUNCE_SURFACE_WIDTH; ++x)
            surface->pixels[y * BOUNCE_SURFACE_WIDTH + x] = pixel;
    }

    return 0;
}

int bounce_surface_set_pixel(BounceSurface *surface, int x, int y, uint32_t pixel)
{
    if (surface == NULL || !bounce_surface_valid_coordinate(x, y))
        return -1;

    surface->pixels[(unsigned int)y * BOUNCE_SURFACE_WIDTH + (unsigned int)x] = pixel;
    return 0;
}

int bounce_surface_get_pixel(const BounceSurface *surface, int x, int y, uint32_t *pixel_out)
{
    if (surface == NULL || pixel_out == NULL || !bounce_surface_valid_coordinate(x, y))
        return -1;

    *pixel_out = surface->pixels[(unsigned int)y * BOUNCE_SURFACE_WIDTH + (unsigned int)x];
    return 0;
}

uint32_t bounce_surface_width(const BounceSurface *surface)
{
    return surface == NULL ? 0u : BOUNCE_SURFACE_WIDTH;
}

uint32_t bounce_surface_height(const BounceSurface *surface)
{
    return surface == NULL ? 0u : BOUNCE_SURFACE_HEIGHT;
}
