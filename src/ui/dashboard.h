#pragma once
#include "gfx.h"
#include "../weather/weather_data.h"

struct UiState {
  time_t      now = 0;            // current UTC time
  const char* location = "";        // main city, e.g. "Paris, FR"
  const char* mapNames[2] = {"", ""};  // the two extra cities on the map
  bool        metric = true;
  bool        clock24 = true;
};

// Load fonts. Call once before drawing.
void uiInit();

// Full dashboard, 480x320. Rendered in horizontal strips through a small sprite,
// so it works on an ESP32 without PSRAM and never flickers.
void uiDrawDashboard(LovyanGFX& display, const WeatherData& w, const UiState& st);

// Full-screen status message (boot, Wi-Fi problems, ...).
void uiDrawMessage(LovyanGFX& display, const char* title, const char* detail);
