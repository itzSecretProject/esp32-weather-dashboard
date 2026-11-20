#pragma once
#include "weather_data.h"

// Download the forecast from Open-Meteo (free, no API key) into `out`.
// On failure `out` is left unchanged and a short reason is written to `err`.
bool fetchWeather(WeatherData& out, char* err, size_t errLen);
