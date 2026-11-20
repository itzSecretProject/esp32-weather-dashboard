// ESP32 Weather Dashboard
// Fetches weather from Open-Meteo and draws a dark dashboard on a 480x320 screen.
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "board.h"
#include "config.h"
#include "ui/dashboard.h"
#include "weather/weather_api.h"

static LGFX        lcd;
static WeatherData weather;

static uint32_t lastFetchMs   = 0;
static bool     lastFetchOk   = false;
static time_t   apiTime       = 0;  // server time from the last fetch, fallback clock until NTP syncs
static uint32_t apiTimeMs     = 0;
static int      lastDrawnMin  = -1;

static constexpr uint32_t REFRESH_MS = REFRESH_MINUTES * 60UL * 1000UL;
static constexpr uint32_t RETRY_MS   = 30UL * 1000UL;

static time_t nowUtc() {
  time_t t = time(nullptr);
  if (t > 1700000000) return t;  // NTP has synced
  if (apiTime) return apiTime + (millis() - apiTimeMs) / 1000;
  return 0;
}

static bool connectWifi(uint32_t timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) delay(250);
  return WiFi.status() == WL_CONNECTED;
}

static void redraw() {
  UiState st;
  st.now           = nowUtc();
  st.location      = LOCATION_NAME;
  st.mapNames[0]   = MAP_CITY2_NAME;
  st.mapNames[1]   = MAP_CITY3_NAME;
  st.metric        = USE_METRIC;
  st.clock24       = USE_24H_CLOCK;
  uint32_t t0 = millis();
  uiDrawDashboard(lcd, weather, st);
  Serial.printf("Redraw took %lu ms, free heap %u\n", (unsigned long)(millis() - t0), ESP.getFreeHeap());
}

static void updateWeather() {
  char err[64] = "";
  if (!weather.valid) uiDrawMessage(lcd, "Loading weather", LOCATION_NAME);
  lastFetchOk = fetchWeather(weather, err, sizeof err);
  lastFetchMs = millis();
  if (lastFetchOk) {
    apiTime   = weather.updated;
    apiTimeMs = millis();
    if (time(nullptr) > 1700000000) weather.updated = time(nullptr);
    Serial.printf("Weather updated: %.1f, code %d\n", weather.temp, weather.code);
    lastDrawnMin = -1;  // force a redraw
  } else {
    Serial.printf("Weather fetch failed: %s\n", err);
    if (!weather.valid) uiDrawMessage(lcd, "Couldn't get weather", err);
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(SCREEN_ROTATION);
  lcd.setBrightness(BACKLIGHT_LEVEL);
  uiInit();

  uiDrawMessage(lcd, "Connecting to Wi-Fi", WIFI_SSID);
  if (!connectWifi(20000)) {
    uiDrawMessage(lcd, "Wi-Fi not connected", "Check WIFI_SSID and WIFI_PASSWORD in config.h");
  }
  // Keep the system clock in UTC; the dashboard applies the location's offset from Open-Meteo.
  configTime(0, 0, "pool.ntp.org", "time.google.com");
}

void loop() {
  static uint32_t lastReconnectMs = 0;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (!connected && millis() - lastReconnectMs > RETRY_MS) {
    lastReconnectMs = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  if (connected) {
    bool due = !weather.valid || !lastFetchMs ||
               millis() - lastFetchMs > (lastFetchOk ? REFRESH_MS : RETRY_MS);
    if (due) updateWeather();
  }

  if (weather.valid) {
    time_t now = nowUtc();
    int minute = (int)((now / 60) % 60);
    if (minute != lastDrawnMin) {
      lastDrawnMin = minute;
      redraw();
    }
  }
  delay(200);
}
