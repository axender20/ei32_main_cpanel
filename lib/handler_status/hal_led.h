#pragma once
#ifndef _HAL_LED_EXTERNAL_H_
#define _HAL_LED_EXTERNAL_H_

#include "stdint.h"
#include "wifi_status.h"
#include "realtime_status.h"

struct LedPattern {
  const uint32_t* durations;
  uint8_t len; // si 0 -> sólido
  bool solidValueIfLenZero;
};

const LedPattern* choose_pattern(wl_status_t wifi, rl_status_t realtime);
#endif
