#include "asset_inventory.h"

#include "../assets/asset.h"
#include "../level/level_loader.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const BounceAssetInventoryEntry inventory[] = {
    {
        "/levels/J2MElvl.001",
        "levels/J2MElvl.001",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        112u,
        8u,
        "b.LoadLevelId(/levels/J2MElvl.001)",
        "parsed and runtime LevelTiles verified; rendered by current smoke slice",
        1
    },
    {
        "/levels/J2MElvl.002",
        "levels/J2MElvl.002",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        134u,
        22u,
        "b.LoadLevelId(/levels/J2MElvl.002)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.003",
        "levels/J2MElvl.003",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        134u,
        36u,
        "b.LoadLevelId(/levels/J2MElvl.003)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.004",
        "levels/J2MElvl.004",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        134u,
        29u,
        "b.LoadLevelId(/levels/J2MElvl.004)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.005",
        "levels/J2MElvl.005",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        90u,
        43u,
        "b.LoadLevelId(/levels/J2MElvl.005)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.006",
        "levels/J2MElvl.006",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        112u,
        36u,
        "b.LoadLevelId(/levels/J2MElvl.006)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.007",
        "levels/J2MElvl.007",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        134u,
        36u,
        "b.LoadLevelId(/levels/J2MElvl.007)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.008",
        "levels/J2MElvl.008",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        156u,
        36u,
        "b.LoadLevelId(/levels/J2MElvl.008)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.009",
        "levels/J2MElvl.009",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        222u,
        36u,
        "b.LoadLevelId(/levels/J2MElvl.009)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.010",
        "levels/J2MElvl.010",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        112u,
        35u,
        "b.LoadLevelId(/levels/J2MElvl.010)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/levels/J2MElvl.011",
        "levels/J2MElvl.011",
        BOUNCE_ASSET_KIND_LEVEL,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Bounce custom binary",
        178u,
        57u,
        "b.LoadLevelId(/levels/J2MElvl.011)",
        "parsed and runtime LevelTiles verified; not rendered",
        0
    },
    {
        "/icons/objects_nm.png",
        "icons/objects_nm.png",
        BOUNCE_ASSET_KIND_IMAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "PNG 8-bit colormap 48x72",
        48u,
        72u,
        "b.LoadSpriteSheet(/icons/objects_nm.png)",
        "PNG decoded with palette/tRNS; objects atlas used by native tile/ball rendering",
        1
    },
    {
        "/icons/bouncesplash.png",
        "icons/bouncesplash.png",
        BOUNCE_ASSET_KIND_IMAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "PNG 8-bit RGB 128x128",
        128u,
        128u,
        "e.SPLASHES and Image.createImage",
        "PNG decoded; bouncesplash used for native startup/menu artwork",
        1
    },
    {
        "/icons/nokiagames.png",
        "icons/nokiagames.png",
        BOUNCE_ASSET_KIND_IMAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "PNG 1-bit colormap 104x16",
        104u,
        16u,
        "e.SPLASHES and Image.createImage",
        "PNG decoded; source splash asset identity verified, activation path not yet used",
        0
    },
    {
        "/icons/icon.png",
        "icons/icon.png",
        BOUNCE_ASSET_KIND_IMAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_AVAILABLE_USAGE_UNKNOWN,
        "PNG 8-bit colormap 16x16",
        16u,
        16u,
        "No recovered source reference; purpose not assigned",
        "PNG decoded with palette/tRNS; source usage remains unknown",
        0
    },
    {
        "/sounds/up.ott",
        "sounds/up.ott",
        BOUNCE_ASSET_KIND_SOUND,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Nokia Sound FORMAT_TONE payload",
        0u,
        0u,
        "e.LoadSound(/sounds/up.ott)",
        "VERIFIED ASSET — RTPL DECODED (14B-23); semantics match 14B-19/20; identity/file check; audio synthesis and playback not implemented",
        0
    },
    {
        "/sounds/pickup.ott",
        "sounds/pickup.ott",
        BOUNCE_ASSET_KIND_SOUND,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Nokia Sound FORMAT_TONE payload",
        0u,
        0u,
        "e.LoadSound(/sounds/pickup.ott)",
        "VERIFIED ASSET — RTPL DECODED (14B-23); semantics match 14B-19/20; identity/file check; audio synthesis and playback not implemented",
        0
    },
    {
        "/sounds/pop.ott",
        "sounds/pop.ott",
        BOUNCE_ASSET_KIND_SOUND,
        BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME,
        "Nokia Sound FORMAT_TONE payload",
        0u,
        0u,
        "e.LoadSound(/sounds/pop.ott)",
        "VERIFIED ASSET — RTPL DECODED (14B-23); semantics match 14B-19/20; identity/file check; audio synthesis and playback not implemented",
        0
    },
    {
        "/lang.xx",
        "lang.xx",
        BOUNCE_ASSET_KIND_LANGUAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT,
        "Java ME translation table",
        0u,
        0u,
        "Translation fallback /lang.xx",
        "VERIFIED ASSET — DECODER NOT YET IMPLEMENTED; BounceAsset identity/file check; translation decoder not implemented",
        0
    },
    {
        "/lang.zh-CN",
        "lang.zh-CN",
        BOUNCE_ASSET_KIND_LANGUAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT,
        "Java ME translation table",
        0u,
        0u,
        "Translation locale resource /lang.<locale>",
        "VERIFIED ASSET — DECODER NOT YET IMPLEMENTED; BounceAsset identity/file check; translation decoder not implemented",
        0
    },
    {
        "/lang.zh-TW",
        "lang.zh-TW",
        BOUNCE_ASSET_KIND_LANGUAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT,
        "Java ME translation table",
        0u,
        0u,
        "Translation locale resource /lang.<locale>",
        "VERIFIED ASSET — DECODER NOT YET IMPLEMENTED; BounceAsset identity/file check; translation decoder not implemented",
        0
    },
    {
        "/lang.th-TH",
        "lang.th-TH",
        BOUNCE_ASSET_KIND_LANGUAGE,
        BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT,
        "Java ME translation table",
        0u,
        0u,
        "Translation locale resource /lang.<locale>",
        "VERIFIED ASSET — DECODER NOT YET IMPLEMENTED; BounceAsset identity/file check; translation decoder not implemented",
        0
    },
    {
        "/META-INF/MANIFEST.MF",
        "META-INF/MANIFEST.MF",
        BOUNCE_ASSET_KIND_MANIFEST,
        BOUNCE_ASSET_COVERAGE_VERIFIED_AVAILABLE_USAGE_UNKNOWN,
        "JAR manifest",
        0u,
        0u,
        "Main-Class com.nokia.mid.appl.boun.Main; runtime use not applicable",
        "BounceAsset identity and file check; packaging metadata, not rendered",
        0
    }
};

static const char *coverage_name(BounceAssetCoverage coverage)
{
    switch (coverage) {
        case BOUNCE_ASSET_COVERAGE_VERIFIED_USED_BY_GAME:
            return "VERIFIED USED BY GAME";
        case BOUNCE_ASSET_COVERAGE_VERIFIED_AVAILABLE_USAGE_UNKNOWN:
            return "VERIFIED AVAILABLE BUT USAGE NOT YET RECOVERED";
        case BOUNCE_ASSET_COVERAGE_VERIFIED_SOURCE_DECODER_INPUT:
            return "VERIFIED SOURCE/DECODER INPUT";
        case BOUNCE_ASSET_COVERAGE_UNKNOWN_REQUIRES_INVESTIGATION:
            return "UNKNOWN / REQUIRES FUTURE INVESTIGATION";
    }
    return "UNKNOWN / REQUIRES FUTURE INVESTIGATION";
}

static const char *kind_name(BounceAssetKind kind)
{
    switch (kind) {
        case BOUNCE_ASSET_KIND_LEVEL:
            return "level";
        case BOUNCE_ASSET_KIND_IMAGE:
            return "image";
        case BOUNCE_ASSET_KIND_SOUND:
            return "sound";
        case BOUNCE_ASSET_KIND_LANGUAGE:
            return "language";
        case BOUNCE_ASSET_KIND_MANIFEST:
            return "manifest";
    }
    return "unknown";
}

const BounceAssetInventoryEntry *bounce_asset_inventory_entries(
    size_t *count_out
)
{
    if (count_out != NULL)
        *count_out = sizeof inventory / sizeof inventory[0];
    return inventory;
}

static int make_filesystem_path(
    const char *resource_root,
    const char *relative_path,
    char *path_out,
    size_t path_size
)
{
    int written;

    if (resource_root == NULL
        || relative_path == NULL
        || path_out == NULL
        || path_size == 0u)
        return -1;
    written = snprintf(
        path_out,
        path_size,
        "%s/%s",
        resource_root,
        relative_path
    );
    if (written < 0 || (size_t)written >= path_size)
        return -1;
    return 0;
}

static int file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    int close_status;

    if (file == NULL)
        return 0;
    close_status = fclose(file);
    return close_status == 0;
}

static int verify_level_entry(
    const BounceAssetInventoryEntry *entry,
    const char *filesystem_path
)
{
    BounceLevel *level = NULL;
    BounceRuntimeLevelTiles *runtime;
    BounceLevelLoadStatus status;
    int result = -1;

    status = bounce_level_load_file(filesystem_path, &level);
    if (status != BOUNCE_LEVEL_LOAD_OK || level == NULL)
        goto done;
    if (bounce_level_width(level) != entry->width
        || bounce_level_height(level) != entry->height)
        goto done;
    runtime = bounce_runtime_level_tiles_create(level);
    if (runtime == NULL)
        goto done;
    if (bounce_runtime_level_tiles_width(runtime) != entry->width
        || bounce_runtime_level_tiles_height(runtime) != entry->height)
        goto done;
    bounce_runtime_level_tiles_destroy(runtime);
    result = 0;

done:
    bounce_level_destroy(level);
    return result;
}

static int verify_entry(
    const BounceAssetInventoryEntry *entry,
    const char *resource_root,
    FILE *output_file
)
{
    char filesystem_path[512];
    BounceAsset *asset = NULL;
    BounceImage *decoded_image = NULL;
    int result = -1;

    if (make_filesystem_path(
            resource_root,
            entry->relative_filesystem_path,
            filesystem_path,
            sizeof filesystem_path
        ) != 0
        || !file_exists(filesystem_path))
        goto done;

    asset = bounce_asset_create(entry->resource_path);
    if (asset == NULL
        || bounce_asset_resource_path(asset) == NULL)
        goto done;

    if (entry->kind == BOUNCE_ASSET_KIND_IMAGE) {
        BounceAssetDecodeStatus decode_status = bounce_asset_decode_file(
            filesystem_path,
            &decoded_image
        );
        if (decode_status != BOUNCE_ASSET_DECODE_OK
            || bounce_image_width(decoded_image) != entry->width
            || bounce_image_height(decoded_image) != entry->height)
            goto done;
    }
    if (entry->kind == BOUNCE_ASSET_KIND_LEVEL
        && verify_level_entry(entry, filesystem_path) != 0)
        goto done;

    if (output_file != NULL) {
        (void)fprintf(
            output_file,
            "%s | %s | %s | %ux%u | identity=ok file=ok | rendered=%s | %s\n",
            coverage_name(entry->coverage),
            entry->resource_path,
            kind_name(entry->kind),
            entry->width,
            entry->height,
            entry->rendered != 0 ? "yes" : "no",
            entry->native_status
        );
    }
    result = 0;

done:
    bounce_image_destroy(decoded_image);
    bounce_asset_destroy(asset);
    return result;
}

int bounce_asset_inventory_check(
    const char *resource_root,
    FILE *output_file
)
{
    const BounceAssetInventoryEntry *entries;
    size_t count;
    size_t index;

    if (resource_root == NULL)
        return -1;
    entries = bounce_asset_inventory_entries(&count);
    if (entries == NULL || count == 0u)
        return -1;

    for (index = 0u; index < count; ++index) {
        if (verify_entry(&entries[index], resource_root, output_file) != 0) {
            (void)fprintf(
                stderr,
                "asset inventory check failed: %s\n",
                entries[index].resource_path
            );
            return -1;
        }
    }
    return 0;
}
