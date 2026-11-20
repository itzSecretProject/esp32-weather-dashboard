#pragma once
// Plain data shared by the network code (ESP32) and the UI (ESP32 + desktop preview).
#include <stdint.h>
#include <time.h>

struct HourlyPoint {
  time_t  time;   // UTC epoch
  float   temp;
  float   wind;
  uint8_t pop;    // precipitation probability, %
  uint8_t code;   // WMO weather code
  bool    isDay;
};

struct DailyPoint {
  time_t  time;   // UTC epoch of local midnight
  float   tMax;
  float   tMin;
  uint8_t code;
  uint8_t pop;
};

// Current conditions for one of the extra cities shown on the map.
struct CityNow {
  bool    valid = false;
  float   temp = 0, wind = 0, humidity = 0;
  uint8_t code = 0;
  bool    isDay = true;
};

struct WeatherData {
  static constexpr int HOURS = 48;  // today + tomorrow (wind trend, "Tomorrow" chart)
  static constexpr int DAYS  = 7;

  bool    valid     = false;
  time_t  updated   = 0;   // UTC epoch of the last successful fetch
  int32_t utcOffset = 0;   // seconds, local time = UTC + utcOffset

  // Current conditions
  float   temp = 0, feelsLike = 0, humidity = 0, dewPoint = 0;
  float   windSpeed = 0, windDir = 0, windGust = 0;
  float   visibility = 0;  // metres
  float   uvIndex = 0, uvMax = 0;
  float   pressure = 0;    // hPa
  uint8_t code = 0;
  bool    isDay = true;
  time_t  sunrise = 0, sunset = 0;

  HourlyPoint hourly[HOURS];
  int         hourlyCount = 0;
  DailyPoint  daily[DAYS];
  int         dailyCount = 0;

  CityNow     mapCities[2];  // the two extra cities from config.h
};
