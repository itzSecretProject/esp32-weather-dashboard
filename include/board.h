#pragma once
// ============================================================================
//  Display wiring. Pick your board in platformio.ini (the -D BOARD_... flag).
//  The UI is drawn for a 480x320 landscape screen.
// ============================================================================
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#if defined(BOARD_ESP32_3248S035)
// "Cheap Yellow Display" 3.5" — ESP32-3248S035 (R or C), ST7796 480x320.
// Everything is pre-wired on this board, nothing to connect.
  #define TFT_SPI_HOST  SPI2_HOST
  #define TFT_SCLK      14
  #define TFT_MOSI      13
  #define TFT_MISO      12
  #define TFT_DC         2
  #define TFT_CS        15
  #define TFT_RST       -1
  #define TFT_BL        27
  #define TFT_PANEL     lgfx::Panel_ST7796
  #ifndef TFT_INVERT
    #define TFT_INVERT  false
  #endif

#elif defined(BOARD_DEVKIT_SPI)
// Any ESP32 DevKit + a 3.5" 480x320 SPI display module (ST7796 or ILI9488).
// Wiring (display pin -> ESP32 GPIO):
//   VCC->3V3  GND->GND  SCK->18  SDI/MOSI->23  SDO/MISO->19
//   CS->5  DC/RS->16  RESET->17  LED/BL->4
  #define TFT_SPI_HOST  SPI3_HOST
  #define TFT_SCLK      18
  #define TFT_MOSI      23
  #define TFT_MISO      19
  #define TFT_DC        16
  #define TFT_CS         5
  #define TFT_RST       17
  #define TFT_BL         4
  #if defined(PANEL_ILI9488)
    #define TFT_PANEL   lgfx::Panel_ILI9488
  #else
    #define TFT_PANEL   lgfx::Panel_ST7796
  #endif
  #ifndef TFT_INVERT
    #define TFT_INVERT  false
  #endif

#else
  #error "No board selected. Add -D BOARD_ESP32_3248S035 or -D BOARD_DEVKIT_SPI to build_flags."
#endif

class LGFX : public lgfx::LGFX_Device {
  TFT_PANEL       _panel;
  lgfx::Bus_SPI   _bus;
  lgfx::Light_PWM _light;

 public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = TFT_SPI_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.pin_sclk    = TFT_SCLK;
      cfg.pin_mosi    = TFT_MOSI;
      cfg.pin_miso    = TFT_MISO;
      cfg.pin_dc      = TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs          = TFT_CS;
      cfg.pin_rst         = TFT_RST;
      cfg.pin_busy        = -1;
      cfg.panel_width     = 320;
      cfg.panel_height    = 480;
      cfg.offset_x        = 0;
      cfg.offset_y        = 0;
      cfg.offset_rotation = 0;
      cfg.readable        = false;  // we never read back from the panel
      cfg.invert          = TFT_INVERT;
      cfg.rgb_order       = false;
      cfg.dlen_16bit      = false;
      cfg.bus_shared      = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl      = TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
