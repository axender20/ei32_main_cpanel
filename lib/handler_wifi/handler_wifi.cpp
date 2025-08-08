#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "handler_wifi.h"
#include "wifi_status.h"
#include "Arduino.h"
#include "WiFi.h"
#include "main_config.h"

static const char* TAG = "WIFI";

//* WI-FI Status */
static uint8_t current_wl_status;
uint8_t get_wifi_status() {
  return current_wl_status;
}

void init_thr_wifi() {
  //? main config pre-loaded 
  ESP_LOGI(TAG, "Init thread");

  //! Try first connection
  ESP_LOGI(TAG, "Try connect to: %s", cnfg.get_ssid());
  WiFi.begin(cnfg.get_ssid(), cnfg.get_pass());

  ulong cnt_timeout = millis();
  ulong tick_timeout = 5000u;

  uint8_t wlst = WL_DISCONNECTED;
  while ((millis() - cnt_timeout < tick_timeout) || (wlst != WL_CONNECTED)) {
    wlst = WiFi.status();
    delay(100);
  }

  const char* str_wls = (wlst == WL_CONNECTED) ? "Connected correctly" : "Could not connect";
  ESP_LOGI(TAG, "%s", str_wls);
  current_wl_status = wlst;
}

void thread_wifi(void* parametres) {
  while (true) {
    current_wl_status = WiFi.status();
    if (current_wl_status != WL_CONNECTED) {
      //> Try reconect
      ESP_LOGE(TAG, "Wifi disconnected. Try reconect");
      ulong ticker = millis();
      ulong timeout = 10000u;

      while (millis() - ticker <= timeout) {
        current_wl_status = WiFi.status();
        if (current_wl_status == WL_CONNECTED) {
          break;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      const char* str_wls = (current_wl_status == WL_CONNECTED) ? "Re:connected correctly" : "Could not connect";
      ESP_LOGI(TAG, "%s", str_wls);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}


