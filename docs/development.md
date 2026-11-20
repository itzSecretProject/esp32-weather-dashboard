# Development guide

## Architecture

```
        ┌──────────────┐  HTTPS/JSON  ┌────────────────────┐
        │  Open-Meteo  │ ───────────▶ │  weather/          │
        │  (forecast)  │              │  weather_api.cpp   │  ESP32 only (WiFi, HTTPClient)
        └──────────────┘              │  weather_parse.cpp │  portable (ArduinoJson)
                                      └─────────┬──────────┘
                                                │ WeatherData (plain struct)
                                      ┌─────────▼──────────┐
        main.cpp ── refresh loop ───▶ │  ui/dashboard.cpp  │  portable (LovyanGFX)
        (Wi-Fi, NTP, timers)          │  ui/icons.cpp      │
                                      └─────────┬──────────┘
                                                │ LovyanGFX& (panel or sprite)
                               ┌────────────────┴────────────────┐
                               ▼                                 ▼
                     SPI display (ESP32)             PNG file (desktop preview)
```

- **`src/weather/`** downloads and parses data. `weather_parse.cpp` has no Arduino dependency, so the
  preview and tests can reuse it.
- **`src/ui/`** draws only. Every function takes a `LovyanGFX&`, so the same code renders to the panel
  or to an off-screen sprite on the desktop.
- **`src/main.cpp`** connects the pieces. It handles Wi-Fi, NTP, the 10-minute refresh and the redraw each minute.
- **`include/`** holds the user configuration (`config.h`, `secrets.h`) and the display wiring (`board.h`).

### Rendering

The ESP32 has no memory for a full 480 × 320 × 16-bit frame buffer (300 KB). The dashboard is drawn
in **horizontal strips** of 80 px into a 77 KB sprite. Each strip is pushed to the panel over DMA.
Every draw call is offset by the strip's position, and the sprite clips the drawing.

- Text uses **anti-aliased VLW fonts** generated from Poppins, which LovyanGFX alpha-blends.
- Cards, glows and the map background are computed per pixel. They get **ordered dithering** (4 × 4 Bayer),
  so dark gradients don't band on the RGB565 panel.
- A full redraw happens once a minute. Nothing flickers, because the panel only ever receives finished strips.

### Data flow and timing

| What | When |
|---|---|
| Forecast (current, 48 h hourly, 7 days) | Every `REFRESH_MINUTES`, or every 30 s after a failure |
| Extra map cities (current conditions) | Same request cycle. A failure here doesn't stop the update. |
| Clock | NTP in UTC. The location's UTC offset comes from Open-Meteo (`timezone=auto`). |
| Screen redraw | Every minute and after each successful fetch |

## Desktop preview

The preview compiles `src/ui` and `src/weather/weather_parse.cpp` for the desktop and renders a PNG.
It's the fastest way to work on the layout.

**Requirements:** CMake ≥ 3.16, a C++17 compiler and SDL2 (`brew install cmake sdl2` or
`sudo apt install cmake libsdl2-dev`).

```bash
cmake -S tools/preview -B build/preview -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build/preview -j
```

```bash
./build/preview/preview --weather tools/preview/fixtures/paris.json --cities tools/preview/fixtures/paris-cities.json --out preview.png
```

Run `./build/preview/preview --help` for every option (units, 12-hour clock, fixed time, city labels, status screen).

To regenerate all the screenshots in `docs/images/`:

```bash
tools/preview/render-docs.sh
```

### Using live data

```bash
curl -s "https://api.open-meteo.com/v1/forecast?latitude=48.85&longitude=2.35&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m,dew_point_2m,surface_pressure,visibility,uv_index&hourly=temperature_2m,precipitation_probability,weather_code,wind_speed_10m,is_day&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,precipitation_probability_max&timezone=auto&timeformat=unixtime&forecast_days=7&forecast_hours=48" -o weather.json
```

```bash
./build/preview/preview --weather weather.json --out preview.png
```

## Fonts

Fonts are embedded as anti-aliased VLW bitmaps in `src/ui/fonts.cpp`. To change sizes or weights,
edit the `FONTS` table in [`tools/fonts/make_fonts.py`](../tools/fonts/make_fonts.py) and regenerate:

```bash
pip install pillow
```

```bash
python3 tools/fonts/make_fonts.py
```

Font IDs in `dashboard.cpp` (`R8`, `M16`, …) follow the order of that table.

## Continuous integration

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs on every push and pull request. It:

1. builds the firmware for every PlatformIO environment and uploads each `firmware.bin` as an artifact
2. builds the desktop preview on Linux and uploads the rendered screenshots

## Code style

- C++17, 2-space indentation, 120 columns (see `.editorconfig`)
- Colors are always `Color` (RGB888 `uint32_t`). LovyanGFX reads a plain `int` as RGB565.
- UI code must not include Arduino headers. That keeps it buildable on the desktop.
