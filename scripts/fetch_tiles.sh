#!/usr/bin/env bash
# Download a raster tile pyramid from MapTiler into ./tiles/{z}/{x}/{y}.png
# Usage: MAPTILER_KEY=... ./scripts/fetch_tiles.sh [max_zoom]
set -u

: "${MAPTILER_KEY:?export MAPTILER_KEY first}"

STYLE="aquarelle-v4"
MAXZ="${1:-5}"
UA="raster-map/0.1"

fails=0
for ((z = 0; z <= MAXZ; z++)); do
  n=$((1 << z))
  echo "z=$z ($((n * n)) tiles)"
  for ((x = 0; x < n; x++)); do
    mkdir -p "tiles/$z/$x"
    for ((y = 0; y < n; y++)); do
      out="tiles/$z/$x/$y.png"
      [ -s "$out" ] && continue # already have it - safe to re-run after a stop
      if ! curl -sf -A "$UA" \
        "https://api.maptiler.com/maps/$STYLE/256/$z/$x/$y.png?key=$MAPTILER_KEY" \
        -o "$out"; then
        rm -f "$out"
        echo "  fail $z/$x/$y"
        fails=$((fails + 1))
      fi
    done
  done
done

echo "total: $(find tiles -name '*.png' | wc -l | tr -d ' ') tiles, $fails failed"
