#ifndef BOUNCE_NATIVE_APP_ASSET_INVENTORY_H
#define BOUNCE_NATIVE_APP_ASSET_INVENTORY_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BounceAssetCoverage {
    BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME = 0,
    BOUNCE_ASSET_COVERAGE_VERIFIED_AVAILABLE_USAGE_UNKNOWN = 1,
    BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT = 2,
    BOUNCE_ASSET_COVERAGE_UNKNOWN_REQUIRES_INVESTIGATION = 3
} BounceAssetCoverage;

typedef enum BounceAssetKind {
    BOUNCE_ASSET_KIND_LEVEL = 0,
    BOUNCE_ASSET_KIND_IMAGE = 1,
    BOUNCE_ASSET_KIND_SOUND = 2,
    BOUNCE_ASSET_KIND_LANGUAGE = 3,
    BOUNCE_ASSET_KIND_MANIFEST = 4
} BounceAssetKind;

typedef struct BounceAssetInventoryEntry {
    const char *resource_path;
    const char *relative_filesystem_path;
    BounceAssetKind kind;
    BounceAssetCoverage coverage;
    const char *format;
    unsigned int width;
    unsigned int height;
    const char *source_usage;
    const char *native_status;
    int rendered;
} BounceAssetInventoryEntry;

/* Returns the immutable inventory table; the caller does not own it. */
const BounceAssetInventoryEntry *bounce_asset_inventory_entries(
    size_t *count_out
);

/*
 * Verifies every inventoried resource exists and owns a BounceAsset identity.
 * PNG entries are decoded through the explicit filesystem decoder and must
 * match their verified dimensions. Level entries are additionally loaded
 * through the verified level parser and runtime-tiles factory. If output is
 * non-NULL, one inventory line is printed per entry.
 */
int bounce_asset_inventory_check(
    const char *resource_root,
    FILE *output_file
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_ASSET_INVENTORY_H */
