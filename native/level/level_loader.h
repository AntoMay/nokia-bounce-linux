#ifndef BOUNCE_NATIVE_LEVEL_LOADER_H
#define BOUNCE_NATIVE_LEVEL_LOADER_H

#include "level.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal explicit-path file loader for the structural level parser.
 *
 * The loader reads the complete file as binary bytes, passes those bytes to
 * bounce_level_parse(), releases the temporary buffer, and returns the
 * parser-owned BounceLevel. It performs no discovery, caching, or resource
 * search and does not retain the path or FILE pointer.
 */

typedef enum BounceLevelLoadStatus {
    BOUNCE_LEVEL_LOAD_OK = 0,
    BOUNCE_LEVEL_LOAD_INVALID_PATH,
    BOUNCE_LEVEL_LOAD_INVALID_OUTPUT,
    BOUNCE_LEVEL_LOAD_OPEN_FAILED,
    BOUNCE_LEVEL_LOAD_SIZE_FAILED,
    BOUNCE_LEVEL_LOAD_ALLOCATION_FAILED,
    BOUNCE_LEVEL_LOAD_READ_FAILED,
    BOUNCE_LEVEL_LOAD_SHORT_READ,
    BOUNCE_LEVEL_LOAD_PARSE_FAILED
} BounceLevelLoadStatus;

/*
 * Load exactly the file named by path. level_out must be non-NULL and is set
 * to NULL before work begins. On success, the caller owns the returned level
 * and must destroy it with bounce_level_destroy(). No error text is printed.
 */
BounceLevelLoadStatus bounce_level_load_file(
    const char *path,
    BounceLevel **level_out
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_LEVEL_LOADER_H */
