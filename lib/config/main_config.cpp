#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "main_config.h"

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

static const char* TAG = "CFG";

Config cnfg;
static const char* default_ssid = "INNOVA OCCIDENTE S.A";
static const char* default_pass = "Innova2025";

static const uint8_t memory_size = 255u;
static const uint8_t validation_byte = 0x3E;


enum map_memory : uint8_t {
  xmp_validation = 0,
  xmp_new_config = 1,
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
    EEPROM.writeByte(xmp_new_config, validation_byte);
    EEPROM.writeString(xmp_ssid, String(default_ssid));
    EEPROM.writeString(xmp_pass, String(default_pass));
    EEPROM.commit();

    ESP_LOGI(TAG, "Reboot required...");
    delay(1000);
    ESP.restart();
  }

  uint8_t new_config = EEPROM.readByte(xmp_new_config);
  if(new_config == validation_byte){
    //Create new config
  }

  String i_pass = EEPROM.readString(xmp_pass);
  String i_ssid = EEPROM.readString(xmp_ssid);

  cnfg.update_parametres(i_ssid.c_str(), i_pass.c_str());
  ESP_LOGI(TAG, "Configuration loaded successfully");
}

void update_config(Config _cfg) {
  //> Only update config, external Reboot required
  ESP_LOGI(TAG, "Write new config");

  ESP_LOGI(TAG, "Clear data");
  uint8_t clear_value = 0xff;
  for (uint8_t i = 0; i < memory_size; i++) {
    EEPROM.writeByte(i, clear_value);
  }

  EEPROM.writeByte(xmp_validation, validation_byte);
  EEPROM.writeString(xmp_ssid, String(_cfg.get_ssid()));
  EEPROM.writeString(xmp_pass, String(_cfg.get_pass()));
  EEPROM.commit();

  ESP_LOGI(TAG, "New data writed (a reboot is still necessary)");
}
