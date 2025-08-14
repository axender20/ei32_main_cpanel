#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "handler_realtime.h"
#include "Arduino.h"
#include "wifi_status.h"
#include "SupabaseRealtimeClient.h"
#include "realtime_status.h"
#include "utilities_realtime.h"
#include "alarms.h"

static const char* TAG = "RLTM";

//> Realtime status
uint8_t realtime_status = RL_STATE_DISCONNECTED;
uint8_t get_realtime_status() {
  return realtime_status;
}

void init_thr_realtime() {
  ESP_LOGI(TAG, "Init thread");
}

static void task_realtime();
void thread_realtime(void* parametres) {
  //? Intervalo del thread (1 ticks)
  static TickType_t lw_thread_rtime, interval_thread_rtime = 1;

  BaseType_t delayed;
  lw_thread_rtime = xTaskGetTickCount();
  while (1) {
    task_realtime();
    delayed = xTaskDelayUntil(&lw_thread_rtime, interval_thread_rtime);
    if (delayed == pdFALSE) {
      lw_thread_rtime = xTaskGetTickCount();
    }
  }
  vTaskDelete(NULL);
}

//> Supabase realtime
SupabaseRealtimeClient supa;
String supabaseURL = "uqmexwvdctoznkqhopry.supabase.co";
String apikey = "sb_publishable_nCoEPBqUzH2_skM_4NhCiw_y4nFUM94";

int prv_status = SupabaseRealtimeClient::STATE_ERROR;

void onChange(const SupabaseRealtimeClient::Change& ch) {
  ESP_LOGI(TAG, "Cambio recibido:");
  ESP_LOGI(TAG, "  message: %s", ch.message.c_str());

  bool enable_alarm = true;
  bool enable_updates = false;

  if (ch.message.equals("EI32_UPDATE")) {
    ESP_LOGI(TAG, "Request update. Initializing...");
  }
  else {
    //? Revisar si tiene el formato normal
    bool rcvdata[6];
    if (parseBinaryArray(ch.message, rcvdata)) {
      set_alarms(rcvdata);
    }
  }
}

static void task_realtime() {
  static bool config = false;
  if (get_wifi_status() == WL_CONNECTED) {
    if (!config) {
      ESP_LOGI(TAG, "Connected. Initializing...");
      // Configurar (no tocar WiFi aquí)
      supa.init(supabaseURL, apikey, true, 443);
      supa.setChangeCallback(onChange);
      supa.setLogLevel(SupabaseRealtimeClient::LogLevel::LOG_DEBUG);
      // Iniciar websocket (llamar sólo después de que la red ya esté conectada)
      supa.begin();
      config = true;
    }
    supa.loop();

    int c_status = supa.getState();
    realtime_status = c_status;
    if (prv_status != c_status) {
      prv_status = c_status;
      ESP_LOGI(TAG, " Status: %s", supa.getStateString().c_str());
    }
  }
  else {
    if (config) {
      ESP_LOGI(TAG, "Dissconnected :(. Closing...");
      supa.close();
      config = false;
    }
  }
}
