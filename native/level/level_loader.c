#include "level_loader.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static BounceLevelLoadStatus bounce_level_load_close(
    FILE *file,
    BounceLevelLoadStatus prior_status
)
{
    if (fclose(file) != 0 && prior_status == BOUNCE_LEVEL_LOAD_OK)
        return BOUNCE_LEVEL_LOAD_READ_FAILED;
    return prior_status;
}

BounceLevelLoadStatus bounce_level_load_file(
    const char *path,
    BounceLevel **level_out
)
{
    FILE *file;
    long file_length;
    size_t file_size;
    uint8_t *raw_data = NULL;
    size_t bytes_read;
    BounceLevel *level;
    BounceLevelLoadStatus status;

    if (level_out == NULL)
        return BOUNCE_LEVEL_LOAD_INVALID_OUTPUT;
    *level_out = NULL;

    if (path == NULL || path[0] == '\0')
        return BOUNCE_LEVEL_LOAD_INVALID_PATH;

    file = fopen(path, "rb");
    if (file == NULL)
        return BOUNCE_LEVEL_LOAD_OPEN_FAILED;

    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return BOUNCE_LEVEL_LOAD_SIZE_FAILED;
    }

    file_length = ftell(file);
    if (file_length < 0L
        || (uintmax_t)file_length > (uintmax_t)SIZE_MAX) {
        (void)fclose(file);
        return BOUNCE_LEVEL_LOAD_SIZE_FAILED;
    }
    file_size = (size_t)file_length;

    if (fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return BOUNCE_LEVEL_LOAD_SIZE_FAILED;
    }

    if (file_size != 0u) {
        raw_data = (uint8_t *)malloc(file_size);
        if (raw_data == NULL) {
            (void)fclose(file);
            return BOUNCE_LEVEL_LOAD_ALLOCATION_FAILED;
        }

        bytes_read = fread(raw_data, 1u, file_size, file);
        if (bytes_read != file_size || ferror(file) != 0) {
            status = ferror(file) != 0
                ? BOUNCE_LEVEL_LOAD_READ_FAILED
                : BOUNCE_LEVEL_LOAD_SHORT_READ;
            free(raw_data);
            (void)fclose(file);
            return status;
        }
    }

    status = bounce_level_load_close(file, BOUNCE_LEVEL_LOAD_OK);
    if (status != BOUNCE_LEVEL_LOAD_OK) {
        free(raw_data);
        return status;
    }

    level = bounce_level_parse(raw_data, file_size);
    free(raw_data);
    if (level == NULL)
        return BOUNCE_LEVEL_LOAD_PARSE_FAILED;

    *level_out = level;
    return BOUNCE_LEVEL_LOAD_OK;
}
