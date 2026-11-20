#include "weather_api.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "weather_parse.h"

static const char* UNITS = USE_METRIC ? "&temperature_unit=celsius&wind_speed_unit=kmh"
                                      : "&temperature_unit=fahrenheit&wind_speed_unit=mph";

static bool getJson(const char* url, JsonDocument& doc, char* err, size_t errLen) {
  WiFiClientSecure client;
  client.setInsecure();  // public weather data; skips certificate checks so no root CA has to be kept up to date
  HTTPClient http;
  http.useHTTP10(true);  // no chunked encoding, so ArduinoJson can read the stream directly
  http.setTimeout(15000);

  if (!http.begin(client, url)) {
    snprintf(err, errLen, "Could not start request");
    return false;
  }
  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    snprintf(err, errLen, "HTTP error %d", status);
    http.end();
    return false;
  }
  DeserializationError jsonErr = deserializeJson(doc, http.getStream());
  http.end();
  if (jsonErr) {
    snprintf(err, errLen, "Bad data: %s", jsonErr.c_str());
    return false;
  }
  return true;
}

bool fetchWeather(WeatherData& out, char* err, size_t errLen) {
  char url[900];
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,"
           "wind_speed_10m,wind_direction_10m,wind_gusts_10m,dew_point_2m,surface_pressure,visibility,uv_index"
           "&hourly=temperature_2m,precipitation_probability,weather_code,wind_speed_10m,is_day"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,"
           "precipitation_probability_max"
           "&timezone=auto&timeformat=unixtime&forecast_days=7&forecast_hours=48%s",
           (double)LATITUDE, (double)LONGITUDE, UNITS);
  {
    JsonDocument doc;
    if (!getJson(url, doc, err, errLen)) return false;
    if (!parseWeather(doc, out)) {
      snprintf(err, errLen, "Unexpected response");
      return false;
    }
  }

  // The two extra cities on the map: one small request, failure here is not fatal.
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f,%.4f&longitude=%.4f,%.4f"
           "&current=temperature_2m,relative_humidity_2m,is_day,weather_code,wind_speed_10m"
           "&timezone=auto%s",
           (double)MAP_CITY2_LAT, (double)MAP_CITY3_LAT, (double)MAP_CITY2_LON, (double)MAP_CITY3_LON, UNITS);
  JsonDocument doc;
  char cityErr[48];
  if (getJson(url, doc, cityErr, sizeof cityErr)) parseCities(doc, out.mapCities, 2);
  return true;
}
