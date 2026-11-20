#pragma once
// ============================================================================
//  User configuration
//  Wi-Fi credentials live in include/secrets.h (not committed to git).
// ============================================================================

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "include/secrets.h not found. Copy include/secrets.example.h to include/secrets.h and enter your Wi-Fi details."
#endif

// --- Location ---------------------------------------------------------------
// Coordinates in decimal degrees. In Google Maps, right-click a spot to copy them.
#define LOCATION_NAME  "Paris, FR"
#define LATITUDE       48.8566
#define LONGITUDE      2.3522

// --- Extra cities on the "Weather condition map" card ------------------------
#define MAP_CITY2_NAME "London, UK"
#define MAP_CITY2_LAT  51.5072
#define MAP_CITY2_LON  -0.1276

#define MAP_CITY3_NAME "Madrid, ES"
#define MAP_CITY3_LAT  40.4168
#define MAP_CITY3_LON  -3.7038

// --- Units ------------------------------------------------------------------
#define USE_METRIC     1   // 1: °C, km/h, km    0: °F, mph, mi
#define USE_24H_CLOCK  1   // 1: 17:05           0: 5:05 PM

// --- Behaviour --------------------------------------------------------------
#define REFRESH_MINUTES  10    // weather refresh interval
#define BACKLIGHT_LEVEL  200   // 0-255
#define SCREEN_ROTATION  1     // 1: landscape, 3: landscape rotated 180°
