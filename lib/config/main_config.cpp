#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "main_config.h"
#include "new_network_config.h"

bool is_all_spaces(const char* str) {
  while (*str) {
    if (!isspace(*str)) return false;
    str++;
  }
  return true;
}

bool Config::update_parametres(const char* _ssid, const char* _pass) {
  // Validar SSID
  if (_ssid == nullptr || strlen(_ssid) == 0 || is_all_spaces(_ssid)) {
    return false;
  }

  if (_pass == nullptr) {
    return false;
  }
  // Permitir contraseñas vacías, pero no espacios solamente
  if (strlen(_pass) > 0 && is_all_spaces(_pass)) {
    return false;
  }

  this->ssid = _ssid;
  this->pass = _pass;
  return true;
}

//* Main config definitions */
#include "EEPROM.h"
#include "Arduino.h"
#include "alarms.h"

static const char* TAG = "CFG";

Config cnfg;
static const char* default_ssid = "INNOVA OCCIDENTE S.A";
static const char* default_pass = "Innova2025";

static const uint8_t memory_size = 255u;
static const uint8_t validation_byte = 0x3E;


enum map_memory : uint8_t {
  xmp_validation = 0,
  xmp_enter_nc = 1,
  xmp_alarms = 2,
  xmp_ssid = 10,
  xmp_pass = 110,
};

void load_config() {
  EEPROM.begin(memory_size);

  uint8_t v_byte = EEPROM.readByte(xmp_validation);
  if (v_byte != validation_byte) {
    ESP_LOGI(TAG, "Re: write initial config");
    //? Error re-escribir informacion inicial.
    EEPROM.writeByte(xmp_validation, validation_byte);
    EEPROM.writeByte(xmp_enter_nc, 0);
    EEPROM.writeByte(xmp_alarms, 0);
    EEPROM.writeString(xmp_ssid, String(default_ssid));
    EEPROM.writeString(xmp_pass, String(default_pass));
    EEPROM.commit();

    ESP_LOGI(TAG, "Reboot required...");
    delay(1000);
    ESP.restart();
  }

  uint8_t new_config = EEPROM.readByte(xmp_enter_nc);

  //? Only debug
  bool force_nconfig = false;
  if (new_config == validation_byte || force_nconfig) {
    ESP_LOGI(TAG, "Enter n-config");

    //? Se logro obtener la nueva configuracion
    if (enter_nconfig()) {
      cnfg.update_parametres(get_wm_ssid().c_str(), get_wm_pass().c_str());
      update_config(cnfg);
      ESP_LOGI(TAG, "Update config, resart...");
      delay(1000);
      ESP.restart();
    }
  }

  String i_pass = EEPROM.readString(xmp_pass);
  String i_ssid = EEPROM.readString(xmp_ssid);
  uint8_t i_alarms = EEPROM.readByte(xmp_alarms);
  cnfg.update_parametres(i_ssid.c_str(), i_pass.c_str());
  set_u8_alarms(i_alarms);
  ESP_LOGI(TAG, "Configuration loaded successfully");
  EEPROM.end();
}

void update_config(Config _cfg) {
  //> Only update config, external Reboot required
  ESP_LOGI(TAG, "Write new config");
  EEPROM.begin(memory_size);

  ESP_LOGI(TAG, "Clear data");
  uint8_t clear_value = 0xff;
  for (uint8_t i = 0; i < memory_size; i++) {
    EEPROM.writeByte(i, clear_value);
  }

  EEPROM.writeByte(xmp_validation, validation_byte);
  EEPROM.writeByte(xmp_enter_nc, 0);
  EEPROM.writeByte(xmp_alarms, 0);
  EEPROM.writeString(xmp_ssid, String(_cfg.get_ssid()));
  EEPROM.writeString(xmp_pass, String(_cfg.get_pass()));
  EEPROM.commit();
  EEPROM.end();
  ESP_LOGI(TAG, "New data writed (a reboot is still necessary)");
}

void generate_new_config() {
  EEPROM.begin(memory_size);
  EEPROM.writeByte(xmp_enter_nc, validation_byte);
  EEPROM.commit();
  EEPROM.end();
  ESP_LOGI(TAG, "Generate new config, resart...");
  ESP.restart();
}
