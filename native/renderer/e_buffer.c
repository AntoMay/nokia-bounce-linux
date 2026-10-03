#include "e_buffer.h"
#include "image.h"

#include <stdlib.h>

struct BounceEBuffer {
    BounceImage *storage;
};

BounceEBuffer *bounce_e_buffer_create(void)
{
    BounceEBuffer *buffer;
    BounceImage *storage;

    storage = bounce_image_create(
        BOUNCE_E_BUFFER_WIDTH,
        BOUNCE_E_BUFFER_HEIGHT
    );
    if (storage == NULL)
        return NULL;

    buffer = (BounceEBuffer *)calloc(1, sizeof *buffer);
    if (buffer == NULL) {
        bounce_image_destroy(storage);
        return NULL;
    }

    buffer->storage = storage;
    return buffer;
}

void bounce_e_buffer_destroy(BounceEBuffer *buffer)
{
    if (buffer == NULL)
        return;

    bounce_image_destroy(buffer->storage);
    free(buffer);
}

uint32_t bounce_e_buffer_width(const BounceEBuffer *buffer)
{
    return buffer == NULL || buffer->storage == NULL
        ? 0u
        : BOUNCE_E_BUFFER_WIDTH;
}

uint32_t bounce_e_buffer_height(const BounceEBuffer *buffer)
{
    return buffer == NULL || buffer->storage == NULL
        ? 0u
        : BOUNCE_E_BUFFER_HEIGHT;
}

int bounce_e_buffer_fill(BounceEBuffer *buffer, uint32_t pixel)
{
    unsigned int y;

    if (buffer == NULL || buffer->storage == NULL)
        return -1;

    for (y = 0; y < BOUNCE_E_BUFFER_HEIGHT; ++y) {
        unsigned int x;
        for (x = 0; x < BOUNCE_E_BUFFER_WIDTH; ++x) {
            if (bounce_image_set_pixel(
                    buffer->storage,
                    (int)x,
                    (int)y,
                    pixel
                ) != 0)
                return -1;
        }
    }

    return 0;
}

int bounce_e_buffer_set_pixel(
    BounceEBuffer *buffer,
    int x,
    int y,
    uint32_t pixel
)
{
    if (buffer == NULL || buffer->storage == NULL)
        return -1;

    return bounce_image_set_pixel(buffer->storage, x, y, pixel);
}

int bounce_e_buffer_get_pixel(
    const BounceEBuffer *buffer,
    int x,
    int y,
    uint32_t *pixel_out
)
{
    if (buffer == NULL || buffer->storage == NULL)
        return -1;

    return bounce_image_get_pixel(buffer->storage, x, y, pixel_out);
}
