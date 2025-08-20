#pragma once
#ifndef _TRIGGER_UPDATE_H_
#define _TRIGGER_UPDATE_H_

#include "stdint.h"

typedef enum {
  OTA_UPDATE_OK = 0,              // Actualización exitosa
  OTA_UPDATE_FAILED = 1,          // Error genérico durante la actualización
  OTA_UPDATE_NO_INTERNET = 2,     // No había conexión a Internet
  OTA_UPDATE_ABORTED = 3,          // Actualización cancelada o interrumpida
  OTA_UPDATE_NO_WIFI_CONNECTION = 4,
  OTA_UPDATE_FAIL_DOWNLOAD = 5,
  OTA_UPDATE_FAIL_MEMORY = 6,
  OTA_UPDATE_FAIL_READ = 7,
  OTA_UPDATE_FAIL_DOWNLOAD_SIG = 8,
  OTA_UPDATE_FAIL_READ_SIG = 9,
  OTA_UPDATE_FAIL_SIGNATURE = 10,
  OTA_UPDATE_FAIL_OTA_BEGIN = 11,
  OTA_UPDATE_FAIL_OTA_WRITE = 12,
  OTA_UPDATE_FAIL_OTA_END = 13,
} ota_update_result_t;

void try_update_controller();
#endif