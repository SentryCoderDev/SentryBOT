#ifndef SENTRY_PERIPHERALS_OLED_H
#define SENTRY_PERIPHERALS_OLED_H

#include <Arduino.h>
#include "../xConfig.h"

#if defined(OLED_ENABLED) && OLED_ENABLED
#if __has_include(<Adafruit_SSD1306.h>)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class OledDisplay {
public:
  bool begin(uint8_t addr = OLED_I2C_ADDR, uint8_t w = 128, uint8_t h = 64){
    _addr = addr; _w = w; _h = h;
#if defined(ARDUINO_ARCH_AVR)
    Wire.begin();
#else
    Wire.begin();
#endif
    if (!_display){
      _display = new Adafruit_SSD1306(_w, _h, &Wire);
    }
    if (!_display->begin(SSD1306_SWITCHCAPVCC, _addr)) return false;
    _display->clearDisplay();
    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    _display->display();
    return true;
  }

  void showLogo(){
    if (!_display) return;
    _display->clearDisplay();
    // Custom XBM logo support (draw via drawBitmap)
    extern const unsigned char image_Ads_z_bits[] U8X8_PROGMEM;
    _display->drawBitmap(27, 0, image_Ads_z_bits, 74, 64, SSD1306_WHITE);
    _display->display();
  }

private:
  uint8_t _addr{OLED_I2C_ADDR};
  uint8_t _w{128}, _h{64};
  Adafruit_SSD1306 *_display{nullptr};
};
#else
// If Adafruit_SSD1306 not available, provide a stub so firmware still compiles
class OledDisplay {
public:
  bool begin(uint8_t = 0x3C, uint8_t = 128, uint8_t = 64){ return false; }
  void showLogo() {}
};
#endif // include check
#else
// OLED disabled -> empty stub
class OledDisplay { public: bool begin(uint8_t=0){ return false; } void showLogo(){} };
#endif // OLED_ENABLED

#endif // SENTRY_PERIPHERALS_OLED_H
