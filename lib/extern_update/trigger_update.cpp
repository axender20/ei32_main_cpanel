#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "trigger_update.h"
#include "wifi_status.h"
#include "Arduino.h"
#include "HTTPClient.h"
#include "WiFiClient.h"
#include "verify_signature.h"
#include "HTTPUpdate.h"

static const char* TAG = "UPDT";

//? URL To firmware download (SupaBase Firmware)
const char* updateURL = "https://uqmexwvdctoznkqhopry.supabase.co/storage/v1/object/public/update/";
const char* firmwarePath = "firmware.bin";
const char* sigPath = "firmware.sig";
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

void pre_init_update() {
  //> Do things prev update
}

uint8_t update_controller() {
  ESP_LOGI(TAG, "Init update");

  uint8_t cwl_status = get_wifi_status();
  if (cwl_status != WL_CONNECTED) {
    ESP_LOGI(TAG, "Failed no wi-fi");
    return OTA_UPDATE_NO_WIFI_CONNECTION;
  }

  if (!hasInternet()) {
    ESP_LOGI(TAG, "Failed no internet");
    return OTA_UPDATE_NO_INTERNET;
  }

  WiFiClientSecure client;
  client.setInsecure(); //Desable CA

  //> Descargar firmware
  String firmwareURL = String(updateURL) + firmwarePath;
  HTTPClient httpFirmware;
  httpFirmware.begin(client, firmwareURL);
  int httpCode = httpFirmware.GET();
  if (httpCode != HTTP_CODE_OK) {
    ESP_LOGE(TAG, "Failed to download firmware: %d", httpCode);
    httpFirmware.end();
    return OTA_UPDATE_FAIL_DOWNLOAD;
  }
  int contentLength = httpFirmware.getSize();
  WiFiClient* stream = httpFirmware.getStreamPtr();

  //! Leer firmware en buffer (ojo: ajustar tamaño o leer por chunks en producción)
  uint8_t* firmwareBuf = (uint8_t*)malloc(contentLength);
  if (!firmwareBuf) {
    ESP_LOGE(TAG, "No memory for firmware buffer");
    httpFirmware.end();
    return OTA_UPDATE_FAIL_MEMORY;
  }
  int readBytes = stream->readBytes(firmwareBuf, contentLength);
  httpFirmware.end();
  if (readBytes != contentLength) {
    ESP_LOGE(TAG, "Firmware read incomplete");
    free(firmwareBuf);
    return OTA_UPDATE_FAIL_READ;
  }

  //> Descargar firma
  String sigURL = String(updateURL) + sigPath;
  HTTPClient httpSig;
  httpSig.begin(client, sigURL);
  httpCode = httpSig.GET();
  if (httpCode != HTTP_CODE_OK) {
    ESP_LOGE(TAG, "Failed to download signature: %d", httpCode);
    httpSig.end();
    free(firmwareBuf);
    return OTA_UPDATE_FAIL_DOWNLOAD_SIG;
  }
  int sigLength = httpSig.getSize();
  WiFiClient* sigStream = httpSig.getStreamPtr();

  uint8_t* sigBuf = (uint8_t*)malloc(sigLength);
  if (!sigBuf) {
    ESP_LOGE(TAG, "No memory for signature buffer");
    httpSig.end();
    free(firmwareBuf);
    return OTA_UPDATE_FAIL_MEMORY;
  }
  readBytes = sigStream->readBytes(sigBuf, sigLength);
  httpSig.end();
  if (readBytes != sigLength) {
    ESP_LOGE(TAG, "Signature read incomplete");
    free(firmwareBuf);
    free(sigBuf);
    return OTA_UPDATE_FAIL_READ_SIG;
  }

  //> Verificar firma
  if (!verifySignature(firmwareBuf, contentLength, sigBuf, sigLength)) {
    ESP_LOGE(TAG, "Firmware signature invalid!");
    free(firmwareBuf);
    free(sigBuf);
    return OTA_UPDATE_FAIL_SIGNATURE;
  }

  ESP_LOGI(TAG, "Signature verified. Starting OTA...");

  //> Iniciar OTA con el firmware descargado (usar Update.begin + write)
  if (!Update.begin(contentLength)) {
    ESP_LOGE(TAG, "OTA begin failed");
    free(firmwareBuf);
    free(sigBuf);
    return OTA_UPDATE_FAIL_OTA_BEGIN;
  }

  size_t written = Update.write(firmwareBuf, contentLength);
  if (written != contentLength) {
    ESP_LOGE(TAG, "OTA write failed");
    free(firmwareBuf);
    free(sigBuf);
    Update.end(false);
    return OTA_UPDATE_FAIL_OTA_WRITE;
  }

  if (!Update.end(true)) {
    ESP_LOGE(TAG, "OTA end failed");
    free(firmwareBuf);
    free(sigBuf);
    return OTA_UPDATE_FAIL_OTA_END;
  }

  free(firmwareBuf);
  free(sigBuf);

  ESP_LOGI(TAG, "OTA Update complete. Restarting...");
  delay(1000);
  ESP.restart();

  return OTA_UPDATE_OK;
}

void try_update_controller() {
  delay(2000);
  const uint8_t max_retries = 5;
  for (uint8_t attempt = 0; attempt < max_retries; ++attempt) {
    uint8_t human_attempt = attempt + 1;
    ESP_LOGI(TAG, "OTA: try %u off %u\n", human_attempt, (uint16_t)(max_retries + 1));

    uint8_t result = update_controller();

    if (result == OTA_UPDATE_OK) {
      Serial.println("OTA: Done. Restart...");
      delay(1000);
      ESP.restart();
    }

    const uint32_t delay_ms = 1000u;
    if (attempt < max_retries) {
      Serial.printf("OTA: failure. Reintentando en %lums...\n", (unsigned long)delay_ms);
      delay(delay_ms);
    }
    else {
      Serial.printf("OTA: failure :,( ...");
    }
  }
}
