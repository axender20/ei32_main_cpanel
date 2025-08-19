#pragma once
#ifndef _REALTIME_STATUS_H_
#define _REALTIME_STATUS_H_

#include "stdint.h"

typedef enum : uint8_t {
  RL_STATE_DISCONNECTED = 0,
  RL_STATE_CONNECTING,
  RL_STATE_CONNECTED,
  RL_STATE_JOINED,
  RL_STATE_ERROR
}rl_status_t;

uint8_t get_realtime_status();
#endif