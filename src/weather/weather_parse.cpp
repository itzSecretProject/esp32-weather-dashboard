#include "weather_parse.h"

bool parseWeather(JsonDocument& doc, WeatherData& out) {
  JsonObject cur = doc["current"];
  JsonObject hourly = doc["hourly"];
  JsonObject daily = doc["daily"];
  if (cur.isNull() || hourly.isNull() || daily.isNull()) return false;

  WeatherData w;
  w.utcOffset  = doc["utc_offset_seconds"] | 0;
  w.updated    = (time_t)(cur["time"] | 0L);
  w.temp       = cur["temperature_2m"] | 0.0f;
  w.humidity   = cur["relative_humidity_2m"] | 0.0f;
  w.feelsLike  = cur["apparent_temperature"] | w.temp;
  w.isDay      = (cur["is_day"] | 1) != 0;
  w.code       = cur["weather_code"] | 0;
  w.windSpeed  = cur["wind_speed_10m"] | 0.0f;
  w.windDir    = cur["wind_direction_10m"] | 0.0f;
  w.windGust   = cur["wind_gusts_10m"] | 0.0f;
  w.dewPoint   = cur["dew_point_2m"] | 0.0f;
  w.pressure   = cur["surface_pressure"] | 0.0f;
  w.visibility = cur["visibility"] | 0.0f;
  w.uvIndex    = cur["uv_index"] | 0.0f;

  JsonArray ht = hourly["time"];
  w.hourlyCount = 0;
  for (size_t i = 0; i < ht.size() && w.hourlyCount < WeatherData::HOURS; i++) {
    HourlyPoint& p = w.hourly[w.hourlyCount++];
    p.time  = (time_t)(ht[i] | 0L);
    p.temp  = hourly["temperature_2m"][i] | 0.0f;
    p.wind  = hourly["wind_speed_10m"][i] | 0.0f;
    p.pop   = hourly["precipitation_probability"][i] | 0;
    p.code  = hourly["weather_code"][i] | 0;
    p.isDay = (hourly["is_day"][i] | 1) != 0;
  }

  JsonArray dt = daily["time"];
  w.dailyCount = 0;
  for (size_t i = 0; i < dt.size() && w.dailyCount < WeatherData::DAYS; i++) {
    DailyPoint& d = w.daily[w.dailyCount++];
    d.time = (time_t)(dt[i] | 0L);
    d.tMax = daily["temperature_2m_max"][i] | 0.0f;
    d.tMin = daily["temperature_2m_min"][i] | 0.0f;
    d.code = daily["weather_code"][i] | 0;
    d.pop  = daily["precipitation_probability_max"][i] | 0;
  }
  if (w.dailyCount == 0 || w.hourlyCount == 0) return false;

  w.sunrise = (time_t)(daily["sunrise"][0] | 0L);
  w.sunset  = (time_t)(daily["sunset"][0] | 0L);
  w.uvMax   = daily["uv_index_max"][0] | 0.0f;
  w.valid   = true;
  for (int i = 0; i < 2; i++) w.mapCities[i] = out.mapCities[i];  // fetched separately
  out = w;
  return true;
}

static bool parseCity(JsonObject o, CityNow& c) {
  JsonObject cur = o["current"];
  if (cur.isNull()) return false;
  c.temp     = cur["temperature_2m"] | 0.0f;
  c.wind     = cur["wind_speed_10m"] | 0.0f;
  c.humidity = cur["relative_humidity_2m"] | 0.0f;
  c.code     = cur["weather_code"] | 0;
  c.isDay    = (cur["is_day"] | 1) != 0;
  c.valid    = true;
  return true;
}

bool parseCities(JsonDocument& doc, CityNow* out, int n) {
  if (doc.is<JsonArray>()) {
    JsonArray arr = doc.as<JsonArray>();
    int i = 0;
    for (JsonObject o : arr) {
      if (i >= n) break;
      parseCity(o, out[i++]);
    }
    return i > 0;
  }
  return n > 0 && parseCity(doc.as<JsonObject>(), out[0]);
}
