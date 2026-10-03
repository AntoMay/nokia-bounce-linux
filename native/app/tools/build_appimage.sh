#!/usr/bin/env bash
#
# Build Bounce-x86_64.AppImage from the working tree.
#
# WHAT IT DOES, IN ORDER, and why each step exists rather than being assumed:
#
#   1. Resizes src/main/resources/icons/icon.png to the sizes an AppImage wants.
#      That source is 16x16, so 256 is a 16x upscale and the result is soft. It is
#      used because it is the project's icon and the request was to resize it rather
#      than substitute another; bouncesplash.png at 128x128 is sharper if that is
#      ever wanted instead.
#
#   2. Assembles an AppDir. The data keeps its src/main/resources layout at the AppDir
#      ROOT because game.c builds level paths relative to the current working
#      directory -- "src/main/resources/levels/J2MElvl.%03d", with no configurable
#      prefix. AppRun therefore cd's to the mount point.
#
#   3. Copies the non-libc shared libraries the binary needs. libc, libm and the
#      dynamic loader are deliberately excluded: they are the kernel interface, and a
#      second libc inside an AppImage turns a missing library into a crash.
#
#   4. Runs appimagetool, which needs network access the first time because neither
#      appimagetool nor squashfs-tools is installed here. It is extracted into its OWN
#      directory: extracting an AppImage over an existing squashfs-root/ merges the two
#      trees, and a stale copy of the game's binary in there makes appimagetool exit 1
#      with nothing but the game's own usage text as the error.
#
# Usage:  native/app/tools/build_appimage.sh
# Output: dist/Bounce-x86_64.AppImage
set -euo pipefail

REPO=$(cd "$(dirname "$0")/../../.." && pwd)
WORK=${APPIMAGE_WORK:-/tmp/opencode/appimage-build}
APPID=io.github.antomay.bounce
OUT="$REPO/dist"

command -v magick >/dev/null || { echo "need ImageMagick (magick)" >&2; exit 1; }
[ -x "$REPO/native/app/bounce_vertical_slice" ] || {
    echo "build the game first: make -C native/app" >&2; exit 1; }

mkdir -p "$WORK" "$OUT"
rm -rf "$WORK/AppDir" "$WORK/tool"

# ---- 1. icon ----------------------------------------------------------------
echo "icon: resizing src/main/resources/icons/icon.png"
magick "$REPO/src/main/resources/icons/icon.png" \
    -filter Lanczos -resize 512x512 "$WORK/icon512.png"
for s in 256 128 64 48 32 16; do
    magick "$WORK/icon512.png" -filter Lanczos -resize "${s}x${s}" "$WORK/icon-$s.png"
done

# ---- 2. AppDir --------------------------------------------------------------
echo "appdir: assembling"
APPDIR="$WORK/AppDir"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
         "$APPDIR/usr/share/metainfo"

cp "$REPO/native/app/bounce_vertical_slice" "$APPDIR/usr/bin/"

# The CWD-relative layout the game resolves its levels against.
mkdir -p "$APPDIR/src/main"
cp -r "$REPO/src/main/resources" "$APPDIR/src/main/"

cp "$WORK/icon-256.png" "$APPDIR/bounce.png"
cp "$WORK/icon-256.png" \
   "$APPDIR/usr/share/icons/hicolor/256x256/apps/bounce.png"
cp "$WORK/icon-256.png" "$OUT/bounce-icon-256.png"

# ---- 3. libraries -----------------------------------------------------------
echo "libs: bundling"
for l in libX11.so.6 libxcb.so.1 libXau.so.6 libXdmcp.so.6 \
         libpng16.so.16 libasound.so.2 libz.so.1; do
    p=$(ldd "$REPO/native/app/bounce_vertical_slice" | awk -v n="$l" '$1==n {print $3}')
    [ -n "$p" ] || { echo "  missing $l" >&2; exit 1; }
    cp -L "$p" "$APPDIR/usr/lib/"
done

# ---- 4. desktop + metainfo + AppRun ----------------------------------------
cat > "$APPDIR/AppRun" <<'APPEOF'
#!/bin/sh
# BOUNCE -- native Linux reconstruction, packaged as an AppImage.
#
# THREE THINGS THIS SCRIPT EXISTS TO DO.
#
# 1. FIND THE MOUNT POINT. $APPDIR is exported by the AppImage runtime and is the only
#    variable correct in every mode. $0 is not: when the image is FUSE-mounted the
#    runtime may exec the AppImage's own path, so dirname of $0 is the .AppImage FILE
#    and not where its contents were mounted. The dirname is a fallback for running an
#    extracted AppDir directly.
#
# 2. cd THERE, because the game reads its data through paths relative to the CURRENT
#    WORKING DIRECTORY: game.c builds "src/main/resources/levels/J2MElvl.%03d" with no
#    configurable prefix. So the AppDir keeps that layout at its root. The mount is
#    read-only, which is fine -- the save lives under $XDG_DATA_HOME, never beside it.
#
# 3. PUT THE BUNDLED LIBRARIES FIRST, so the image runs on a host whose X11, libpng or
#    ALSA are a different generation. libc, libm and the loader are not bundled: they
#    are the kernel interface, and a second libc turns a missing library into a crash.
#
# Any argument is passed straight through; the game takes a level path positionally.
set -eu

if [ -n "${APPDIR:-}" ] && [ -d "$APPDIR" ]; then
    HERE="$APPDIR"
else
    HERE=$(dirname "$(readlink -f "$0")")
fi

if [ ! -d "$HERE/src/main/resources/levels" ]; then
    echo "Bounce: the level data is missing from $HERE." >&2
    echo "        expected $HERE/src/main/resources/levels/J2MElvl.001" >&2
    exit 1
fi

cd "$HERE"

LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH

exec "$HERE/usr/bin/bounce_vertical_slice" "$@"
APPEOF
chmod +x "$APPDIR/AppRun"

cat > "$APPDIR/bounce.desktop" <<'DESKEOF'
[Desktop Entry]
Type=Application
Name=Bounce
GenericName=Nokia Bounce
Comment=Nokia Bounce, reconstructed natively for Linux
Exec=AppRun
Icon=bounce
Categories=Game;ArcadeGame;
Terminal=false
DESKEOF
cp "$APPDIR/bounce.desktop" "$APPDIR/usr/share/applications/bounce.desktop"

# appstreamcli is run by appimagetool and treats warnings as fatal, so the file has
# to be named after the component id and carry a homepage, a developer and a rating.
cat > "$APPDIR/usr/share/metainfo/$APPID.appdata.xml" <<APPEOF2
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>$APPID</id>
  <name>Bounce</name>
  <summary>Nokia Bounce, reconstructed natively for Linux</summary>
  <developer_name>The nokia-bounce-linux contributors</developer_name>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>GPL-3.0-or-later</project_license>
  <url type="homepage">https://github.com/AntoMay/nokia-bounce-linux</url>
  <url type="bugtracker">https://github.com/AntoMay/nokia-bounce-linux/issues</url>
  <launchable type="desktop-id">bounce.desktop</launchable>
  <provides>
    <binary>bounce_vertical_slice</binary>
  </provides>
  <description>
    <p>
      A native Linux reconstruction of Nokia Bounce, built from the recovered Java
      sources. The tick period is 40 ms, taken from BounceTimer.java rather than from
      any other timing the sources imply.
    </p>
    <p>
      Eleven levels, the recovered locale resources, a settings screen, and a Reset to
      Default that returns a save to the state a fresh install starts in.
    </p>
  </description>
  <content_rating type="oars-1.1"/>
  <releases>
    <release version="1.0.0" date="2026-10-04"/>
  </releases>
</component>
APPEOF2

# ---- 5. appimagetool --------------------------------------------------------
TOOL="$WORK/tool"
if [ ! -x "$TOOL/squashfs-root/AppRun" ]; then
    echo "appimagetool: fetching (needs network once)"
    curl -sL -o "$WORK/appimagetool" \
      https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$WORK/appimagetool"
    mkdir -p "$TOOL"
    # Into its own directory, never over an existing squashfs-root/.
    ( cd "$TOOL" && "$WORK/appimagetool" --appimage-extract >/dev/null )
fi

ARCH=x86_64 "$TOOL/squashfs-root/AppRun" "$APPDIR" "$OUT/Bounce-x86_64.AppImage"
chmod +x "$OUT/Bounce-x86_64.AppImage"

echo
echo "built $OUT/Bounce-x86_64.AppImage ($(stat -c%s "$OUT/Bounce-x86_64.AppImage") bytes)"
