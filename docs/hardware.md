# Hardware

The firmware targets a **480 × 320** SPI display driven by a classic **ESP32** (no PSRAM needed).

## Option A: ESP32-3248S035 (recommended)

An all-in-one board, often sold as the "Cheap Yellow Display 3.5\"". It has an ESP32-WROOM, a 3.5" ST7796 panel,
a micro-SD slot and a USB-serial chip on one PCB. There's nothing to wire.

| | |
|---|---|
| PlatformIO environment | `cyd35` |
| Display controller | ST7796, 480 × 320 |
| Variants | `R` (resistive touch) and `C` (capacitive touch) both work; touch is not used |
| Typical price | USD 15–25 |

Pin mapping (pre-wired on the board):

| Signal | GPIO |
|---|---|
| SCLK | 14 |
| MOSI | 13 |
| MISO | 12 |
| DC | 2 |
| CS | 15 |
| Backlight | 27 |
| RESET | tied to EN |

## Option B: ESP32 DevKit + SPI display module

Any ESP32 DevKit (ESP32-WROOM-32) with a 3.5" 480 × 320 SPI module.

| Module | PlatformIO environment |
|---|---|
| ST7796 | `devkit-st7796` |
| ILI9488 | `devkit-ili9488` |

### Wiring

| Display pin | ESP32 GPIO | Notes |
|---|---|---|
| VCC | 3V3 | Some modules take 5 V; check yours |
| GND | GND | |
| SCK | 18 | |
| SDI / MOSI | 23 | |
| SDO / MISO | 19 | Optional (read-back is not used) |
| CS | 5 | |
| DC / RS | 16 | |
| RESET | 17 | |
| LED / BL | 4 | PWM backlight |

To use other pins, edit the `BOARD_DEVKIT_SPI` block in [`include/board.h`](../include/board.h).

## Adding another board

1. Add a new `#elif defined(BOARD_MY_BOARD)` block to [`include/board.h`](../include/board.h) with its pins and panel class.
   LovyanGFX supports many panels, for example `Panel_ILI9341`, `Panel_ST7789` and `Panel_GC9A01`.
2. Add a matching `[env:my-board]` section to [`platformio.ini`](../platformio.ini) with `-D BOARD_MY_BOARD`.

The layout is fixed at 480 × 320. A screen of another size needs layout changes in
[`src/ui/dashboard.cpp`](../src/ui/dashboard.cpp).

## Troubleshooting

| Symptom | Likely cause and fix |
|---|---|
| Blank white or black screen | Wrong environment, or a wiring error. Check the pins above. |
| Colors inverted | Add `-D TFT_INVERT=true` to the environment's `build_flags`. |
| Image upside down | Set `SCREEN_ROTATION` to `3` in `include/config.h`. |
| Upload fails or no serial port | Use a USB cable that carries data. Hold **BOOT** while the upload starts. |
| Brown-outs or resets on Wi-Fi start | Weak USB power. Use a better cable or power supply. |
