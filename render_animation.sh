#!/usr/bin/env bash
# Render a frame sequence from a Lua scene (expects FRAME, FPS, QUALITY, OUTFILE like rolling_glass.lua).
# Run from anywhere; A5 root is the directory containing this script.
#
# Usage:
#   ./render_animation.sh Assets/[SCRIPT.lua]
#   ./render_animation.sh                      # defaults to sample.lua in Assets/
#   QUALITY=high ./render_animation.sh rolling_glass.lua
#   START_FRAME=0 END_FRAME=47 ./render_animation.sh simple.lua
#
# After rendering frames, muxes to <stem>_<QUALITY>.mp4 with ffmpeg (stem = SCRIPT without .lua).
#
# Builds the A5 binary first via make in the script directory. Override with MAKE_ARGS, e.g.:
#   MAKE_ARGS="config=release" ./render_animation.sh

set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Building A5 ==="
make -C "$DIR" ${MAKE_ARGS:-} 

A5_EXE="${A5_EXE:-$DIR/A5}"
QUALITY="${QUALITY:-low}"
START_FRAME="${START_FRAME:-0}"
END_FRAME="${END_FRAME:-143}"

if [[ "$QUALITY" != "low" && "$QUALITY" != "high" ]]; then
  echo "QUALITY must be low or high (got: $QUALITY)" >&2
  exit 1
fi

if [[ "$QUALITY" == "low" ]]; then
  FPS=48
else
  FPS=48
fi

LUA_FILE="${1:-sample.lua}"
LUA_FILE="$(basename "$LUA_FILE")"
STEM="${LUA_FILE%.lua}"

ASSETS="$DIR/Assets"
if [[ ! -f "$ASSETS/$LUA_FILE" ]]; then
  echo "Lua scene not found: $ASSETS/$LUA_FILE" >&2
  echo "Pass a .lua filename in $ASSETS/ (default: sample.lua)." >&2
  exit 1
fi

OUTDIR="${OUTDIR:-$DIR/frames_${QUALITY}_${STEM}}"
OUT_MP4="${OUT_MP4:-$DIR/${STEM}_${QUALITY}.mp4}"

mkdir -p "$OUTDIR"

if [[ ! -f "$A5_EXE" ]]; then
  echo "A5 executable not found at: $A5_EXE" >&2
  echo "Build with: cd \"$DIR\" && make" >&2
  exit 1
fi

cd "$ASSETS"

for FRAME in $(seq "$START_FRAME" "$END_FRAME"); do
  OUTFILE="$OUTDIR/$(printf 'frame_%04d.png' "$FRAME")"
  export FRAME FPS QUALITY OUTFILE
  "$A5_EXE" "$LUA_FILE"
done

echo ""
echo "Encoding: $OUT_MP4"
ffmpeg -y -framerate "$FPS" -i "$OUTDIR/frame_%04d.png" -c:v libx264 -pix_fmt yuv420p "$OUT_MP4"

echo "Done. Frames: $OUTDIR/frame_%04d.png"
echo "Video: $OUT_MP4"
