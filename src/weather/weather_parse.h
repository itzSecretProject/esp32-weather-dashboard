#pragma once
#include <ArduinoJson.h>

#include "weather_data.h"

// Turn an Open-Meteo /v1/forecast response (timeformat=unixtime) into WeatherData.
// Leaves `out` untouched and returns false if the response is unusable.
bool parseWeather(JsonDocument& doc, WeatherData& out);

// Parse a multi-location Open-Meteo response (current conditions only) into `out[0..n-1]`.
// The response is a JSON array when several coordinates are requested.
bool parseCities(JsonDocument& doc, CityNow* out, int n);
