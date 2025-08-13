#include "rgb_led.h"

RGBLed::RGBLed(Adafruit_NeoPixel* strip, uint16_t index) :
  _strip(strip), _index(index), _r(0), _g(0), _b(255) {
}

void RGBLed::on() {
  _wait();
  _strip->setPixelColor(_index, _r, _g, _b);
  _strip->show();
}

void RGBLed::off() {
  _wait();
  _strip->setPixelColor(_index, 0, 0, 0);
  _strip->show();
}

void RGBLed::switch_color(uint8_t r, uint8_t g, uint8_t b) {
  _wait();
  _r = g;
  _g = r;
  _b = b;
  _strip->setPixelColor(_index, _r, _g, _b);
  _strip->show();
}

void RGBLed::_wait() {
  while (!_strip->canShow()) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static const uint8_t NUM_LEDS = 1u;

static Adafruit_NeoPixel m_strip(NUM_LEDS, 38, NEO_GRB + NEO_KHZ800);

RGBLed wrgb_1(&m_strip, 0);

void init_rgb_strip(uint8_t bigbrightness) {
  m_strip.begin();
  m_strip.setBrightness(bigbrightness);

  wrgb_1.off();
}
