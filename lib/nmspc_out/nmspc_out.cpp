#include "nmspc_out.h"
#include "Adafruit_XCA9554.h"
#include "ei32_cpanel_pinout.h"

//? Helpers
SemaphoreHandle_t xIMutex;
void xIM_init() {
  xIMutex = xSemaphoreCreateMutex();
  if (xIMutex == NULL) {
    //TODO: Panic xd
  }
  xSemaphoreGive(xIMutex);
}

bool xIM_take(uint32_t tick = portMAX_DELAY) {
  bool resul = (xSemaphoreTake(xIMutex, tick)) ? true : false;
  return resul;
}

void xIM_give() {
  xSemaphoreGive(xIMutex);
}

Adafruit_XCA9554 ioex;

void outs::init() {
  //> Mutex config
  xIM_init();

  //> Wire config
  Wire.setPins(pin_sda, pin_scl);
  Wire.begin();

  //> Expander config
  const uint8_t ioex_address = 0x20;
  bool i_begun = ioex.begin(ioex_address, &Wire);

  //TODO: Panic no fue posible iniciar el i2c
  if (!i_begun) {}

  for (uint8_t i = 0; i < 8; i++) {
    ioex.pinMode(i, OUTPUT);
  }

  for (uint8_t i = 0; i < 8; i++) {
    ioex.digitalWrite(i, false);
  }
}

void outs::set_state(uint8_t _pin, bool value) {
  if (_pin >= 8) return;

  xIM_take();
  ioex.digitalWrite(_pin, value);
  xIM_give();
}
