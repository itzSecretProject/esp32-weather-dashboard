#include "icons.h"

Sky skyFromWmo(uint8_t c) {
  if (c == 0) return Sky::Clear;
  if (c <= 2) return Sky::PartlyCloudy;
  if (c == 3) return Sky::Cloudy;
  if (c == 45 || c == 48) return Sky::Fog;
  if (c >= 51 && c <= 57) return Sky::Drizzle;
  if ((c >= 61 && c <= 67) || (c >= 80 && c <= 82)) return Sky::Rain;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return Sky::Snow;
  if (c >= 95) return Sky::Thunder;
  return Sky::Cloudy;
}

const char* wmoDescription(uint8_t c, bool isDay) {
  switch (c) {
    case 0:  return isDay ? "Clear Sky" : "Clear Night";
    case 1:  return "Mainly Clear";
    case 2:  return "Partly Cloudy";
    case 3:  return "Overcast Clouds";
    case 45: return "Foggy";
    case 48: return "Freezing Fog";
    case 51: return "Light Drizzle";
    case 53: return "Drizzle";
    case 55: return "Heavy Drizzle";
    case 56: case 57: return "Freezing Drizzle";
    case 61: return "Light Rain";
    case 63: return "Rainy Clouds";
    case 65: return "Heavy Rain";
    case 66: case 67: return "Freezing Rain";
    case 71: return "Light Snow";
    case 73: return "Snowy Clouds";
    case 75: return "Heavy Snow";
    case 77: return "Snow Grains";
    case 80: return "Rain Showers";
    case 81: return "Heavy Showers";
    case 82: return "Violent Showers";
    case 85: case 86: return "Snow Showers";
    case 95: return "Rainy Storm Clouds";
    case 96: case 99: return "Storm With Hail";
    default: return "Unknown";
  }
}

// ---------------------------------------------------------------------------
// Per-pixel helpers
// ---------------------------------------------------------------------------
static inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static inline int   fl(float v) { return (int)floorf(v); }
static inline int   ce(float v) { return (int)ceilf(v); }

static float segDist(float px, float py, float x0, float y0, float x1, float y1) {
  float dx = x1 - x0, dy = y1 - y0;
  float l2 = dx * dx + dy * dy;
  float t = l2 > 0 ? clamp01(((px - x0) * dx + (py - y0) * dy) / l2) : 0;
  float ex = px - (x0 + t * dx), ey = py - (y0 + t * dy);
  return sqrtf(ex * ex + ey * ey);
}

void aaLine(LovyanGFX& g, float x0, float y0, float x1, float y1, float thick, Color c) {
  float h = thick / 2 + 1;
  int xa = fl(fminf(x0, x1) - h), xb = ce(fmaxf(x0, x1) + h);
  int ya = fl(fminf(y0, y1) - h), yb = ce(fmaxf(y0, y1) + h);
  for (int y = ya; y <= yb; y++)
    for (int x = xa; x <= xb; x++) {
      float cov = clamp01(thick / 2 - segDist(x + 0.5f, y + 0.5f, x0, y0, x1, y1) + 0.5f);
      if (cov > 0) blendPixel(g, x, y, c, cov);
    }
}

void aaArc(LovyanGFX& g, float cx, float cy, float r, float thick, float a0, float a1, Color c) {
  float ext = r + thick / 2 + 1;
  bool full = a1 - a0 >= 360;
  for (int y = fl(cy - ext); y <= ce(cy + ext); y++)
    for (int x = fl(cx - ext); x <= ce(cx + ext); x++) {
      float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
      float cov = clamp01(thick / 2 - fabsf(sqrtf(dx * dx + dy * dy) - r) + 0.5f);
      if (cov <= 0) continue;
      if (!full) {
        float ang = atan2f(dy, dx) * 180.0f / (float)M_PI;
        float rel = fmodf(ang - a0 + 720.0f, 360.0f);
        if (rel > a1 - a0) continue;
      }
      blendPixel(g, x, y, c, cov);
    }
}

// ---------------------------------------------------------------------------
// Shaded shapes
// ---------------------------------------------------------------------------
struct CloudGeom {
  float x0, x1, yb, h, c1x, c1y, r1, c2x, c2y, r2, top;
};

static CloudGeom cloudGeom(float cx, float cy, float w) {
  CloudGeom k;
  k.yb = cy + w * 0.30f;
  k.h = w * 0.36f;
  k.x0 = cx - w / 2 + k.h / 2;
  k.x1 = cx + w / 2 - k.h / 2;
  k.c1x = cx - w * 0.17f; k.c1y = k.yb - k.h * 0.95f; k.r1 = w * 0.22f;
  k.c2x = cx + w * 0.10f; k.c2y = k.yb - k.h * 1.15f; k.r2 = w * 0.29f;
  k.top = k.c2y - k.r2;
  return k;
}

static float cloudSdf(const CloudGeom& k, float px, float py) {
  float d = segDist(px, py, k.x0, k.yb - k.h / 2, k.x1, k.yb - k.h / 2) - k.h / 2;
  d = fminf(d, hypotf(px - k.c1x, py - k.c1y) - k.r1);
  d = fminf(d, hypotf(px - k.c2x, py - k.c2y) - k.r2);
  return d;
}

static void cloudFill(LovyanGFX& g, float cx, float cy, float w, Color top, Color bot, float alpha = 1.0f,
                      bool gloss = true) {
  CloudGeom k = cloudGeom(cx, cy, w);
  for (int y = fl(k.top - 1); y <= ce(k.yb + 1); y++)
    for (int x = fl(cx - w / 2 - 1); x <= ce(cx + w / 2 + 1); x++) {
      float px = x + 0.5f, py = y + 0.5f;
      float d = cloudSdf(k, px, py);
      float cov = clamp01(0.5f - d) * alpha;
      if (cov <= 0) continue;
      float t = clamp01((py - k.top) / (k.yb - k.top));
      Color c = mix(top, bot, t * t);
      if (gloss) {
        float hl = clamp01(1.0f - hypotf(px - (k.c2x - k.r2 * 0.3f), py - (k.c2y - k.r2 * 0.4f)) / (k.r2 * 0.95f));
        c = mix(c, rgb(255, 255, 255), hl * 0.5f);
        c = mix(c, bot, clamp01(1.0f + d / 1.6f) * 0.35f * t);  // soft rim on the lower edge
      }
      blendPixel(g, x, y, c, cov);
    }
}

void drawGhostCloud(LovyanGFX& g, int cx, int cy, int w, float alpha) {
  cloudFill(g, cx, cy, w, rgb(200, 210, 222), rgb(150, 160, 175), alpha, false);
}

static void sunFill(LovyanGFX& g, float cx, float cy, float r, bool rays) {
  if (rays && r >= 4) {
    for (int i = 0; i < 8; i++) {
      float a = i * (float)M_PI / 4 + (float)M_PI / 8;
      aaLine(g, cx + cosf(a) * r * 1.45f, cy + sinf(a) * r * 1.45f, cx + cosf(a) * r * 1.75f, cy + sinf(a) * r * 1.75f,
             fmaxf(1.0f, r * 0.16f), rgb(255, 200, 70));
    }
  }
  float ext = r * 1.4f + 1;
  for (int y = fl(cy - ext); y <= ce(cy + ext); y++)
    for (int x = fl(cx - ext); x <= ce(cx + ext); x++) {
      float px = x + 0.5f, py = y + 0.5f;
      float d = hypotf(px - cx, py - cy);
      if (d > r + 0.5f) {  // halo
        float a = 1.0f - (d - r) / (r * 0.4f);
        if (a > 0) blendPixel(g, x, y, rgb(255, 190, 60), a * a * 0.35f);
        continue;
      }
      float cov = clamp01(r - d + 0.5f);
      float hd = hypotf(px - (cx - r * 0.3f), py - (cy - r * 0.35f)) / (r * 1.3f);
      Color c = mix(rgb(255, 240, 150), rgb(255, 165, 35), clamp01(hd));
      blendPixel(g, x, y, c, cov);
    }
}

static void moonFill(LovyanGFX& g, float cx, float cy, float r) {
  float ox = cx + r * 0.5f, oy = cy - r * 0.4f, r2 = r * 0.82f;
  for (int y = fl(cy - r - 1); y <= ce(cy + r + 1); y++)
    for (int x = fl(cx - r - 1); x <= ce(cx + r + 1); x++) {
      float px = x + 0.5f, py = y + 0.5f;
      float c1 = clamp01(r - hypotf(px - cx, py - cy) + 0.5f);
      if (c1 <= 0) continue;
      float c2 = clamp01(r2 - hypotf(px - ox, py - oy) + 0.5f);
      Color c = mix(rgb(200, 185, 255), rgb(118, 96, 235), clamp01((py - (cy - r)) / (2 * r)));
      blendPixel(g, x, y, c, c1 * (1.0f - c2));
    }
}

static void boltFill(LovyanGFX& g, float cx, float top, float s) {
  Color c = rgb(255, 196, 40);
  g.fillTriangle((int)(cx + s * 0.07f), (int)top, (int)(cx - s * 0.12f), (int)(top + s * 0.21f),
                 (int)(cx + s * 0.02f), (int)(top + s * 0.21f), c);
  g.fillTriangle((int)(cx - s * 0.04f), (int)(top + s * 0.16f), (int)(cx + s * 0.12f), (int)(top + s * 0.16f),
                 (int)(cx - s * 0.08f), (int)(top + s * 0.42f), c);
}

static void dropsFill(LovyanGFX& g, float cx, float top, float s, int n, float len) {
  float t = fmaxf(1.2f, s * 0.05f);
  for (int i = 0; i < n; i++) {
    float x = cx + (i - (n - 1) / 2.0f) * s * 0.2f;
    float y = top + ((i & 1) ? s * 0.07f : 0);
    aaLine(g, x, y, x - len * 0.3f, y + len, t, rgb(80, 175, 255));
  }
}

static const Color CL_TOP = rgb(255, 255, 255), CL_BOT = rgb(176, 188, 204);
static const Color CB_TOP = rgb(214, 222, 232), CB_BOT = rgb(132, 145, 162);
static const Color CD_TOP = rgb(160, 170, 186), CD_BOT = rgb(78, 88, 104);

static void celestial(LovyanGFX& g, float cx, float cy, float r, bool day, bool rays) {
  if (day) sunFill(g, cx, cy, r, rays);
  else     moonFill(g, cx, cy, r * 1.1f);
}

void drawWeatherIcon(LovyanGFX& g, int icx, int icy, int size, uint8_t code, bool isDay) {
  const float s = size, cx = icx, cy = icy;
  switch (skyFromWmo(code)) {
    case Sky::Clear:
      celestial(g, cx, cy, s * 0.26f, isDay, true);
      break;
    case Sky::PartlyCloudy:
      celestial(g, cx + s * 0.14f, cy - s * 0.14f, s * 0.2f, isDay, true);
      cloudFill(g, cx - s * 0.06f, cy + s * 0.10f, s * 0.78f, CL_TOP, CL_BOT);
      break;
    case Sky::Cloudy:
      cloudFill(g, cx + s * 0.14f, cy - s * 0.12f, s * 0.6f, CB_TOP, CB_BOT);
      cloudFill(g, cx - s * 0.06f, cy + s * 0.08f, s * 0.8f, CL_TOP, CL_BOT);
      break;
    case Sky::Fog:
      cloudFill(g, cx, cy - s * 0.1f, s * 0.76f, CL_TOP, CL_BOT);
      for (int i = 0; i < 3; i++)
        aaLine(g, cx - s * 0.28f + (i & 1) * s * 0.06f, cy + s * (0.24f + i * 0.1f), cx + s * 0.26f, cy + s * (0.24f + i * 0.1f),
               fmaxf(1.2f, s * 0.05f), rgb(170, 182, 198));
      break;
    case Sky::Drizzle:
    case Sky::Rain:
      if (isDay && (code >= 80 || code <= 55)) celestial(g, cx + s * 0.18f, cy - s * 0.2f, s * 0.17f, true, true);
      cloudFill(g, cx, cy - s * 0.08f, s * 0.82f, CL_TOP, CL_BOT);
      dropsFill(g, cx, cy + s * 0.25f, s, 3, s * (skyFromWmo(code) == Sky::Rain ? 0.15f : 0.08f));
      break;
    case Sky::Snow:
      cloudFill(g, cx, cy - s * 0.08f, s * 0.82f, CL_TOP, CL_BOT);
      for (int i = 0; i < 3; i++)
        g.fillSmoothCircle((int)(cx + (i - 1) * s * 0.22f), (int)(cy + s * ((i & 1) ? 0.38f : 0.3f)),
                           (int)fmaxf(1, s * 0.05f), rgb(230, 242, 255));
      break;
    case Sky::Thunder:
      if (isDay) celestial(g, cx + s * 0.18f, cy - s * 0.22f, s * 0.17f, true, true);
      cloudFill(g, cx + s * 0.1f, cy - s * 0.16f, s * 0.55f, CD_TOP, CD_BOT);
      cloudFill(g, cx - s * 0.04f, cy - s * 0.02f, s * 0.8f, CL_TOP, CL_BOT);
      boltFill(g, cx + s * 0.02f, cy + s * 0.16f, s);
      dropsFill(g, cx - s * 0.22f, cy + s * 0.28f, s, 1, s * 0.1f);
      dropsFill(g, cx + s * 0.24f, cy + s * 0.28f, s, 1, s * 0.1f);
      break;
  }
}

// ---------------------------------------------------------------------------
// Outline glyphs
// ---------------------------------------------------------------------------
static const float T = 1.1f;  // stroke width

void glyphLogo(LovyanGFX& g, int cx, int cy, Color c) {
  for (int i = 0; i < 3; i++) {
    float a = i * (float)M_PI / 3 + (float)M_PI / 2;
    float dx = cosf(a) * 6, dy = sinf(a) * 6;
    aaLine(g, cx - dx * 0.35f, cy - dy * 0.35f, cx - dx, cy - dy, 1.6f, c);
    aaLine(g, cx + dx * 0.35f, cy + dy * 0.35f, cx + dx, cy + dy, 1.6f, c);
  }
}

void glyphGrid(LovyanGFX& g, int cx, int cy, Color c) {
  for (int i = 0; i < 4; i++) aaRing(g, cx + ((i & 1) ? 3.2f : -3.2f), cy + ((i & 2) ? 3.2f : -3.2f), 2.3f, T, c);
}

void glyphMap(LovyanGFX& g, int cx, int cy, Color c) {
  const float p[][2] = {{-5.5f, -3.5f}, {-2, -5}, {2, -3.5f}, {5.5f, -5}, {5.5f, 3.5f}, {2, 5}, {-2, 3.5f}, {-5.5f, 5}};
  for (int i = 0; i < 8; i++) {
    const float* a = p[i];
    const float* b = p[(i + 1) % 8];
    if (i == 3 || i == 7) continue;  // the vertical edges are drawn below
    aaLine(g, cx + a[0], cy + a[1], cx + b[0], cy + b[1], T, c);
  }
  aaLine(g, cx + 5.5f, cy - 5, cx + 5.5f, cy + 3.5f, T, c);
  aaLine(g, cx - 5.5f, cy - 3.5f, cx - 5.5f, cy + 5, T, c);
  aaLine(g, cx - 2, cy - 5, cx - 2, cy + 3.5f, T, c);
  aaLine(g, cx + 2, cy - 3.5f, cx + 2, cy + 5, T, c);
}

void glyphPin(LovyanGFX& g, int cx, int cy, Color c) {
  aaArc(g, cx, cy - 1.5f, 4, T, 150, 390, c);
  aaLine(g, cx - 3.5f, cy + 0.5f, cx, cy + 5.5f, T, c);
  aaLine(g, cx + 3.5f, cy + 0.5f, cx, cy + 5.5f, T, c);
  aaRing(g, cx, cy - 1.5f, 1.4f, T, c);
}

void glyphPie(LovyanGFX& g, int cx, int cy, Color c) {
  aaRing(g, cx, cy, 5, T, c);
  for (float r = 0.8f; r < 4.5f; r += 0.9f) aaArc(g, cx, cy, r, 1.2f, 90, 270, c);
}

void glyphCalendar(LovyanGFX& g, int cx, int cy, Color c) {
  aaLine(g, cx - 5, cy - 3.5f, cx + 5, cy - 3.5f, T, c);
  aaLine(g, cx - 5, cy + 5, cx + 5, cy + 5, T, c);
  aaLine(g, cx - 5, cy - 3.5f, cx - 5, cy + 5, T, c);
  aaLine(g, cx + 5, cy - 3.5f, cx + 5, cy + 5, T, c);
  aaLine(g, cx - 5, cy - 0.5f, cx + 5, cy - 0.5f, T, c);
  aaLine(g, cx - 2.5f, cy - 6, cx - 2.5f, cy - 2.5f, T, c);
  aaLine(g, cx + 2.5f, cy - 6, cx + 2.5f, cy - 2.5f, T, c);
}

void glyphGear(LovyanGFX& g, int cx, int cy, Color c) {
  aaRing(g, cx, cy, 2, T, c);
  aaRing(g, cx, cy, 4.3f, T, c);
  for (int i = 0; i < 6; i++) {
    float a = i * (float)M_PI / 3;
    aaLine(g, cx + cosf(a) * 4.6f, cy + sinf(a) * 4.6f, cx + cosf(a) * 6, cy + sinf(a) * 6, 1.8f, c);
  }
}

void glyphBell(LovyanGFX& g, int cx, int cy, Color c) {
  aaArc(g, cx, cy - 1.5f, 3.5f, T, 180, 360, c);
  aaLine(g, cx - 3.5f, cy - 1.5f, cx - 4.2f, cy + 3, T, c);
  aaLine(g, cx + 3.5f, cy - 1.5f, cx + 4.2f, cy + 3, T, c);
  aaLine(g, cx - 5.2f, cy + 3, cx + 5.2f, cy + 3, T, c);
  aaArc(g, cx, cy + 4, 1.5f, T, 0, 180, c);
}

void glyphSearch(LovyanGFX& g, int cx, int cy, Color c) {
  aaRing(g, cx - 1, cy - 1, 4, 1.3f, c);
  aaLine(g, cx + 2, cy + 2, cx + 5, cy + 5, 1.5f, c);
}

void glyphCloud(LovyanGFX& g, int cx, int cy, Color c) {
  CloudGeom k = cloudGeom(cx, cy + 0.5f, 13);
  for (int y = fl(k.top - 2); y <= ce(k.yb + 2); y++)
    for (int x = cx - 9; x <= cx + 9; x++) {
      float d = cloudSdf(k, x + 0.5f, y + 0.5f);
      float cov = clamp01(T / 2 - fabsf(d) + 0.5f);
      if (cov > 0) blendPixel(g, x, y, c, cov);
    }
}

void glyphDrop(LovyanGFX& g, int cx, int cy, Color c) {
  float r = 3.2f, by = cy + 1.8f;
  aaArc(g, cx, by, r, T, -35, 215, c);
  aaLine(g, cx + r * cosf(-35 * (float)M_PI / 180), by + r * sinf(-35 * (float)M_PI / 180), cx, cy - 4.5f, T, c);
  aaLine(g, cx + r * cosf(215 * (float)M_PI / 180), by + r * sinf(215 * (float)M_PI / 180), cx, cy - 4.5f, T, c);
}

void glyphEye(LovyanGFX& g, int cx, int cy, Color c) {
  aaArc(g, cx, cy + 4.5f, 7, T, 220, 320, c);
  aaArc(g, cx, cy - 4.5f, 7, T, 40, 140, c);
  aaRing(g, cx, cy, 1.8f, T, c);
}

void glyphThermo(LovyanGFX& g, int cx, int cy, Color c) {
  aaRing(g, cx, cy + 4, 2.8f, T, c);
  aaLine(g, cx - 1.6f, cy + 1.8f, cx - 1.6f, cy - 4.5f, T, c);
  aaLine(g, cx + 1.6f, cy + 1.8f, cx + 1.6f, cy - 4.5f, T, c);
  aaArc(g, cx, cy - 4.5f, 1.6f, T, 180, 360, c);
  g.fillSmoothCircle(cx, cy + 4, 1, c);
}

void glyphWind(LovyanGFX& g, int cx, int cy, Color c) {
  aaLine(g, cx - 4, cy - 2, cx + 2, cy - 2, T, c);
  aaArc(g, cx + 2, cy - 3.5f, 1.5f, T, 270, 450, c);
  aaLine(g, cx - 4, cy + 1, cx + 4, cy + 1, T, c);
  aaLine(g, cx - 4, cy + 3.5f, cx + 1, cy + 3.5f, T, c);
}

void glyphSunrise(LovyanGFX& g, int cx, int cy, Color c, bool rising) {
  aaArc(g, cx, cy + 2, 3.2f, T, 180, 360, c);
  aaLine(g, cx - 5.5f, cy + 2, cx + 5.5f, cy + 2, T, c);
  float ay = cy - 3.5f, dir = rising ? -1.0f : 1.0f;
  aaLine(g, cx - 2, ay - dir * 0.5f, cx, ay + dir * 1.5f, T, c);
  aaLine(g, cx + 2, ay - dir * 0.5f, cx, ay + dir * 1.5f, T, c);
  aaLine(g, cx - 5, cy - 1, cx - 4, cy, T, c);
  aaLine(g, cx + 5, cy - 1, cx + 4, cy, T, c);
}

void glyphChevron(LovyanGFX& g, int cx, int cy, Color c) {
  aaLine(g, cx - 2.5f, cy - 1.2f, cx, cy + 1.3f, T, c);
  aaLine(g, cx + 2.5f, cy - 1.2f, cx, cy + 1.3f, T, c);
}

void glyphPlus(LovyanGFX& g, int cx, int cy, Color c) {
  aaLine(g, cx - 4, cy, cx + 4, cy, 1.4f, c);
  aaLine(g, cx, cy - 4, cx, cy + 4, 1.4f, c);
}

void glyphMinus(LovyanGFX& g, int cx, int cy, Color c) { aaLine(g, cx - 4, cy, cx + 4, cy, 1.4f, c); }

void glyphLocate(LovyanGFX& g, int cx, int cy, Color c) {
  aaLine(g, cx, cy - 5, cx + 5, cy, T, c);
  aaLine(g, cx + 5, cy, cx, cy + 5, T, c);
  aaLine(g, cx, cy + 5, cx - 5, cy, T, c);
  aaLine(g, cx - 5, cy, cx, cy - 5, T, c);
  aaRing(g, cx, cy, 1.6f, T, c);
}

void glyphList(LovyanGFX& g, int cx, int cy, Color c) {
  for (int i = -1; i <= 1; i++) {
    g.fillSmoothCircle(cx - 4, cy + i * 3, 1, c);
    aaLine(g, cx - 1.5f, cy + i * 3, cx + 5, cy + i * 3, T, c);
  }
}

void glyphLayers(LovyanGFX& g, int cx, int cy, Color c) {
  for (int k = 0; k < 2; k++) {
    float y = cy - 1.5f + k * 3.5f;
    aaLine(g, cx - 5.5f, y, cx, y - 3, T, c);
    aaLine(g, cx, y - 3, cx + 5.5f, y, T, c);
    aaLine(g, cx + 5.5f, y, cx, y + 3, T, c);
    aaLine(g, cx, y + 3, cx - 5.5f, y, T, c);
  }
}

void drawAvatar(LovyanGFX& g, int cx, int cy, int r) {
  for (int y = cy - r - 1; y <= cy + r + 1; y++)
    for (int x = cx - r - 1; x <= cx + r + 1; x++) {
      float px = x + 0.5f - cx, py = y + 0.5f - cy;
      float cov = clamp01(r - hypotf(px, py) + 0.5f);
      if (cov <= 0) continue;
      Color c = mix(rgb(222, 202, 176), rgb(150, 118, 92), clamp01((py + r) / (2 * r)));
      // body
      float bx = px / (r * 0.78f), by = (py - r * 0.95f) / (r * 0.62f);
      float body = clamp01((1.0f - sqrtf(bx * bx + by * by)) * r * 0.6f + 0.5f);
      c = mix(c, rgb(122, 40, 38), body);
      // head + hair
      float hd = hypotf(px, py + r * 0.12f) - r * 0.36f;
      float head = clamp01(0.5f - hd);
      Color skin = py < -r * 0.36f ? rgb(92, 60, 42) : rgb(240, 200, 166);
      c = mix(c, skin, head);
      blendPixel(g, x, y, c, cov);
    }
}
