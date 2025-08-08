#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "trigger_update.h"
#include "wifi_status.h"
#include "Arduino.h"
#include "HTTPClient.h"
#include "WiFiClient.h"

static const char* TAG = "UPDT";

//? URL To firmware download (SupaBase Firmware)
const char* firmwareURL = "https://gayaluhznhhewaamfcoy.supabase.co/storage/v1/object/public/actualizacion//firmware.bin";

static void pre_update() {
  //? Pre update func
}

bool hasInternet() {
  WiFiClient client;
  if (client.connect("8.8.8.8", 53)) return true;

  HTTPClient http;
  http.begin("http://clients3.google.com/generate_204");
  int code = http.GET();
  http.end();
  return code == 200 || code == 204;
}

uint8_t try_update_controller() {
  ESP_LOGI(TAG, "Init update");

  uint8_t cwl_status = get_wifi_status();
  if (cwl_status != WL_CONNECTED) {
    ESP_LOGI(TAG, "Failed no wi-fi");
    return OTA_UPDATE_NO_WIFI_CONNECTION;
  }

  if (hasInternet()) {
    ESP_LOGI(TAG, "Failed no internet");
    return OTA_UPDATE_NO_INTERNET;
  }

  
}
