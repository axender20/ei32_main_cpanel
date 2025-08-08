#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "handler_update.h"
#include "Arduino.h"
#include "wifi_status.h"

static const char* TAG = "UPDT";
void init_thr_update() {
  ESP_LOGI(TAG, "Init thread");
}

void task_handler_update();
void thread_update(void* parametres) {
  //? Intervalo del thread (100 ticks)
  static TickType_t lw_thread_update, interval_thread_update = 100;

  BaseType_t delayed;
  lw_thread_update = xTaskGetTickCount();
  while (1) {
    task_handler_update();
    delayed = xTaskDelayUntil(&lw_thread_update, interval_thread_update);
    if (delayed == pdFALSE) {
      lw_thread_update = xTaskGetTickCount();
    }
  }
  vTaskDelete(NULL);
}

void task_handler_update() {
  uint8_t clws = get_wifi_status();
  if (clws == WL_CONNECTED) {

  }
  else {
    ESP_LOGV(TAG, "Non update available (ncw - wifi)");
  }
}