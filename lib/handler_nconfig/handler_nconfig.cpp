#include "handler_nconfig.h"
#include "Arduino.h"
#include "button_hold.h"
#include "ei32_cpanel_pinout.h"
#include "main_config.h"

void thread_nconfig(void* parametres) {
  ButtonHold button(pin_bconfig, 2000);
  button.begin();
  
  for (;;) {
    if (button.pressHold()) {
      generate_new_config();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


