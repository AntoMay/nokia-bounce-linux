#ifndef BOUNCE_NATIVE_ASSET_H
#define BOUNCE_NATIVE_ASSET_H

#include "../renderer/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-independent identity and decode boundary for a native asset.
 *
 * A BounceAsset owns only a copied, non-empty resource path. It does not open
 * files, inspect PNG data, maintain global state, or fabricate pixels. The
 * resource path is an opaque relative identity expected from the caller; it
 * is not normalized or interpreted as a filesystem path.
 */

typedef struct BounceAsset BounceAsset;

typedef enum BounceAssetDecodeStatus {
    BOUNCE_ASSET_DECODE_INVALID_ARGUMENT = 0,
    BOUNCE_ASSET_DECODE_UNSUPPORTED = 1,
    BOUNCE_ASSET_DECODE_IO_ERROR = 2,
    BOUNCE_ASSET_DECODE_INVALID_DATA = 3,
    BOUNCE_ASSET_DECODE_OK = 4
} BounceAssetDecodeStatus;

/* Returns NULL for a NULL/empty path, allocation failure, or copy overflow. */
BounceAsset *bounce_asset_create(const char *resource_path);

/* Safe with NULL. */
void bounce_asset_destroy(BounceAsset *asset);

/* Returns NULL for NULL; otherwise the path owned by the asset. */
const char *bounce_asset_resource_path(const BounceAsset *asset);

/*
 * Attempt decoding through the identity-only boundary. A valid identity
 * without an explicit filesystem path remains UNSUPPORTED and sets
 * *image_out to NULL. NULL arguments return INVALID_ARGUMENT when an output
 * pointer is available.
 */
BounceAssetDecodeStatus bounce_asset_decode(
    const BounceAsset *asset,
    BounceImage **image_out
);

/*
 * Decode one explicit filesystem PNG path. The supported source formats are
 * the verified original 8-bit RGB and indexed 1/8-bit PNGs; palette/tRNS
 * alpha is retained in the returned 0xAARRGGBB BounceImage. The caller owns
 * the returned image and must destroy it with bounce_image_destroy().
 */
BounceAssetDecodeStatus bounce_asset_decode_file(
    const char *filesystem_path,
    BounceImage **image_out
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_ASSET_H */
