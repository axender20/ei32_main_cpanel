#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

#include "Arduino.h"
#include "new_network_config.h"
#include "rgb_led.h"
#include "WiFiManager.h"
#include "button_hold.h"
#include "EEPROM.h"
#include "ei32_cpanel_pinout.h"

static const char* TAG = "NNET";
static WiFiManager wm;
static bool wm_exit = false;

void exit_network_config() {
  //TODO: mejorar la arquitectura, codigo algo mierdas
//**--------------------- */
  EEPROM.begin(255);
  uint8_t xmp_enter_nc = 1;
  EEPROM.writeByte(xmp_enter_nc, 0);
  EEPROM.commit();
  EEPROM.end();
  //**--------------------- */

  wrgb_1.switch_color(255, 0, 0);
  ESP_LOGI(TAG, "Exit reset...");
  delay(2000);

  ESP.restart();
}

void saveParamsCallback() {
  wrgb_1.switch_color(0, 255, 0);

  ESP_LOGI(TAG, "Correct new coonect");
  ESP_LOGI(TAG, "New SSID: %s", wm.getWiFiSSID());
  ESP_LOGI(TAG, "New PASS: %s", wm.getWiFiPass());
  wm_exit = true;
  delay(1000);
}

void errCantConnectCallback() {
  exit_network_config();
}

void setParametresCallback() {
  //? Try conneccto
  wrgb_1.switch_color(0, 0, 255);
}

bool enter_nconfig() {
  bool result = false;
  wrgb_1.switch_color(0, 0, 255);

  wm.setDarkMode(true);

  //reset settings - wipe credentials for testing
  wm.resetSettings();

  wm.setConfigPortalBlocking(false);
  wm.setSaveConnectTimeout(20);
  wm.setSaveConfigCallback(saveParamsCallback);
  wm.setConfigPortalTimeoutCallback(errCantConnectCallback);
  wm.setSaveParamsCallback(setParametresCallback);
  //TODO: No hay callbak para cuando el controlador no se pudo conectar *Revisar eso*

  //Generate AP
  wm.autoConnect("EI32_CPANEL", "ei_pass@123"); // password protected ap

  ulong c_timer = millis();
  bool s_blink = true;

  ButtonHold button(pin_bconfig, 3000);
  button.begin();

  for (;;) {
    //> AP Process
    wm.process();
    if (wm_exit) break;

    //> Exit 
    if (button.pressHold()) {
      exit_network_config();
    }

    //> Blink
    if (millis() - c_timer >= 1000) {
      s_blink = !s_blink;

      if (s_blink) wrgb_1.on();
      else wrgb_1.off();
      c_timer = millis();
    }
  }
  return true;
}

String get_wm_ssid() {
  return wm.getWiFiSSID();
}

String get_wm_pass() {
  return wm.getWiFiPass();
}
