#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "Arduino.h"
#include "handler_mute.h"
#include "button_hold.h"
#include "ei32_cpanel_pinout.h"
#include "alarms.h"

static const char* TAG = "MUTE";

void thread_mute(void* parametres) {
  bool prv_status = true;
  ButtonHold m_button(pin_bmute, 2000, INPUT_PULLUP, ButtonHold::NC);
  m_button.begin();
  while (true) {
    if (m_button.pressHold()) {
      reset_alarms();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}