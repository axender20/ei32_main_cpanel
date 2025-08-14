#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "Arduino.h"
#include "main_config.h"
#include "handler_wifi.h"
#include "handler_nconfig.h"
#include "handler_realtime.h"

#include "rgb_led.h"
#include "button_hold.h"
#include "nmspc_out.h"

static const char* TAG = "main";

#include "EEPROM.h"
void setup() {
  delay(1000);
  ESP_LOGI(TAG, "Init code");
  outs::init();
  init_rgb_strip(200);
  load_config();


  init_thr_wifi();
  xTaskCreate(thread_wifi, "tsk_w", 8192, NULL, 1, NULL);

  xTaskCreate(thread_nconfig, "tsk_nc", 2048, NULL, 1, NULL);

  init_thr_realtime();
  xTaskCreate(thread_realtime, "tsk_rl", 8192, NULL, 1, NULL);
}

void loop() {
  yield();
}
