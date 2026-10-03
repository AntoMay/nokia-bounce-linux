#include "asset.h"

#include <png.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct BounceAsset {
    char *resource_path;
};

static int asset_size_multiply(
    size_t left,
    size_t right,
    size_t *result
)
{
    if (result == NULL || (right != 0u && left > SIZE_MAX / right))
        return -1;
    *result = left * right;
    return 0;
}

BounceAsset *bounce_asset_create(const char *resource_path)
{
    BounceAsset *asset;
    size_t path_length;
    char *path_copy;

    if (resource_path == NULL)
        return NULL;

    path_length = strlen(resource_path);
    if (path_length == 0u || path_length == SIZE_MAX)
        return NULL;

    path_copy = (char *)malloc(path_length + 1u);
    if (path_copy == NULL)
        return NULL;
    memcpy(path_copy, resource_path, path_length + 1u);

    asset = (BounceAsset *)calloc(1, sizeof *asset);
    if (asset == NULL) {
        free(path_copy);
        return NULL;
    }

    asset->resource_path = path_copy;
    return asset;
}

void bounce_asset_destroy(BounceAsset *asset)
{
    if (asset == NULL)
        return;

    free(asset->resource_path);
    free(asset);
}

const char *bounce_asset_resource_path(const BounceAsset *asset)
{
    return asset == NULL ? NULL : asset->resource_path;
}

BounceAssetDecodeStatus bounce_asset_decode(
    const BounceAsset *asset,
    BounceImage **image_out
)
{
    if (image_out != NULL)
        *image_out = NULL;

    if (asset == NULL || image_out == NULL)
        return BOUNCE_ASSET_DECODE_INVALID_ARGUMENT;

    return BOUNCE_ASSET_DECODE_UNSUPPORTED;
}

BounceAssetDecodeStatus bounce_asset_decode_file(
    const char *filesystem_path,
    BounceImage **image_out
)
{
    FILE *file;
    png_structp png_ptr;
    png_infop info_ptr;
    BounceImage *image;
    png_bytep *rows;
    uint8_t *raw_pixels;
    size_t raw_size;
    size_t row_pointers_size;
    png_uint_32 width;
    png_uint_32 height;
    png_size_t row_bytes;
    int color_type;
    int bit_depth;
    int channels;
    uint32_t y;
    BounceAssetDecodeStatus status;

    if (image_out != NULL)
        *image_out = NULL;
    if (filesystem_path == NULL
        || filesystem_path[0] == '\0'
        || image_out == NULL)
        return BOUNCE_ASSET_DECODE_INVALID_ARGUMENT;

    file = fopen(filesystem_path, "rb");
    if (file == NULL)
        return BOUNCE_ASSET_DECODE_IO_ERROR;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        (void)fclose(file);
        return BOUNCE_ASSET_DECODE_INVALID_DATA;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        (void)fclose(file);
        return BOUNCE_ASSET_DECODE_INVALID_DATA;
    }

    image = NULL;
    rows = NULL;
    raw_pixels = NULL;
    status = BOUNCE_ASSET_DECODE_INVALID_DATA;
    if (setjmp(png_jmpbuf(png_ptr)) != 0)
        goto done;

    png_init_io(png_ptr, file);
    png_read_info(png_ptr, info_ptr);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    if (width == 0u
        || height == 0u
        || width > UINT32_MAX
        || height > UINT32_MAX
        || (color_type != PNG_COLOR_TYPE_RGB
            && color_type != PNG_COLOR_TYPE_PALETTE)
        || (color_type == PNG_COLOR_TYPE_RGB && bit_depth != 8)
        || (color_type == PNG_COLOR_TYPE_PALETTE
            && bit_depth != 1
            && bit_depth != 8))
        goto done;

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    else
        png_set_add_alpha(png_ptr, 0xffu, PNG_FILLER_AFTER);
    (void)png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);
    if (bit_depth != 8 || (channels != 3 && channels != 4))
        goto done;
    row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    if (row_bytes == 0
        || asset_size_multiply(
            (size_t)height,
            (size_t)row_bytes,
            &raw_size
        ) != 0
        || asset_size_multiply(
            (size_t)height,
            sizeof *rows,
            &row_pointers_size
        ) != 0)
        goto done;
    raw_pixels = (uint8_t *)malloc(raw_size);
    rows = (png_bytep *)malloc(row_pointers_size);
    image = bounce_image_create((uint32_t)width, (uint32_t)height);
    if (raw_pixels == NULL || rows == NULL || image == NULL)
        goto done;
    for (y = 0u; y < (uint32_t)height; ++y)
        rows[y] = raw_pixels + (size_t)y * (size_t)row_bytes;
    png_read_image(png_ptr, rows);
    png_read_end(png_ptr, NULL);

    for (y = 0u; y < (uint32_t)height; ++y) {
        uint32_t x;
        for (x = 0u; x < (uint32_t)width; ++x) {
            size_t offset = (size_t)y * (size_t)row_bytes
                + (size_t)x * (size_t)channels;
            uint32_t alpha = channels == 4
                ? raw_pixels[offset + 3u]
                : 255u;
            uint32_t pixel = (alpha << 24)
                | ((uint32_t)raw_pixels[offset] << 16)
                | ((uint32_t)raw_pixels[offset + 1u] << 8)
                | (uint32_t)raw_pixels[offset + 2u];
            if (bounce_image_set_pixel(
                    image,
                    (int)x,
                    (int)y,
                    pixel
                ) != 0)
                goto done;
        }
    }
    status = BOUNCE_ASSET_DECODE_OK;

done:
    free(raw_pixels);
    free(rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if (fclose(file) != 0 && status == BOUNCE_ASSET_DECODE_OK)
        status = BOUNCE_ASSET_DECODE_IO_ERROR;
    if (status == BOUNCE_ASSET_DECODE_OK) {
        *image_out = image;
        image = NULL;
    }
    bounce_image_destroy(image);
    return status;
}
