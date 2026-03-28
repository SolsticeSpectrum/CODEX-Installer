#!/bin/bash
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
DIST="$DIR/dist"

rm -rf "$DIST" && mkdir "$DIST"

variants=(
    "CODEX:5:1:1:CODEX"
    "PLAZA:7:2:2:PLAZA"
    "RUNE:8:3:3:RUNE"
    "ENDEREX:9:4:4:Amakrits"
)

for entry in "${variants[@]}"; do
    IFS=: read -r VAR LOGO ICON MUSIC THEME <<< "$entry"
    rm -rf "$DIR/build"
    bash "$DIR/build_variant.sh" $VAR $LOGO $ICON $MUSIC $THEME 2>&1 | tail -1
    cp "$DIR/build/$(echo $VAR | tr A-Z a-z)-installer" "$DIST/"
done

for name in codex plaza rune enderex; do
    pkill -f "${name}-installer" 2>/dev/null || true
    sleep 0.5

    "$DIST/${name}-installer" &
    PID=$!
    sleep 2

    GEOM="$(hyprctl clients -j | python3 -c "
import sys,json;cs=json.load(sys.stdin)
c=[x for x in cs if 'Example' in x.get('title','')]
if c: print(f'{c[0][\"at\"][0]},{c[0][\"at\"][1]} {c[0][\"size\"][0]}x{c[0][\"size\"][1]}')
")"

    if [ -z "$GEOM" ]; then
        echo "ERROR: could not find window for ${name}-installer" >&2
        kill $PID 2>/dev/null || true
        continue
    fi

    grim -g "$GEOM" "/tmp/ss_${name}.png"
    echo "captured ${name}: ${GEOM}"
    kill $PID 2>/dev/null || true
done

pkill -f -- "-installer" 2>/dev/null || true

python3 -c "
from PIL import Image
names = ['codex','plaza','rune','enderex']
imgs = [Image.open(f'/tmp/ss_{n}.png') for n in names]
for n,i in zip(names, imgs): print(f'{n}: {i.width}x{i.height}')
max_h = max(i.height for i in imgs)
out = Image.new('RGBA', (sum(i.width for i in imgs), max_h), (0,0,0,255))
x = 0
for img in imgs:
    out.paste(img, (x, 0))
    x += img.width
out.save('$HOME/Obrázky/codex_linux_all_variants.png')
print(f'combined: {out.width}x{out.height}')
"
