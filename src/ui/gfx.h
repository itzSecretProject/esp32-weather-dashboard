#pragma once
// Common graphics helpers used by the UI and icons.
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <math.h>
#include <stdint.h>

// Always RGB888. (LovyanGFX treats a plain `int` as RGB565, so we never pass raw literals.)
using Color = uint32_t;

constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return (Color(r) << 16) | (Color(g) << 8) | b; }

// t = 0 -> a, t = 1 -> b
inline Color mix(Color a, Color b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  auto ch = [&](int s) {
    float ca = (a >> s) & 0xFF, cb = (b >> s) & 0xFF;
    return Color(ca + (cb - ca) * t + 0.5f) << s;
  };
  return ch(16) | ch(8) | ch(0);
}

// Ordered (Bayer 4x4) dither so dark gradients don't band on a 16-bit (RGB565) panel.
inline Color dither(Color c, int x, int y) {
  static const uint8_t bayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
  int t = bayer[((y & 3) << 2) | (x & 3)];
  int r = ((c >> 16) & 0xFF) + (t >> 1);  // red/blue lose 3 bits -> 0..7
  int g = ((c >> 8) & 0xFF) + (t >> 2);   // green loses 2 bits   -> 0..3
  int b = (c & 0xFF) + (t >> 1);
  return rgb(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

inline Color readRGB(LovyanGFX& g, int x, int y) {
  auto c = g.readPixelRGB(x, y);
  return rgb(c.R8(), c.G8(), c.B8());
}

inline void blendPixel(LovyanGFX& g, int x, int y, Color c, float alpha) {
  if (x < 0 || y < 0 || x >= g.width() || y >= g.height() || alpha <= 0) return;
  g.drawPixel(x, y, mix(readRGB(g, x, y), c, alpha));
}
