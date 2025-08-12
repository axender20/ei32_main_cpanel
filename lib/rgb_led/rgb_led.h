#pragma once
#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#include "Adafruit_NeoPixel.h"
class RGBLed {
public:
  RGBLed(Adafruit_NeoPixel* strip, uint16_t index);

  void on();
  void off();
  void switch_color(uint8_t r, uint8_t g, uint8_t b);

private:
  Adafruit_NeoPixel* _strip;
  uint16_t _index;
  uint8_t _r, _g, _b;
  void _wait();
};

/// @brief Inicializa la tira led, todos los leds apagados
void init_rgb_strip(uint8_t bigbrightness = 255);

extern RGBLed wrgb_1;
#endif