#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "Arduino.h"
#include "handler_status.h"

void task_status();

void thread_status(void* parametres) {
  //? Intervalo del thread (1 ticks)
  static TickType_t lw_thread_status, interval_thread_status = 10;

  BaseType_t delayed;
  lw_thread_status = xTaskGetTickCount();
  while (1) {
    task_status();
    delayed = xTaskDelayUntil(&lw_thread_status, interval_thread_status);
    if (delayed == pdFALSE) {
      lw_thread_status = xTaskGetTickCount();
    }
  }
  vTaskDelete(NULL);
}

#include "wifi_status.h"
#include "realtime_status.h"
#include "nmspc_out.h"
#include "hal_led.h"

static const uint8_t bit_sled = 7u;

void s_led(bool _state) {
  outs::set_state(bit_sled, _state);
}

void task_status() {
  static const LedPattern* currentPattern = nullptr;
  static uint8_t phaseIndex = 0;
  static uint32_t lastChangeMs = 0;
  static bool ledState = false;
  static wl_status_t prevWifi = (wl_status_t)0xFF;
  static rl_status_t prevRealtime = (rl_status_t)0xFF;

  wl_status_t wifi = (wl_status_t)get_wifi_status();
  rl_status_t realtime = (rl_status_t)get_realtime_status();

  const LedPattern* newPattern = choose_pattern(wifi, realtime);

  // Si cambió patrón o cualquiera de los estados, reiniciamos la secuencia
  if (newPattern != currentPattern || wifi != prevWifi || realtime != prevRealtime) {
    currentPattern = newPattern;
    phaseIndex = 0;
    lastChangeMs = millis();
    if (currentPattern->len == 0) {
      s_led(currentPattern->solidValueIfLenZero);
      ledState = currentPattern->solidValueIfLenZero;
    }
    else {
      s_led(true);
      ledState = true;
    }
    prevWifi = wifi;
    prevRealtime = realtime;
    return; 
  }

  // Si patrón sólido, nada más que comprobar cambios — retornamos rápidamente.
  if (currentPattern->len == 0) {
    return;
  }

  // Patrón por fases: comprobar si debemos avanzar fase
  uint32_t now = millis();
  uint32_t dur = currentPattern->durations[phaseIndex];

  if ((uint32_t)(now - lastChangeMs) >= dur) {
    phaseIndex = (phaseIndex + 1) % currentPattern->len;
    lastChangeMs = now;
    bool newLedState = ((phaseIndex % 2) == 0); // 0,2,4 -> ON ; 1,3,5 -> OFF
    if (newLedState != ledState) {
      s_led(newLedState);
      ledState = newLedState;
    }
  }
}