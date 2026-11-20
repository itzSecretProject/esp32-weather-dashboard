#pragma once
#include "gfx.h"

enum class Sky : uint8_t { Clear, PartlyCloudy, Cloudy, Fog, Drizzle, Rain, Snow, Thunder };

Sky         skyFromWmo(uint8_t code);
const char* wmoDescription(uint8_t code, bool isDay);

// Glossy weather icon drawn from shaded shapes, centred in a size x size box.
void drawWeatherIcon(LovyanGFX& g, int cx, int cy, int size, uint8_t wmoCode, bool isDay);
// Soft, translucent cloud silhouette (background decoration).
void drawGhostCloud(LovyanGFX& g, int cx, int cy, int w, float alpha);

// Anti-aliased primitives
void aaArc(LovyanGFX& g, float cx, float cy, float r, float thick, float a0, float a1, Color c);
inline void aaRing(LovyanGFX& g, float cx, float cy, float r, float thick, Color c) { aaArc(g, cx, cy, r, thick, 0, 360, c); }
void aaLine(LovyanGFX& g, float x0, float y0, float x1, float y1, float thick, Color c);

// Outline glyphs (about 11 px), centred on (cx, cy)
void glyphLogo(LovyanGFX& g, int cx, int cy, Color c);
void glyphGrid(LovyanGFX& g, int cx, int cy, Color c);
void glyphMap(LovyanGFX& g, int cx, int cy, Color c);
void glyphPin(LovyanGFX& g, int cx, int cy, Color c);
void glyphPie(LovyanGFX& g, int cx, int cy, Color c);
void glyphCalendar(LovyanGFX& g, int cx, int cy, Color c);
void glyphGear(LovyanGFX& g, int cx, int cy, Color c);
void glyphBell(LovyanGFX& g, int cx, int cy, Color c);
void glyphSearch(LovyanGFX& g, int cx, int cy, Color c);
void glyphCloud(LovyanGFX& g, int cx, int cy, Color c);
void glyphDrop(LovyanGFX& g, int cx, int cy, Color c);
void glyphEye(LovyanGFX& g, int cx, int cy, Color c);
void glyphThermo(LovyanGFX& g, int cx, int cy, Color c);
void glyphWind(LovyanGFX& g, int cx, int cy, Color c);
void glyphSunrise(LovyanGFX& g, int cx, int cy, Color c, bool rising);
void glyphChevron(LovyanGFX& g, int cx, int cy, Color c);
void glyphPlus(LovyanGFX& g, int cx, int cy, Color c);
void glyphMinus(LovyanGFX& g, int cx, int cy, Color c);
void glyphLocate(LovyanGFX& g, int cx, int cy, Color c);
void glyphList(LovyanGFX& g, int cx, int cy, Color c);
void glyphLayers(LovyanGFX& g, int cx, int cy, Color c);
void drawAvatar(LovyanGFX& g, int cx, int cy, int r);
