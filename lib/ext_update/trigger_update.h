#pragma once
#ifndef _TRIGGER_UPDATE_H_
#define _TRIGGER_UPDATE_H_

#include "stdint.h"

typedef enum {
  OTA_UPDATE_OK = 0,              // Actualización exitosa
  OTA_UPDATE_FAILED = 1,          // Error genérico durante la actualización
  OTA_UPDATE_NO_INTERNET = 2,     // No había conexión a Internet
  OTA_UPDATE_INVALID_IMAGE = 3,   // El archivo descargado no es válido
  OTA_UPDATE_APPLY_ERROR = 4,     // Error al aplicar el firmware
  OTA_UPDATE_ABORTED = 5,          // Actualización cancelada o interrumpida
  OTA_UPDATE_NO_WIFI_CONNECTION = 6
} ota_update_result_t;

static uint8_t try_update_controller();
#endif