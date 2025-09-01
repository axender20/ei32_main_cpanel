#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "Arduino.h"
#include "main_config.h"
#include "handler_wifi.h"
#include "handler_nconfig.h"
#include "handler_realtime.h"
#include "handler_mute.h"
#include "handler_status.h"

#include "rgb_led.h"
#include "button_hold.h"
#include "nmspc_out.h"

#define RELEASE_DATE __DATE__ " " __TIME__
static const char* TAG = "main";

void setup() {
  delay(1000);
  const char* release_data = RELEASE_DATE;
  ESP_LOGI(TAG, "Init code. Released: %s", release_data);

  //> Init hardware peripherals
  outs::init();
  init_rgb_strip(200);

  //> Load config
  load_config();

  //> Make functionallity
  init_thr_wifi();
  xTaskCreate(thread_wifi, "tsk_w", 8192, NULL, 1, NULL);

  xTaskCreate(thread_nconfig, "tsk_nc", 2048, NULL, 1, NULL);

  xTaskCreate(thread_realtime, "tsk_rl", 8192, NULL, 1, NULL);

  xTaskCreate(thread_mute, "tsk_m", 2048, NULL, 1, NULL);

  xTaskCreate(thread_status, "tsk_st", 2048, NULL, 1, NULL);
}

void loop() {
  yield();
}
