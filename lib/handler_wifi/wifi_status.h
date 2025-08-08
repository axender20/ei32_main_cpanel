#pragma once
#ifndef _WIFI_STATUS_H_
#define _WIFI_STATUS_H_

#include "stdint.h"
#include "WiFiType.h"

/// @brief Get Wi-fi status
/// @return Wifi Type
uint8_t get_wifi_status();
#endif