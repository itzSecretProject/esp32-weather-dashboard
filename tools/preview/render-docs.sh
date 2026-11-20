#!/usr/bin/env bash
# Builds the desktop preview and regenerates the screenshots in docs/images/.
# Usage: tools/preview/render-docs.sh [build-dir]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${1:-$ROOT/build/preview}"
FIX="$ROOT/tools/preview/fixtures"
OUT="$ROOT/docs/images"

cmake -S "$ROOT/tools/preview" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BUILD" -j

"$BUILD/preview" --weather "$FIX/design-sample.json" --cities "$FIX/design-sample-cities.json" \
  --location "Florida, US" --city2 "California, US" --city3 "Texas, US" \
  --now 1795005637 --imperial --12h --out "$OUT/dashboard-sample.png"

"$BUILD/preview" --weather "$FIX/paris.json" --cities "$FIX/paris-cities.json" \
  --out "$OUT/dashboard-paris.png"

"$BUILD/preview" --message "Connecting to Wi-Fi" "my-network" --out "$OUT/boot-screen.png"
