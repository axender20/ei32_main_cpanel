#include "hal_led.h"

// ---- Patrones (igual que antes) ----
static const uint32_t PATTERN_SLOW_BLINK[] = { 500, 500 };
static const uint32_t PATTERN_FAST_BLINK[] = { 150, 150 };
static const uint32_t PATTERN_DOUBLE_BLINK[] = { 120, 80, 120, 680 };
static const uint32_t PATTERN_ERROR_BLINK[] = { 100, 100, 100, 100, 100, 500 };

static const LedPattern PATTERN_SOLID_ON = { nullptr, 0, true };
static const LedPattern PATTERN_SOLID_OFF = { nullptr, 0, false };
static const LedPattern PATTERN_SLOW = { PATTERN_SLOW_BLINK,   2, false };
static const LedPattern PATTERN_FAST = { PATTERN_FAST_BLINK,   2, false };
static const LedPattern PATTERN_DOUBLE = { PATTERN_DOUBLE_BLINK, 4, false };
static const LedPattern PATTERN_ERROR = { PATTERN_ERROR_BLINK,  6, false };

// ---- Selección de patrón según estados ----
const LedPattern* choose_pattern(wl_status_t wifi, rl_status_t realtime) {
  if (realtime == RL_STATE_ERROR) return &PATTERN_ERROR;

  if (wifi == WL_NO_SHIELD || wifi == WL_NO_SSID_AVAIL || wifi == WL_DISCONNECTED)
    return &PATTERN_FAST;

  if (wifi == WL_CONNECT_FAILED) return &PATTERN_FAST;

  if (wifi == WL_CONNECTED) {
    switch (realtime) {
    case RL_STATE_DISCONNECTED:
    case RL_STATE_CONNECTING:
      return &PATTERN_SLOW;
    case RL_STATE_CONNECTED:
      return &PATTERN_SOLID_ON;
    case RL_STATE_JOINED:
      return &PATTERN_SOLID_ON;
    default:
      return &PATTERN_SLOW;
    }
  }

  if (wifi == WL_IDLE_STATUS || wifi == WL_SCAN_COMPLETED) return &PATTERN_SLOW;

  return &PATTERN_SLOW;
}