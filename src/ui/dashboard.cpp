// Dashboard layout, a 1:1 adaptation of the "Weathry" weather dashboard design for 480x320.
#include "dashboard.h"

#include <stdio.h>
#include <string.h>

#include "fonts.h"
#include "icons.h"

using D = lgfx::textdatum_t;

// ---------------------------------------------------------------------------
// Fonts
// ---------------------------------------------------------------------------
enum FontId { R8, R9, R10, M10, M11, R13, M16, L28, FONT_COUNT };

namespace {
struct LoadedFont {
  lgfx::PointerWrapper data;
  lgfx::VLWfont        font;
};
LoadedFont gFonts[FONT_COUNT];
bool       gFontsReady = false;
}  // namespace

void uiInit() {
  if (gFontsReady) return;
  const uint8_t* src[FONT_COUNT] = {font_r8_vlw, font_r9_vlw, font_r10_vlw, font_m10_vlw,
                                    font_m11_vlw, font_r13_vlw, font_m16_vlw, font_l28_vlw};
  const uint32_t len[FONT_COUNT] = {sizeof(font_r8_vlw),  sizeof(font_r9_vlw),  sizeof(font_r10_vlw),
                                    sizeof(font_m10_vlw), sizeof(font_m11_vlw), sizeof(font_r13_vlw),
                                    sizeof(font_m16_vlw), sizeof(font_l28_vlw)};
  for (int i = 0; i < FONT_COUNT; i++) {
    gFonts[i].data.set(src[i], len[i]);
    gFonts[i].font.loadFont(&gFonts[i].data);
  }
  gFontsReady = true;
}

static int text(LovyanGFX& g, FontId f, Color c, int x, int y, const char* s, D datum = D::top_left) {
  g.setFont(&gFonts[f].font);
  g.setTextColor(c);
  g.setTextDatum(datum);
  return g.drawString(s, x, y);
}

static int textWidth(LovyanGFX& g, FontId f, const char* s) {
  g.setFont(&gFonts[f].font);
  return g.textWidth(s);
}

// Largest of the given fonts that fits in maxW.
static FontId fit(LovyanGFX& g, const char* s, int maxW, FontId a, FontId b, FontId c = R8) {
  if (textWidth(g, a, s) <= maxW) return a;
  if (textWidth(g, b, s) <= maxW) return b;
  return c;
}

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------
static const char* const DOW_LONG[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char* const DOW[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* const MON_LONG[] = {"January", "February", "March", "April", "May", "June", "July",
                                       "August", "September", "October", "November", "December"};
static const char* const MON[] = {"jan", "feb", "mar", "apr", "may", "june", "july", "aug", "sep", "oct", "nov", "dec"};
#define DEG "\xC2\xB0"

static struct tm localTm(time_t utc, int32_t offset) {
  time_t t = utc + offset;
  struct tm tm;
  gmtime_r(&t, &tm);
  return tm;
}

static int roundi(float v) {
  int r = (int)lroundf(v);
  return r == 0 ? 0 : r;
}

static void fmtClock(char* b, size_t n, time_t utc, int32_t off, bool h24) {
  struct tm tm = localTm(utc, off);
  if (h24) {
    snprintf(b, n, "%02d:%02d", tm.tm_hour, tm.tm_min);
  } else {
    int h = tm.tm_hour % 12;
    snprintf(b, n, "%d:%02d %s", h == 0 ? 12 : h, tm.tm_min, tm.tm_hour < 12 ? "AM" : "PM");
  }
}

static void fmtSigned(char* b, size_t n, float t, const char* suffix) {
  int v = roundi(t);
  snprintf(b, n, "%s%d%s", v > 0 ? "+" : "", v, suffix);
}

// "Paris, FR" -> "Paris"
static void shortName(char* b, size_t n, const char* full) {
  snprintf(b, n, "%s", full);
  char* comma = strchr(b, ',');
  if (comma) *comma = 0;
}

// ---------------------------------------------------------------------------
// Palette (sampled from the design)
// ---------------------------------------------------------------------------
namespace ds {
constexpr Color screen   = rgb(13, 15, 18);
constexpr Color white    = rgb(255, 255, 255);
constexpr Color text2    = rgb(196, 202, 210);
constexpr Color dim      = rgb(128, 136, 148);
constexpr Color faint    = rgb(84, 92, 104);
constexpr Color line     = rgb(48, 54, 62);
constexpr Color pill     = rgb(27, 31, 37);
constexpr Color blue     = rgb(70, 150, 255);
constexpr Color gold     = rgb(242, 193, 78);
constexpr Color teal     = rgb(42, 98, 128);
constexpr Color red      = rgb(255, 92, 92);
constexpr Color subTop   = rgb(30, 34, 41);
constexpr Color subBot   = rgb(22, 25, 31);
}  // namespace ds

// ---------------------------------------------------------------------------
// Rounded shapes
// ---------------------------------------------------------------------------
struct Glow {
  float x, y, r;
  Color c;
  float a;
};

static inline float edgeCoverage(int px, float dy, int r) {
  float dx = r - (px + 0.5f);
  float c = r - sqrtf(dx * dx + dy * dy) + 0.5f;
  return c < 0 ? 0 : (c > 1 ? 1 : c);
}

// Fills a rounded rectangle with colours from colorAt(x, y), anti-aliased corners, dithered.
template <class F>
static void paintRounded(LovyanGFX& g, int x, int y, int w, int h, int r, F&& colorAt) {
  static lgfx::rgb888_t row[480];
  if (w > 480) w = 480;
  int j0 = y < 0 ? -y : 0;
  int j1 = (y + h > g.height()) ? g.height() - y : h;
  for (int j = j0; j < j1; j++) {
    float py = j + 0.5f;
    float dy = py < r ? r - py : (py > h - r ? py - (h - r) : 0);
    int solid = 0;
    if (dy > 0) {
      solid = r;
      for (int px = 0; px < r; px++)
        if (edgeCoverage(px, dy, r) >= 1) { solid = px; break; }
    }
    const int yy = y + j;
    int n = 0;
    for (int i = solid; i < w - solid; i++) row[n++] = lgfx::rgb888_t(dither(colorAt(x + i, yy), x + i, yy));
    for (int px = 0; px < solid; px++) {
      float cov = edgeCoverage(px, dy, r);
      blendPixel(g, x + px, yy, colorAt(x + px, yy), cov);
      blendPixel(g, x + w - 1 - px, yy, colorAt(x + w - 1 - px, yy), cov);
    }
    if (n > 0) g.pushImage(x + solid, yy, n, 1, row);
  }
}

static Color glowed(Color base, const Glow* glow, int x, int y) {
  if (!glow) return base;
  float gx = x + 0.5f - glow->x, gy = y + 0.5f - glow->y;
  float d = sqrtf(gx * gx + gy * gy);
  if (d >= glow->r) return base;
  float k = 1.0f - d / glow->r;
  return mix(base, glow->c, glow->a * k * k);
}

static void card(LovyanGFX& g, int x, int y, int w, int h, int r, Color top, Color bot, const Glow* glow = nullptr) {
  paintRounded(g, x, y, w, h, r, [&](int px, int py) {
    return glowed(mix(top, bot, h > 1 ? (float)(py - y) / (h - 1) : 0), glow, px, py);
  });
  if (y >= 0 && y < g.height()) {  // faint top highlight, like light catching the glass edge
    for (int i = r; i < w - r; i++) blendPixel(g, x + i, y, ds::white, 0.06f);
  }
}

static void glass(LovyanGFX& g, int x, int y, int w, int h, int r, Color c, float a) {
  paintRounded(g, x, y, w, h, r, [&](int px, int py) { return mix(readRGB(g, px, py), c, a); });
}

static bool visible(LovyanGFX& g, int y, int h) { return y + h > 0 && y < g.height(); }

static void pillButton(LovyanGFX& g, int right, int y, const char* label) {
  int w = textWidth(g, R8, label) + 20, x = right - w;
  card(g, x, y, w, 15, 7, ds::pill, ds::pill);
  text(g, R8, ds::text2, x + 6, y + 8, label, D::middle_left);
  glyphChevron(g, x + w - 7, y + 8, ds::text2);
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------
static void drawSidebar(LovyanGFX& g, int oy) {
  const int X = 8, Y = 9 - oy, W = 34, H = 302, cx = X + W / 2;
  if (!visible(g, Y, H)) return;
  Glow glow{(float)cx, (float)(Y + H), 110.0f, ds::teal, 0.35f};
  card(g, X, Y, W, H, 10, rgb(32, 36, 42), rgb(22, 26, 31), &glow);

  glyphLogo(g, cx, Y + 16, ds::blue);
  text(g, R8, ds::white, cx, Y + 31, "Weathry", D::middle_center);
  g.drawFastHLine(X + 5, Y + 41, W - 10, ds::line);

  void (*nav[])(LovyanGFX&, int, int, Color) = {glyphGrid, glyphMap, glyphPin, glyphPie, glyphCalendar, glyphGear};
  for (int i = 0; i < 6; i++) {
    int iy = Y + 60 + i * 28;
    if (i == 0) {
      paintRounded(g, X + 4, iy - 11, W - 8, 22, 6, [&](int px, int py) { return mix(readRGB(g, px, py), ds::white, 0.05f); });
      g.fillRect(X, iy - 9, 2, 18, ds::white);
    }
    nav[i](g, cx, iy, i == 0 ? ds::white : ds::dim);
  }

  g.fillSmoothCircle(cx, Y + 252, 11, rgb(12, 14, 17));
  glyphBell(g, cx, Y + 251, ds::white);
  g.fillSmoothCircle(cx + 5, Y + 246, 2, ds::red);
  drawAvatar(g, cx, Y + 283, 13);
}

// ---------------------------------------------------------------------------
// Current weather card
// ---------------------------------------------------------------------------
static void drawNow(LovyanGFX& g, int oy, const WeatherData& w, const UiState& st) {
  const int X = 48, Y = 9 - oy, W = 135, H = 141;
  if (!visible(g, Y, H)) return;
  Glow glow{X + W * 0.72f, Y + H * 0.62f, 125.0f, ds::teal, 0.6f};
  card(g, X, Y, W, H, 12, rgb(36, 40, 47), rgb(26, 36, 46), &glow);

  // soft background clouds (clipped to the card)
  g.setClipRect(X + 2, Y + 2, W - 4, H - 4);
  drawGhostCloud(g, X + 26, Y + 20, 26, 0.12f);
  drawGhostCloud(g, X + W - 2, Y + 58, 24, 0.10f);
  g.clearClipRect();

  drawWeatherIcon(g, X + 30, Y + 42, 48, w.code, w.isDay);

  g.fillSmoothCircle(X + W - 19, Y + 19, 12, rgb(24, 28, 34));
  glyphSearch(g, X + W - 19, Y + 19, ds::white);

  char buf[40];
  snprintf(buf, sizeof buf, "%d" DEG "%s", roundi(w.temp), st.metric ? "c" : "f");
  text(g, L28, ds::white, X + 9, Y + 90, buf, D::baseline_left);

  glyphCloud(g, X + 15, Y + 104, ds::white);
  const char* desc = wmoDescription(w.code, w.isDay);
  text(g, fit(g, desc, W - 36, R10, R9), ds::white, X + 26, Y + 104, desc, D::middle_left);

  g.drawFastHLine(X + 9, Y + 115, W - 18, rgb(58, 66, 76));

  glyphPin(g, X + 15, Y + 125, ds::text2);
  text(g, R9, ds::text2, X + 25, Y + 125, st.location, D::middle_left);

  glyphCalendar(g, X + 15, Y + 135, ds::text2);
  struct tm tm = localTm(st.now, w.utcOffset);
  char clock[16];
  fmtClock(clock, sizeof clock, st.now, w.utcOffset, st.clock24);
  int cw = textWidth(g, M10, clock);
  snprintf(buf, sizeof buf, "%d %s, %d", tm.tm_mday, MON_LONG[tm.tm_mon], tm.tm_year + 1900);
  if (textWidth(g, R9, buf) + cw + 4 > W - 34) snprintf(buf, sizeof buf, "%d %.3s, %d", tm.tm_mday, MON_LONG[tm.tm_mon], tm.tm_year + 1900);
  if (textWidth(g, R9, buf) + cw + 4 > W - 34) snprintf(buf, sizeof buf, "%d %.3s", tm.tm_mday, MON_LONG[tm.tm_mon]);
  int dw = text(g, R9, ds::text2, X + 25, Y + 135, buf, D::middle_left);
  text(g, M10, ds::white, X + 25 + dw + 4, Y + 135, clock, D::middle_left);
}

// ---------------------------------------------------------------------------
// Today's Highlight
// ---------------------------------------------------------------------------
static float spline(const float* v, int n, float fi) {
  if (n == 1 || fi <= 0) return v[0];
  if (fi >= n - 1) return v[n - 1];
  int i = (int)fi;
  float t = fi - i;
  float p0 = v[i > 0 ? i - 1 : 0], p1 = v[i], p2 = v[i + 1], p3 = v[i + 2 < n ? i + 2 : n - 1];
  float t2 = t * t, t3 = t2 * t;
  return 0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3);
}

static void subCard(LovyanGFX& g, int x, int y, int w, int h, const char* label, const Glow* glow = nullptr) {
  card(g, x, y, w, h, 9, ds::subTop, ds::subBot, glow);
  text(g, fit(g, label, w - 12, R9, R8), ds::text2, x + 7, y + 10, label, D::middle_left);
}

static void drawWind(LovyanGFX& g, int x, int y, int w, int h, const WeatherData& wd, const UiState& st) {
  subCard(g, x, y, w, h, "Wind Status");
  const int n = wd.hourlyCount < 14 ? wd.hourlyCount : 14;
  if (n > 1) {
    float v[14], vmax = 0.1f, vmin = 1e9f;
    for (int i = 0; i < n; i++) {
      v[i] = wd.hourly[i].wind;
      vmax = fmaxf(vmax, v[i]);
      vmin = fminf(vmin, v[i]);
    }
    float span = fmaxf(vmax - vmin, 1.0f);
    // wave
    const float wx0 = x + 6, wx1 = x + w - 6, step = (wx1 - wx0) / (n - 1);
    float px = wx0, py = y + 34 - (spline(v, n, 0) - vmin) / span * 11;
    for (float sx = wx0 + 1.5f; sx <= wx1; sx += 1.5f) {
      float sy = y + 34 - (spline(v, n, (sx - wx0) / step) - vmin) / span * 11;
      aaLine(g, px, py, sx, sy, 1.0f, rgb(190, 198, 208));
      px = sx;
      py = sy;
    }
    // bars
    for (int i = 0; i < n; i++) {
      float k = (v[i] - vmin) / span;
      int bh = 3 + (int)(k * 8);
      int bx = (int)(wx0 + i * step);
      g.fillRect(bx, y + 50 - bh, 2, bh, k > 0.7f ? ds::blue : rgb(70, 78, 90));
    }
  }
  // "7.90 km/h ... 5:01 AM" (the clock is shown only when it fits beside the value)
  const char* unit = st.metric ? "km/h" : "mph";
  char clock[16], buf[16];
  fmtClock(clock, sizeof clock, st.now, wd.utcOffset, st.clock24);
  snprintf(buf, sizeof buf, wd.windSpeed < 100 ? "%.2f" : "%.0f", wd.windSpeed);
  bool showClock = textWidth(g, M16, buf) + 1 + textWidth(g, R8, unit) + 6 + textWidth(g, R8, clock) <= w - 14;
  int vw = text(g, M16, ds::white, x + 7, y + h - 8, buf, D::baseline_left);
  text(g, R8, ds::dim, x + 8 + vw, y + h - 8, unit, D::baseline_left);
  if (showClock) text(g, R8, ds::dim, x + w - 7, y + h - 8, clock, D::baseline_right);
}

static void drawUv(LovyanGFX& g, int x, int y, int w, int h, const WeatherData& wd) {
  Glow glow{x + w / 2.0f, (float)(y + h), 60.0f, ds::teal, 0.35f};
  subCard(g, x, y, w, h, "UV Index", &glow);
  const float cx = x + w / 2.0f, cy = y + 50;
  const float rOut = 29, rIn = 21;
  const float uvMaxScale = 12;
  float frac = fminf(fmaxf(wd.uvIndex / uvMaxScale, 0), 1);
  float end = 180 + 180 * frac;

  // thin track for the rest of the scale
  aaArc(g, cx, cy, rOut - 1, 1.3f, end, 360, rgb(150, 158, 168));
  // thick blue band for the current value
  for (float a = 180; a < end; a += 2) {
    float b = fminf(a + 2.5f, end);
    Color c = mix(rgb(30, 80, 170), rgb(110, 190, 255), (a - 180) / fmaxf(end - 180, 1));
    aaArc(g, cx, cy, (rOut + rIn) / 2, rOut - rIn, a, b, c);
  }
  if (frac > 0) aaArc(g, cx, cy, rOut - 1, 1.2f, 180, end, rgb(170, 220, 255));  // highlight edge
  // scale numbers
  for (int v = 0; v <= 12; v += 3) {
    float a = (180 + 180 * v / uvMaxScale) * (float)M_PI / 180;
    char n[4];
    snprintf(n, sizeof n, "%d", v);
    text(g, R8, ds::faint, (int)(cx + cosf(a) * 15), (int)(cy + sinf(a) * 15) - 1, n, D::middle_center);
  }

  char buf[16];
  snprintf(buf, sizeof buf, "%.2f", wd.uvIndex);
  int vw = textWidth(g, M16, buf), uw = textWidth(g, R8, "uv");
  int vx = (int)cx - (vw + uw + 2) / 2;
  text(g, M16, ds::white, vx, y + h - 6, buf, D::baseline_left);
  text(g, R8, ds::dim, vx + vw + 2, y + h - 6, "uv", D::baseline_left);
}

static void drawSun(LovyanGFX& g, int x, int y, int w, int h, const WeatherData& wd, const UiState& st) {
  Glow glow{x + w / 2.0f, (float)(y + 44), 50.0f, rgb(120, 96, 40), 0.30f};
  subCard(g, x, y, w, h, "Sunrise & Sunset", &glow);
  const float cx = x + w / 2.0f, cy = y + 44, r = 24;

  bool up = wd.sunset > wd.sunrise && st.now >= wd.sunrise && st.now <= wd.sunset;
  float frac = up ? (float)(st.now - wd.sunrise) / (float)(wd.sunset - wd.sunrise) : (st.now > wd.sunset ? 1.0f : 0.0f);
  float sunA = 180 + 180 * frac;

  // horizon with gold end marks
  g.drawFastHLine(x + 6, (int)cy, w - 12, ds::line);
  g.fillRect(x + 6, (int)cy - 1, 7, 2, ds::gold);
  g.fillRect(x + w - 13, (int)cy - 1, 7, 2, ds::gold);

  // dashed arc: bright where the sun has been, dim ahead
  for (float a = 180; a < 360; a += 10) {
    Color c = a < sunA ? ds::gold : mix(ds::subBot, ds::gold, 0.45f);
    aaArc(g, cx, cy, r, 1.2f, a, a + 5, c);
  }
  if (up) {
    float rad = sunA * (float)M_PI / 180;
    float sx = cx + cosf(rad) * r, sy = cy + sinf(rad) * r;
    aaLine(g, sx, sy, sx, cy, 1.0f, mix(ds::subBot, ds::gold, 0.35f));
    for (int k = 9; k > 4; k--) g.fillSmoothCircle((int)sx, (int)sy, k, mix(ds::subTop, ds::gold, 0.06f * (10 - k)));
    g.fillSmoothCircle((int)sx, (int)sy, 4, ds::gold);
  }

  char t1[16], t2[16];
  fmtClock(t1, sizeof t1, wd.sunrise, wd.utcOffset, st.clock24);
  fmtClock(t2, sizeof t2, wd.sunset, wd.utcOffset, st.clock24);
  FontId tf = textWidth(g, M10, t1) + textWidth(g, M10, t2) + 10 <= w - 14 ? M10
            : textWidth(g, R9, t1) + textWidth(g, R9, t2) + 8 <= w - 14 ? R9 : R8;
  glyphSunrise(g, x + 12, y + 51, ds::faint, true);
  glyphSunrise(g, x + w - 12, y + 51, ds::faint, false);
  text(g, R8, ds::gold, x + 7, y + 59, "Sunrise", D::middle_left);
  text(g, R8, ds::gold, x + w - 7, y + 59, "Sunset", D::middle_right);
  text(g, tf, ds::white, x + 7, y + 67, t1, D::middle_left);
  text(g, tf, ds::white, x + w - 7, y + 67, t2, D::middle_right);
}

// Small card: label, big value with unit, icon + two-line note on the right.
static void drawStat(LovyanGFX& g, int x, int y, int w, int h, const char* label, const char* value, const char* unit,
                     void (*glyph)(LovyanGFX&, int, int, Color), bool glyphLeft, const char* note1, const char* note2) {
  subCard(g, x, y, w, h, label);
  int vx = x + 7;
  if (glyphLeft) {
    glyph(g, x + 12, y + h - 13, ds::white);
    vx = x + 20;
  }
  int vw = text(g, M16, ds::white, vx, y + h - 7, value, D::baseline_left);
  int uw = unit && *unit ? text(g, R8, ds::dim, vx + vw + 1, y + h - 7, unit, D::baseline_left) + 1 : 0;
  int nx = vx + vw + uw + 5;
  int nw = textWidth(g, R8, note1) > textWidth(g, R8, note2) ? textWidth(g, R8, note1) : textWidth(g, R8, note2);
  if (nx < x + w - 6 - nw) nx = x + w - 6 - nw;  // right-align the note block when there is room
  if (nx + nw > x + w - 3) return;                // no room for a note at all
  int labelEnd = x + 7 + textWidth(g, R9, label);
  if (!glyphLeft && nx + 4 > labelEnd + 6) glyph(g, nx + 4, y + 10, ds::dim);
  text(g, R8, ds::dim, nx, y + h - 15, note1, D::middle_left);
  text(g, R8, ds::dim, nx, y + h - 7, note2, D::middle_left);
}

static void drawHighlights(LovyanGFX& g, int oy, const WeatherData& w, const UiState& st) {
  const int X = 192, Y = 9 - oy, W = 280, H = 141;
  if (!visible(g, Y, H)) return;
  Glow glow{X + W * 0.45f, (float)(Y + H), 170.0f, ds::teal, 0.35f};
  card(g, X, Y, W, H, 12, rgb(32, 36, 42), rgb(24, 28, 33), &glow);
  text(g, M10, ds::white, X + 9, Y + 10, "Today's Highlight", D::middle_left);

  const int sx[3] = {X + 7, X + 98, X + 189}, cw = 84;
  const int r1 = Y + 20, h1 = 73, r2 = Y + 99, h2 = 35;
  if (visible(g, r1, h1)) {
    drawWind(g, sx[0], r1, cw, h1, w, st);
    drawUv(g, sx[1], r1, cw, h1, w);
    drawSun(g, sx[2], r1, cw, h1, w, st);
  }
  if (!visible(g, r2, h2)) return;

  char v[16], n2[16];
  snprintf(v, sizeof v, "%d", roundi(w.humidity));
  snprintf(n2, sizeof n2, "is %d" DEG " now", roundi(w.dewPoint));
  drawStat(g, sx[0], r2, cw, h2, "Humidity", v, "%", glyphDrop, false, "Dew point", n2);

  float vis = st.metric ? w.visibility / 1000.0f : w.visibility / 1609.34f;
  if (vis < 10) snprintf(v, sizeof v, "%02d", (int)vis);  // "03" like the design
  else          snprintf(v, sizeof v, "%d", roundi(vis));
  const char *a = "Clear", *b = "view";
  if (w.visibility < 1000) a = "Thick", b = "fog";
  else if (w.visibility < 4000) a = "Mist is", b = "limiting";
  else if (w.visibility < 10000) a = "Haze is", b = "affecting";
  drawStat(g, sx[1], r2, cw, h2, "Visibility", v, st.metric ? "km" : "mi", glyphEye, false, a, b);

  snprintf(v, sizeof v, "%d" DEG, roundi(w.feelsLike));
  float diff = w.feelsLike - w.temp;
  const char *f1 = "Same as", *f2 = "actual";
  if (diff >= 2) f1 = "Feels", f2 = "hotter";
  else if (diff <= -2) f1 = "Feels", f2 = "colder";
  drawStat(g, sx[2], r2, cw, h2, "Feels Like", v, "", glyphThermo, true, f1, f2);
}

// ---------------------------------------------------------------------------
// 7 days Forecast
// ---------------------------------------------------------------------------
static void drawForecast(LovyanGFX& g, int oy, const WeatherData& w) {
  const int TY = 166 - oy;
  if (visible(g, TY - 9, 18)) {
    text(g, M11, ds::white, 49, TY, "7 days Forecast", D::middle_left);
    pillButton(g, 183, TY - 8, "7 day");
  }

  const int X = 48, Y = 179 - oy, W = 135, H = 132;
  if (!visible(g, Y, H)) return;
  Glow glow{X + W * 0.85f, (float)(Y + 10), 110.0f, rgb(60, 96, 128), 0.30f};
  auto cardColor = [&](int px, int py) {
    return glowed(mix(rgb(34, 38, 45), rgb(22, 25, 30), (float)(py - Y) / (H - 1)), &glow, px, py);
  };
  paintRounded(g, X, Y, W, H, 12, cardColor);

  char hi[12], lo[12], date[16];
  const int rows = w.dailyCount < 4 ? w.dailyCount : 4;
  for (int i = 0; i < rows; i++) {
    const DailyPoint& d = w.daily[i];
    int yc = Y + 15 + i * 20;
    struct tm tm = localTm(d.time, w.utcOffset);
    drawWeatherIcon(g, X + 15, yc, 20, d.code, true);
    fmtSigned(hi, sizeof hi, d.tMax, DEG);
    snprintf(lo, sizeof lo, "/");
    fmtSigned(lo + 1, sizeof lo - 1, d.tMin, "");
    int hw = text(g, R13, ds::white, X + 28, yc + 5, hi, D::baseline_left);
    int lw = text(g, R9, ds::dim, X + 28 + hw, yc + 5, lo, D::baseline_left);
    snprintf(date, sizeof date, "%d %s", tm.tm_mday, MON[tm.tm_mon]);
    int dx = X + 28 + hw + lw + 6;
    text(g, R8, ds::dim, dx, yc + 1, date, D::middle_left);
    int dateEnd = dx + textWidth(g, R8, date) + 5;
    const char* day = DOW_LONG[tm.tm_wday];
    if (X + W - 8 - textWidth(g, R8, day) < dateEnd) day = DOW[tm.tm_wday];
    text(g, R8, ds::text2, X + W - 8, yc + 1, day, D::middle_right);
  }
  // the list fades out under the "Tomorrow" card, as in the design
  if (rows == 4) {
    int y0 = Y + 66, y1 = Y + 86;
    for (int yy = y0; yy < y1; yy++) {
      if (yy < 0 || yy >= g.height()) continue;
      float a = 0.15f + 0.65f * (yy - y0) / (float)(y1 - y0);
      for (int xx = X + 4; xx < X + W - 4; xx++) g.drawPixel(xx, yy, dither(mix(readRGB(g, xx, yy), cardColor(xx, yy), a), xx, yy));
    }
  }

  // Tomorrow card
  if (w.dailyCount < 2) return;
  const DailyPoint& t = w.daily[1];
  const int tx = X + 6, ty = Y + 86, tw = W - 12, th = 40;
  card(g, tx, ty, tw, th, 9, rgb(44, 49, 57), rgb(34, 38, 45));
  drawWeatherIcon(g, tx + 17, ty + 21, 30, t.code, true);
  text(g, R8, ds::dim, tx + 35, ty + 9, "Tomorrow", D::middle_left);
  fmtSigned(hi, sizeof hi, t.tMax, DEG);
  text(g, M16, ds::white, tx + 35, ty + 27, hi[0] == '+' ? hi + 1 : hi, D::baseline_left);
  static const char* const SHORT[] = {"Sunny day", "Partly cloudy", "Cloudy day", "Foggy day", "Drizzle day",
                                      "Rainy day", "Snowy day", "Storm day"};
  const char* full = wmoDescription(t.code, true);
  char desc[32];
  snprintf(desc, sizeof desc, "%s", textWidth(g, R8, full) <= 50 ? full : SHORT[(int)skyFromWmo(t.code)]);
  text(g, R8, ds::text2, tx + 35, ty + 33, desc, D::middle_left);

  // tomorrow's temperature through the day (6 samples)
  float pts[6];
  int np = 0;
  for (int k = 0; k < 6; k++) {
    time_t want = t.time + (2 + k * 4) * 3600;
    for (int i = 0; i < w.hourlyCount; i++)
      if (w.hourly[i].time == want) { pts[np++] = w.hourly[i].temp; break; }
  }
  if (np >= 2) {
    float lo2 = pts[0], hi2 = pts[0];
    for (int i = 1; i < np; i++) lo2 = fminf(lo2, pts[i]), hi2 = fmaxf(hi2, pts[i]);
    float span = fmaxf(hi2 - lo2, 1);
    int textEnd = tx + 35 + (textWidth(g, R8, desc) > textWidth(g, R8, "Tomorrow") ? textWidth(g, R8, desc) : textWidth(g, R8, "Tomorrow"));
    const float cx0 = fmaxf(textEnd + 8.0f, tx + 74.0f), cx1 = tx + tw - 7, cyT = ty + 13, cyB = ty + 29;
    float stepx = (cx1 - cx0) / (np - 1);
    auto py = [&](int i) { return cyB - (pts[i] - lo2) / span * (cyB - cyT); };
    for (int i = 0; i + 1 < np; i++)
      aaLine(g, cx0 + i * stepx, py(i), cx0 + (i + 1) * stepx, py(i + 1), 1.1f,
             pts[i + 1] > pts[i] ? rgb(255, 120, 120) : rgb(160, 200, 230));
    for (int i = 1; i + 1 < np; i += 2) {
      g.fillSmoothCircle((int)(cx0 + i * stepx), (int)py(i), 3, ds::white);
      g.fillSmoothCircle((int)(cx0 + i * stepx), (int)py(i), 1, rgb(40, 45, 52));
    }
  }
}

// ---------------------------------------------------------------------------
// Weather condition map
// ---------------------------------------------------------------------------
struct Seg {
  float x0, y0, x1, y1, hw;
};
// Stylised street map in card-local coordinates (280 x 132).
static const Seg ROADS[] = {
    {-10, 40, 70, -10, 3.5f},  {-10, 95, 115, -10, 4.0f}, {10, 140, 150, -10, 3.0f},  {0, 60, 60, 132, 2.5f},
    {120, 22, 300, 4, 3.0f},   {160, -5, 160, 30, 2.5f},  {200, -5, 205, 22, 2.5f},   {240, 0, 245, 22, 2.5f},
    {232, 52, 300, 34, 3.0f},  {238, 80, 300, 70, 2.5f},  {226, 110, 300, 92, 3.0f},  {190, 132, 236, 98, 2.5f},
    {130, 132, 150, 112, 2.5f}, {255, 132, 270, 112, 2.0f},
};

static float segDistance(float px, float py, const Seg& s) {
  float dx = s.x1 - s.x0, dy = s.y1 - s.y0;
  float t = ((px - s.x0) * dx + (py - s.y0) * dy) / (dx * dx + dy * dy);
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  float ex = px - (s.x0 + t * dx), ey = py - (s.y0 + t * dy);
  return sqrtf(ex * ex + ey * ey);
}

static Color mapColor(float lx, float ly, float wet) {
  const Color block = rgb(19, 22, 26), road = rgb(40, 45, 52);
  Color c = block;
  for (const Seg& s : ROADS) {
    float cov = s.hw - segDistance(lx, ly, s) + 0.5f;
    if (cov > 0) c = mix(c, road, cov > 1 ? 1 : cov);
  }
  // big ring road around the main city
  float dr = hypotf(lx - 176, ly - 70);
  float ring = 6.5f - fabsf(dr - 66) + 0.5f;
  if (ring > 0) c = mix(c, road, ring > 1 ? 1 : ring);

  // lighter area (the region the forecast covers)
  float ex = (lx - 112) / 125, ey = (ly - 78) / 82;
  float lit = 1 - (ex * ex + ey * ey);
  if (lit > 0) c = mix(c, rgb(46, 60, 74), fminf(lit * 1.6f, 0.75f));

  // dark coastline around the main blob
  float coast = 1 - fabsf(dr - 42) / 9;
  if (coast > 0) c = mix(c, rgb(14, 18, 24), fminf(coast * 1.5f, 0.85f));

  // precipitation glow
  if (dr < 70) {
    float k = 1 - dr / 70;
    c = mix(c, rgb(40, 100, 132), k * k * 0.6f);
  }
  float lobe = hypotf((lx - 172) / 1.3f, (ly - 72) * 1.05f);
  if (lobe < 30) {
    float k = 1 - lobe / 30;
    c = mix(c, mix(rgb(44, 128, 166), rgb(96, 196, 226), k), fminf(k * 2.0f, 1.0f) * (0.55f + 0.35f * wet));
  }
  float d2 = hypotf(lx - 86, ly - 62);
  if (d2 < 26) c = mix(c, rgb(50, 90, 140), (1 - d2 / 26) * 0.35f);
  float d3 = hypotf(lx - 150, ly - 104);
  if (d3 < 20) c = mix(c, rgb(150, 130, 70), (1 - d3 / 20) * 0.35f);
  return c;
}

static void mapButton(LovyanGFX& g, int cx, int cy, void (*glyph)(LovyanGFX&, int, int, Color)) {
  g.fillSmoothCircle(cx, cy, 9, rgb(46, 52, 60));
  g.fillSmoothCircle(cx, cy, 8, ds::pill);
  glyph(g, cx, cy, ds::white);
}

static void drawMap(LovyanGFX& g, int oy, const WeatherData& w, const UiState& st) {
  const int TY = 166 - oy;
  if (visible(g, TY - 9, 18)) {
    text(g, M11, ds::white, 193, TY, "Weather condition map", D::middle_left);
    pillButton(g, 472, TY - 8, "24 hr");
  }

  const int X = 192, Y = 179 - oy, W = 280, H = 132;
  if (!visible(g, Y, H)) return;
  Sky sky = skyFromWmo(w.code);
  float wet = (sky == Sky::Rain || sky == Sky::Drizzle || sky == Sky::Thunder || sky == Sky::Snow) ? 1.0f : 0.3f;
  paintRounded(g, X, Y, W, H, 12, [&](int px, int py) { return mapColor(px - X + 0.5f, py - Y + 0.5f, wet); });

  // Precipitation legend
  glass(g, X + 7, Y + 7, 136, 31, 7, rgb(24, 28, 34), 0.85f);
  text(g, R8, ds::white, X + 13, Y + 14, "Precipitation", D::middle_left);
  {
    const Color stops[] = {rgb(255, 92, 110), rgb(110, 130, 255), rgb(80, 210, 235), rgb(245, 214, 110)};
    const int bx = X + 13, by = Y + 21, bw = 124;
    for (int i = 0; i < bw; i++) {
      float f = i / (float)(bw - 1) * 3;
      int s = f >= 3 ? 2 : (int)f;
      g.drawFastVLine(bx + i, by, 3, mix(stops[s], stops[s + 1], f - s));
    }
    const char* labels[] = {"Extreme", "Heavy", "Moderate", "Light"};
    int lx = bx;
    for (int i = 0; i < 4; i++) lx += text(g, R8, ds::dim, lx, Y + 31, labels[i], D::middle_left) + 4;
  }

  // Right-hand controls
  const int rx = X + W - 17;
  card(g, rx - 9, Y + 27, 18, 34, 9, rgb(46, 52, 60), rgb(46, 52, 60));
  card(g, rx - 8, Y + 28, 16, 32, 8, ds::pill, ds::pill);
  glyphPlus(g, rx, Y + 37, ds::white);
  glyphMinus(g, rx, Y + 52, ds::white);
  mapButton(g, rx, Y + 78, glyphLocate);
  mapButton(g, rx, Y + 99, glyphList);
  mapButton(g, rx, Y + 120, glyphLayers);

  char buf[32], name[24];
  // Main city
  {
    const int cx = X + 174;
    shortName(name, sizeof name, st.location);
    text(g, R8, rgb(200, 225, 240), cx, Y + 42, name, D::middle_center);
    drawWeatherIcon(g, cx, Y + 56, 18, w.code, w.isDay);
    snprintf(buf, sizeof buf, "%d" DEG, roundi(w.temp));
    text(g, M16, ds::white, cx, Y + 82, buf, D::baseline_center);
    char wbuf[16], hbuf[8];
    snprintf(wbuf, sizeof wbuf, "%d %s", roundi(w.windSpeed), st.metric ? "km/h" : "mph");
    snprintf(hbuf, sizeof hbuf, "%d%%", roundi(w.humidity));
    int ww = textWidth(g, R8, wbuf), hw = textWidth(g, R8, hbuf);
    int total = 11 + ww + 8 + 9 + hw, x0 = cx - total / 2;
    glyphWind(g, x0 + 4, Y + 90, ds::white);
    text(g, R8, ds::white, x0 + 11, Y + 90, wbuf, D::middle_left);
    glyphDrop(g, x0 + 11 + ww + 12, Y + 90, ds::white);
    text(g, R8, ds::white, x0 + 11 + ww + 17, Y + 90, hbuf, D::middle_left);
  }
  // City 2: teardrop marker
  {
    const int cx = X + 86, top = Y + 44;
    paintRounded(g, cx - 12, top, 24, 38, 12, [&](int px, int py) { return mix(readRGB(g, px, py), rgb(22, 44, 72), 0.92f); });
    const CityNow& c = w.mapCities[0];
    drawWeatherIcon(g, cx, top + 13, 15, c.valid ? c.code : 3, c.valid ? c.isDay : true);
    snprintf(buf, sizeof buf, c.valid ? "%d" DEG : "--", roundi(c.temp));
    text(g, M10, ds::white, cx, top + 29, buf, D::middle_center);
  }
  // City 3: small marker
  {
    const int cx = X + 154, cy = Y + 104;
    const CityNow& c = w.mapCities[1];
    drawWeatherIcon(g, cx, cy - 6, 15, c.valid ? c.code : 1, c.valid ? c.isDay : true);
    snprintf(buf, sizeof buf, c.valid ? "%d" DEG : "--", roundi(c.temp));
    text(g, M10, ds::white, cx + 1, cy + 9, buf, D::middle_center);
  }

  // Location chips
  const char* names[3] = {st.location, st.mapNames[0], st.mapNames[1]};
  const uint8_t codes[3] = {w.code, w.mapCities[0].code, w.mapCities[1].code};
  for (int i = 0; i < 3; i++) {
    const int cx = X + 7 + i * 44, cy = Y + 90;
    glass(g, cx, cy, 41, 35, 7, i == 0 ? rgb(70, 84, 98) : rgb(30, 35, 42), i == 0 ? 0.75f : 0.8f);
    Sky s = skyFromWmo(codes[i]);
    glyphCloud(g, cx + 9, cy + 9, s == Sky::Clear ? ds::gold : ds::white);
    snprintf(buf, sizeof buf, "%s", names[i] ? names[i] : "");
    char* rest = strchr(buf, ',');
    const char* line2 = "";
    if (rest) {
      rest[1] = 0;  // keep the comma on line 1, like the design
      line2 = names[i] + (rest - buf) + 1;
      while (*line2 == ' ') line2++;
    }
    if (textWidth(g, R8, buf) > 37 && rest) rest[0] = 0;  // drop the comma before truncating
    while (strlen(buf) > 2 && textWidth(g, R8, buf) > 37) buf[strlen(buf) - 1] = 0;
    text(g, R8, i == 0 ? ds::white : ds::text2, cx + 3, cy + 20, buf, D::middle_left);
    text(g, R8, i == 0 ? ds::white : ds::text2, cx + 3, cy + 29, line2, D::middle_left);
  }
}

// ---------------------------------------------------------------------------
// Strip renderer
// ---------------------------------------------------------------------------
template <class Paint>
static void renderStrips(LovyanGFX& display, Paint&& paint) {
  LGFX_Sprite strip(&display);
  strip.setColorDepth(16);
  static const int heights[] = {80, 40, 20};
  int sh = 0;
  for (int h : heights)
    if (strip.createSprite(display.width(), h)) { sh = h; break; }

  if (!sh) {  // out of memory: draw straight to the panel (no AA blending, but readable)
    display.fillScreen(ds::screen);
    paint(display, 0);
    return;
  }
  for (int y = 0; y < display.height(); y += sh) {
    strip.fillScreen(ds::screen);
    paint(strip, y);
    strip.pushSprite(0, y);
  }
  strip.deleteSprite();
}

void uiDrawDashboard(LovyanGFX& display, const WeatherData& w, const UiState& st) {
  uiInit();
  renderStrips(display, [&](LovyanGFX& g, int oy) {
    drawSidebar(g, oy);
    drawNow(g, oy, w, st);
    drawHighlights(g, oy, w, st);
    drawForecast(g, oy, w);
    drawMap(g, oy, w, st);
  });
}

void uiDrawMessage(LovyanGFX& display, const char* title, const char* detail) {
  uiInit();
  const int cx = display.width() / 2, cy = display.height() / 2;
  renderStrips(display, [&](LovyanGFX& g, int oy) {
    Glow glow{(float)cx, (float)(cy + 40 - oy), 180.0f, ds::teal, 0.55f};
    card(g, 8, 9 - oy, display.width() - 16, display.height() - 18, 14, rgb(36, 40, 47), rgb(26, 36, 46), &glow);
    glyphLogo(g, cx, cy - 78 - oy, ds::blue);
    text(g, M10, ds::white, cx, cy - 62 - oy, "Weathry", D::middle_center);
    drawWeatherIcon(g, cx, cy - 18 - oy, 64, 95, true);
    text(g, M16, ds::white, cx, cy + 36 - oy, title, D::baseline_center);
    if (detail && *detail) text(g, R10, ds::text2, cx, cy + 56 - oy, detail, D::baseline_center);
  });
}
