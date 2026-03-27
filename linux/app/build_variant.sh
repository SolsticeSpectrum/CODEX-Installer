#!/bin/bash
set -e

VARIANT=$1
LOGO=$2
ICON=$3
MUSIC=$4
THEME=$5

if [ -z "$VARIANT" ] || [ -z "$LOGO" ] || [ -z "$ICON" ] || [ -z "$MUSIC" ] || [ -z "$THEME" ]; then
    echo "usage: $0 VARIANT LOGO ICON MUSIC THEME"
    echo "  e.g. $0 CODEX 5 1 1 CODEX"
    echo "       $0 PLAZA 7 2 2 PLAZA"
    echo "       $0 RUNE  8 3 3 RUNE"
    echo "       $0 ENDEREX 9 4 4 Amakrits"
    exit 1
fi

DIR="$(cd "$(dirname "$0")" && pwd)"

# pack assets
PACK=$(mktemp -d)
mkdir -p "$PACK/assets"
cp "$DIR/themes/$THEME/assets/"*.png "$PACK/assets/"
cp "$DIR/themes/$THEME/theme.json" "$PACK/"
cp "$DIR/layout.json" "$PACK/"
cp "$DIR/fonts/"*.ttf "$PACK/"
cp "$DIR/assets/Logo${LOGO}.bmp" "$PACK/"
cp "$DIR/assets/Icon${ICON}.png" "$PACK/" 2>/dev/null || cp "$DIR/assets/Icon${ICON}.ico" "$PACK/"
cp "$DIR/assets/Music${MUSIC}.ogg" "$PACK/"
cp "$DIR/assets/Play1.bmp" "$DIR/assets/Pause1.bmp" "$PACK/"
cp "$DIR/assets/TrackBkg.bmp" "$DIR/assets/TrackBtn1.bmp" "$PACK/"

ZIP=$(mktemp --suffix=.zip)
rm -f "$ZIP"
(cd "$PACK" && zip -q -9 -r "$ZIP" .)
xxd -i "$ZIP" > "$DIR/data/assets_data.h"
sed -i "s|unsigned char .*\[\]|unsigned char embedded_assets_zip[]|" "$DIR/data/assets_data.h"
sed -i "s|unsigned int .*_len|unsigned int embedded_assets_zip_len|" "$DIR/data/assets_data.h"
rm -rf "$PACK" "$ZIP"

# build
mkdir -p "$DIR/build"
cd "$DIR/build"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-DLOGO_NUM=\\\"$LOGO\\\" -DICON_NUM=\\\"$ICON\\\" -DMUSIC_NUM=\\\"$MUSIC\\\"" \
    > /dev/null 2>&1
make -j$(nproc)

mv installer "${VARIANT,,}-installer"
echo "built: ${VARIANT,,}-installer"
