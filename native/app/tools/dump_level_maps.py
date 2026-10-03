#!/usr/bin/env python3
"""Offline static level-map dump for the original Nokia Bounce level binaries.

This is a developer inspection tool. It is NOT part of the game, it is not
linked into the native build, and it changes no gameplay, rendering, or level
loading behaviour. It reads the original level binaries and the original
objects atlas exactly as the already-verified native code does, and writes one
PNG per level so the complete shape of each map can be inspected.

Everything this tool reproduces is transcribed from verified sources:

  level binary layout   native/level/level.c:78-116
                        width = data[6], height = data[7], tiles = data[8:],
                        then a thorn count byte and thorn_count * 8 payload
                        bytes; the total size must match exactly.

  tile value decoding   f.java:431  tileId = value & (~0x40) & (~0x80)
                        f.java:430  0x40 is the water bit

  atlas geometry        native/app/visual_assets.c:13-15
                        12 px tiles, 4 columns, 6 rows
                        src/main/resources/icons/objects_nm.png is 48x72

  sprite construction   native/app/visual_assets.c:495-660 (visual_build_sprites)
                        and :460-491 (visual_build_directional_assets)

  transforms            native/app/visual_assets.c:133-173
                        (visual_source_coordinate)

  tile -> sprite        native/app/visual_assets.c:857-1035
                        (bounce_visual_assets_draw_tile)

  tile backgrounds      native/app/visual_assets.c:875-877
                        sky 0xFFB0E0F0, water 0xFF1060B0

Deliberately NOT reproduced, because the native has no verified static
representation for them and inventing one is not allowed:

  tile 9   exit            animated reveal, b.java:812-830 / e.java:285-295
  tile 10  dynamic thorn   moving 24x24 obstacle, b.java:320-325 / :463-479

Those are drawn as the plain tile background only, which is exactly what
bounce_visual_assets_draw_tile does for them (its default arm returns 0 after
the background fill). The tool reports every tile id it encountered and flags
the ones with no verified visual, so a missing graphic is never mistaken for
empty space.

No camera scrolling, player, HUD, menu, animation, or runtime effect is drawn.
This is a static dump of the whole map.
"""

import argparse
import collections
import os
import struct
import sys
import zlib

try:
    from PIL import Image
except ImportError:  # pragma: no cover - the repository toolchain has Pillow
    sys.stderr.write(
        "dump_level_maps: Pillow is required to decode objects_nm.png\n"
    )
    raise SystemExit(2)


# ---------------------------------------------------------------------------
# Constants transcribed from the verified native sources (see module docstring).
# ---------------------------------------------------------------------------

TILE_SIZE = 12                    # visual_assets.c:13 BOUNCE_ATLAS_TILE_SIZE
ATLAS_COLUMNS = 4                 # visual_assets.c:14 BOUNCE_ATLAS_COLUMNS
ATLAS_ROWS = 6                    # visual_assets.c:15 BOUNCE_ATLAS_ROWS
SPRITE_COUNT = 67                 # visual_assets.c:16 BOUNCE_SPRITE_COUNT

# All colours are stored as (R, G, B, A) so they can be written straight into
# an RGBA PNG and read straight out of the decoded atlas. The native packs the
# same values as uint32 ARGB; the native spelling is given in each comment.
SKY = (0xB0, 0xE0, 0xF0, 0xFF)   # visual_argb(255,176,224,240)  visual_assets.c:497
WATER = (0x10, 0x60, 0xB0, 0xFF)  # visual_argb(255,16,96,176)    visual_assets.c:498
TRANSPARENT = (0x00, 0x00, 0x00, 0x00)

# visual_argb(255,252,157,158) / (255,227,58,63) / (255,194,132,142)
# visual_assets.c:333-335
EXIT_OUTER = (0xFC, 0x9D, 0x9E, 0xFF)
EXIT_MIDDLE = (0xE3, 0x3A, 0x3F, 0xFF)
EXIT_INNER = (0xC2, 0x84, 0x8E, 0xFF)

# visual_assets.c:22-27
T_NONE = "none"
T_FLIP_H = "flip_h"
T_FLIP_V = "flip_v"
T_FLIP_BOTH = "flip_both"
T_ROT_90 = "rot_90"
T_ROT_180 = "rot_180"
T_ROT_270 = "rot_270"

# visual_assets.c:462-468: the four directional variants, in index order.
DIRECTIONAL_TRANSFORMS = (T_NONE, T_ROT_270, T_ROT_180, T_ROT_90)

# visual_assets.c:44-52, byte-identical to d.java a[] / b[].
TILE_FRONT_SPRITES = (
    35, 36, 17, 19, 43, 44, 25, 27,
    31, 32, 13, 15, 39, 40, 21, 23,
)
TILE_BACK_SPRITES = (
    33, 34, 18, 20, 41, 42, 26, 28,
    29, 30, 14, 16, 37, 38, 22, 24,
)

# visual_assets.c:495-660: sprite construction.
#   (sprite_index, atlas_x, atlas_y, background, transform)
#   or ("X", sprite_index, source_sprite, transform) for visual_set_transformed
SPRITE_TILES = (
    (0, 1, 0, TRANSPARENT, T_NONE),
    (1, 1, 2, TRANSPARENT, T_NONE),
    (2, 0, 3, SKY, T_NONE),
    (3, 2, T_FLIP_V),
    (4, 2, T_ROT_90),
    (5, 2, T_ROT_270),
    (6, 0, 3, WATER, T_NONE),
    (7, 6, T_FLIP_V),
    (8, 6, T_ROT_90),
    (9, 6, T_ROT_270),
    (10, 0, 4, TRANSPARENT, T_NONE),
    (11, 3, 4, TRANSPARENT, T_NONE),
)
SPRITE_TILES_HOOPS = (
    (14, 0, 5, TRANSPARENT, T_NONE),
    (13, 14, T_FLIP_V),
    (15, 13, T_FLIP_H),
    (16, 14, T_FLIP_H),
    (18, 1, 5, TRANSPARENT, T_NONE),
    (17, 18, T_FLIP_V),
    (19, 17, T_FLIP_H),
    (20, 18, T_FLIP_H),
    (22, 2, 5, TRANSPARENT, T_NONE),
    (21, 22, T_FLIP_V),
    (23, 21, T_FLIP_H),
    (24, 22, T_FLIP_H),
    (26, 3, 5, TRANSPARENT, T_NONE),
    (25, 26, T_FLIP_V),
    (27, 25, T_FLIP_H),
    (28, 26, T_FLIP_H),
)
SPRITE_TILES_ROTATED = (
    (29, 14, T_ROT_270),
    (30, 29, T_FLIP_V),
    (31, 29, T_FLIP_H),
    (32, 30, T_FLIP_H),
    (33, 18, T_ROT_270),
    (34, 33, T_FLIP_V),
    (35, 33, T_FLIP_H),
    (36, 34, T_FLIP_H),
    (37, 22, T_ROT_270),
    (38, 37, T_FLIP_V),
    (39, 37, T_FLIP_H),
    (40, 38, T_FLIP_H),
    (41, 26, T_ROT_270),
    (42, 41, T_FLIP_V),
    (43, 41, T_FLIP_H),
    (44, 42, T_FLIP_H),
)
SPRITE_TILES_TAIL = (
    (45, 3, 3, TRANSPARENT, T_NONE),
    (46, 1, 3, TRANSPARENT, T_NONE),
    (47, 2, 0, TRANSPARENT, T_NONE),
    (48, 0, 1, TRANSPARENT, T_NONE),
    (50, 3, 1, TRANSPARENT, T_NONE),
    (51, 2, 4, TRANSPARENT, T_NONE),
    (52, 3, 2, TRANSPARENT, T_NONE),
    (53, 1, 1, TRANSPARENT, T_NONE),
    (54, 2, 2, TRANSPARENT, T_NONE),
    (55, 0, 0, SKY, T_NONE),
    (56, 55, T_ROT_90),
    (57, 55, T_ROT_180),
    (58, 55, T_ROT_270),
    (59, 0, 0, WATER, T_NONE),
    (60, 59, T_ROT_90),
    (61, 59, T_ROT_180),
    (62, 59, T_ROT_270),
    (63, 0, 2, TRANSPARENT, T_NONE),
    (64, 63, T_ROT_90),
    (65, 63, T_ROT_180),
    (66, 63, T_ROT_270),
)

# visual_assets.c:460-491: base sprite for each directional group.
DIRECTIONAL_GROUPS = (
    ("deflater", 50),
    ("pumper", 51),
    ("gravity", 52),
    ("jump", 54),
)

# Tile ids with no verified static visual. Kept explicit so the report can name
# them instead of silently rendering background.
UNMAPPED_TILE_IDS = {
    9: "exit (animated reveal, b.java:812-830 / e.java:285-295)",
    10: "dynamic thorn (moving 24x24 obstacle, b.java:320-325 / :463-479)",
}


# ---------------------------------------------------------------------------
# Transforms and blending, transcribed from the native sources.
# ---------------------------------------------------------------------------

def source_coordinate(transform, dx, dy, width, height):
    """visual_assets.c:133-173 visual_source_coordinate."""
    if transform == T_FLIP_H:
        return width - 1 - dx, dy
    if transform == T_FLIP_V:
        return dx, height - 1 - dy
    if transform == T_FLIP_BOTH:
        return width - 1 - dx, height - 1 - dy
    if transform == T_ROT_90:
        return dy, height - 1 - dx
    if transform == T_ROT_180:
        return width - 1 - dx, height - 1 - dy
    if transform == T_ROT_270:
        return width - 1 - dy, dx
    return dx, dy


def alpha_over_opaque(source, destination):
    """renderer.c:226-258 bounce_renderer_alpha_over_opaque.

    A palette+tRNS atlas yields only alpha 0 or 255, so in practice this is
    "keep the destination if the source is clear, otherwise take the source".
    The partial-alpha branch is kept so the tool stays faithful if the atlas is
    ever re-encoded.
    """
    sa = source[3]
    if sa == 0:
        return destination
    if sa == 255:
        return source
    ia = 255 - sa
    return (
        255,
        (source[0] * sa + destination[0] * ia + 127) // 255,
        (source[1] * sa + destination[1] * ia + 127) // 255,
        (source[2] * sa + destination[2] * ia + 127) // 255,
    )


# ---------------------------------------------------------------------------
# Atlas and sprite construction.
# ---------------------------------------------------------------------------

class Tile:
    """A fixed-size RGBA pixel buffer, mirroring BounceImage."""

    __slots__ = ("width", "height", "pixels")

    def __init__(self, width, height, fill=TRANSPARENT):
        self.width = width
        self.height = height
        self.pixels = [fill] * (width * height)

    def fill(self, colour):
        self.pixels[:] = [colour] * (self.width * self.height)

    def fill_rect(self, x, y, w, h, colour):
        for row in range(y, y + h):
            for col in range(x, x + w):
                self.pixels[row * self.width + col] = colour

    def get(self, x, y):
        return self.pixels[y * self.width + x]

    def set(self, x, y, colour):
        self.pixels[y * self.width + x] = colour

    def transformed(self, transform):
        """visual_assets.c:234-271 visual_transform_image."""
        out = Tile(self.width, self.height)
        for y in range(self.height):
            for x in range(self.width):
                sx, sy = source_coordinate(
                    transform, x, y, self.width, self.height
                )
                out.pixels[y * self.width + x] = self.pixels[
                    sy * self.width + sx
                ]
        return out


def load_atlas(path):
    with Image.open(path) as handle:
        if handle.mode != "RGBA":
            handle = handle.convert("RGBA")
        else:
            handle = handle.copy()
        width, height = handle.size
    if (width, height) != (ATLAS_COLUMNS * TILE_SIZE, ATLAS_ROWS * TILE_SIZE):
        raise SystemExit(
            "dump_level_maps: atlas is %dx%d, expected %dx%d"
            % (
                width,
                height,
                ATLAS_COLUMNS * TILE_SIZE,
                ATLAS_ROWS * TILE_SIZE,
            )
        )
    atlas = Tile(width, height)
    data = handle.tobytes()
    for y in range(height):
        for x in range(width):
            atlas.set(x, y, tuple(data[(y * width + x) * 4:(y * width + x) * 4 + 4]))
    return atlas


def tile_from_atlas(atlas, atlas_x, atlas_y, background, transform):
    """visual_assets.c:166-221 visual_tile_from_atlas."""
    if not (0 <= atlas_x < ATLAS_COLUMNS and 0 <= atlas_y < ATLAS_ROWS):
        raise SystemExit(
            "dump_level_maps: atlas cell (%d,%d) is out of range"
            % (atlas_x, atlas_y)
        )
    left = atlas_x * TILE_SIZE
    top = atlas_y * TILE_SIZE
    out = Tile(TILE_SIZE, TILE_SIZE, background)
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            sx, sy = source_coordinate(
                transform, x, y, TILE_SIZE, TILE_SIZE
            )
            pixel = atlas.get(left + sx, top + sy)
            if pixel[3] != 0:
                out.set(x, y, pixel)
    return out


def exit_tile(atlas):
    """visual_assets.c:327-398 visual_exit_tile (24x48, the MirrorExitTile)."""
    out = Tile(24, 48, SKY)
    out.fill_rect(4, 0, 16, 48, EXIT_OUTER)
    out.fill_rect(6, 0, 10, 48, EXIT_MIDDLE)
    out.fill_rect(10, 0, 4, 48, EXIT_INNER)
    for y in range(12):
        for x in range(12):
            pixel = atlas.get(2 * TILE_SIZE + x, 3 * TILE_SIZE + y)
            if pixel[3] == 0:
                continue
            out.set(x, y, pixel)
            out.set(23 - x, y, pixel)
    for y in range(12):
        for x in range(12):
            pixel = atlas.get(2 * TILE_SIZE + x, 3 * TILE_SIZE + y)
            if pixel[3] == 0:
                continue
            out.set(x, 23 - y, pixel)
            out.set(23 - x, 23 - y, pixel)
    return out


def build_sprites(atlas):
    """visual_assets.c:495-660 visual_build_sprites."""
    sprites = [None] * SPRITE_COUNT

    def set_tile(index, ax, ay, background, transform):
        sprites[index] = tile_from_atlas(atlas, ax, ay, background, transform)

    def set_transformed(index, source, transform):
        if sprites[source] is None:
            raise SystemExit(
                "dump_level_maps: sprite %d built from unset sprite %d"
                % (index, source)
            )
        sprites[index] = sprites[source].transformed(transform)

    for group in (SPRITE_TILES, SPRITE_TILES_HOOPS, SPRITE_TILES_TAIL):
        for entry in group:
            if len(entry) == 5:
                set_tile(*entry)
            else:
                set_transformed(entry[0], entry[1], entry[2])
    for entry in SPRITE_TILES_ROTATED:
        set_transformed(entry[0], entry[1], entry[2])

    # visual_assets.c:519
    sprites[12] = exit_tile(atlas)

    # visual_assets.c:460-491: the four directional groups. They are appended
    # to the same flat table so every tile mapping is a plain integer index.
    for name, base in DIRECTIONAL_GROUPS:
        if sprites[base] is None:
            raise SystemExit(
                "dump_level_maps: directional base sprite %d unset" % base
            )
        for transform in DIRECTIONAL_TRANSFORMS:
            sprites.append(sprites[base].transformed(transform))
    return sprites


# Flat indices of the directional groups, in the order build_sprites appends
# them. visual_assets.c:460-491.
GROUP_BASE = {}
_next = SPRITE_COUNT
for _name, _ in DIRECTIONAL_GROUPS:
    GROUP_BASE[_name] = _next
    _next += 4


# ---------------------------------------------------------------------------
# Tile -> sprites, transcribed from bounce_visual_assets_draw_tile.
# ---------------------------------------------------------------------------

def tile_sprites(base_tile, water):
    """visual_assets.c:886-1035. Returns a list of sprites, front first."""
    if 13 <= base_tile <= 28:
        offset = base_tile - 13
        return [TILE_FRONT_SPRITES[offset], TILE_BACK_SPRITES[offset]]
    if base_tile == 0:
        return []
    simple = {
        1: 0,
        2: 1,
        7: 10,
        8: 11,
        12: 12,
        29: 45,
        34: 65,
        35: 64,
        36: 63,
        37: 66,
        38: 53,
    }
    if base_tile in simple:
        return [simple[base_tile]]
    if 3 <= base_tile <= 6:
        return [2 + base_tile - 3]
    if 30 <= base_tile <= 33:
        pairs = {30: (61, 57), 31: (60, 56), 32: (59, 55), 33: (62, 58)}
        return [pairs[base_tile][0] if water else pairs[base_tile][1]]
    if 39 <= base_tile <= 42:
        return [GROUP_BASE["deflater"] + base_tile - 39]
    if 43 <= base_tile <= 46:
        return [GROUP_BASE["pumper"] + base_tile - 43]
    if 47 <= base_tile <= 50:
        return [GROUP_BASE["gravity"] + base_tile - 47]
    if 51 <= base_tile <= 54:
        return [GROUP_BASE["jump"] + base_tile - 51]
    # base 9 and 10 land here, as does anything else: background only, which is
    # exactly what the native default arm does.
    return []


# ---------------------------------------------------------------------------
# Level parsing, transcribed from native/level/level.c:78-116.
# ---------------------------------------------------------------------------

def parse_level(path):
    with open(path, "rb") as handle:
        data = handle.read()
    if len(data) < 8:
        raise SystemExit("dump_level_maps: %s is shorter than 8 bytes" % path)
    width = data[6]
    height = data[7]
    if width == 0 or height == 0:
        raise SystemExit("dump_level_maps: %s has a zero dimension" % path)
    tile_offset = 8
    tile_count = width * height
    tail_offset = tile_offset + tile_count
    if len(data) < tail_offset + 1:
        raise SystemExit("dump_level_maps: %s is truncated" % path)
    thorn_count = data[tail_offset]
    expected = tail_offset + 1 + thorn_count * 8
    if len(data) != expected:
        raise SystemExit(
            "dump_level_maps: %s size %d does not match the parsed layout %d"
            % (path, len(data), expected)
        )
    return {
        "width": width,
        "height": height,
        "tiles": data[tile_offset:tail_offset],
        "thorn_count": thorn_count,
        "ball_size": 12 if data[2] == 0 else 16,
        "exit_x": data[3],
        "exit_y": data[4],
    }


def render_map(level, sprites):
    """visual_assets.c:857-885 background fill plus the sprite blit."""
    width = level["width"]
    height = level["height"]
    canvas = Tile(width * TILE_SIZE, height * TILE_SIZE)
    unknown = collections.Counter()
    for ty in range(height):
        for tx in range(width):
            value = level["tiles"][ty * width + tx]
            water = (value & 0x40) != 0
            base = value & ~0x40 & ~0x80
            indices = tile_sprites(base, water)
            if not indices and base not in (0,):
                unknown[base] += 1
            background = WATER if water else SKY
            ox = tx * TILE_SIZE
            oy = ty * TILE_SIZE
            for row in range(TILE_SIZE):
                for col in range(TILE_SIZE):
                    canvas.set(ox + col, oy + row, background)
            for index in indices:
                sprite = sprites[index]
                if sprite is None:
                    raise SystemExit(
                        "dump_level_maps: sprite %d was never built" % index
                    )
                for row in range(sprite.height):
                    for col in range(sprite.width):
                        source = sprite.get(col, row)
                        if source[3] == 0:
                            continue
                        x = ox + col
                        y = oy + row
                        if 0 <= x < canvas.width and 0 <= y < canvas.height:
                            canvas.set(
                                x, y, alpha_over_opaque(source, canvas.get(x, y))
                            )
    return canvas, unknown


# ---------------------------------------------------------------------------
# PNG output.
# ---------------------------------------------------------------------------

def _chunk(tag, payload):
    return (
        struct.pack(">I", len(payload))
        + tag
        + payload
        + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    )


def write_png(path, tile):
    raw = bytearray()
    for y in range(tile.height):
        raw.append(0)  # filter type 0, no prediction
        for x in range(tile.width):
            raw.extend(tile.get(x, y))
    header = struct.pack(">IIBBBBB", tile.width, tile.height, 8, 6, 0, 0, 0)
    data = (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", header)
        + _chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + _chunk(b"IEND", b"")
    )
    with open(path, "wb") as handle:
        handle.write(data)


# ---------------------------------------------------------------------------

def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    app_dir = os.path.dirname(here)
    native_dir = os.path.dirname(app_dir)
    repo = os.path.dirname(native_dir)

    parser = argparse.ArgumentParser(
        description="Dump the original level binaries to PNG maps."
    )
    parser.add_argument(
        "--levels",
        default=os.path.join(repo, "src", "main", "resources", "levels"),
        help="directory holding J2MElvl.NNN",
    )
    parser.add_argument(
        "--atlas",
        default=os.path.join(
            repo, "src", "main", "resources", "icons", "objects_nm.png"
        ),
        help="path to objects_nm.png",
    )
    parser.add_argument(
        "--out",
        default=os.path.expanduser("~/Documents/nokia-bounce"),
        help="output directory for mapNN.png",
    )
    args = parser.parse_args(argv)

    sources = sorted(
        os.path.join(args.levels, "J2MElvl.%03d" % n) for n in range(1, 12)
    )
    missing = [p for p in sources if not os.path.isfile(p)]
    if missing:
        raise SystemExit(
            "dump_level_maps: missing level file(s): %s"
            % ", ".join(missing)
        )

    atlas = load_atlas(args.atlas)
    sprites = build_sprites(atlas)

    os.makedirs(args.out, exist_ok=True)

    all_unknown = collections.Counter()
    for index, source in enumerate(sources, start=1):
        level = parse_level(source)
        canvas, unknown = render_map(level, sprites)
        all_unknown.update(unknown)
        target = os.path.join(args.out, "map%02d.png" % index)
        write_png(target, canvas)
        print(
            "LEVEL %02d:\n"
            "    source = %s\n"
            "    map dimensions = %dx%d tiles (%dx%d px at %d px/tile)\n"
            "    output = %s\n"
            "    PNG dimensions = %dx%d\n"
            "    tiles rendered = %d\n"
            "    ball size = %d, exit = (%d,%d), dynamic thorns = %d\n"
            "    unmapped tile cells = %s"
            % (
                index,
                source,
                level["width"],
                level["height"],
                canvas.width,
                canvas.height,
                TILE_SIZE,
                target,
                canvas.width,
                canvas.height,
                level["width"] * level["height"],
                level["ball_size"],
                level["exit_x"],
                level["exit_y"],
                level["thorn_count"],
                ", ".join(
                    "id %d x%d (%s)" % (k, v, UNMAPPED_TILE_IDS.get(k, "no verified static visual"))
                    for k, v in sorted(unknown.items())
                )
                or "none",
            )
        )

    print("")
    if all_unknown:
        print("TILE IDS WITH NO VERIFIED STATIC VISUAL (drawn as background):")
        for tile_id, count in sorted(all_unknown.items()):
            print(
                "    id %-3d %6d cells  %s"
                % (tile_id, count, UNMAPPED_TILE_IDS.get(tile_id, "unmapped"))
            )
    else:
        print("TILE IDS WITH NO VERIFIED STATIC VISUAL: none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
