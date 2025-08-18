#include "alarms.h"
#include "nmspc_out.h"
#include "EEPROM.h"

#define DISABLE_AUDIBLE_ALARM 

#ifdef DISABLE_AUDIBLE_ALARM
bool ee_audible_alarm = false;
#else 
bool ee_audible_alarm = true;
#endif


const uint8_t audible_alarm_bit = 3;
const uint8_t pinout_bits[6] = { 6, 5, 4, 0, 1, 2 };

// Empaqueta 6 bools en un uint8_t (alarms[0] -> bit 0 (LSB), alarms[5] -> bit 5)
uint8_t pack_alarms(const bool alarms[6]) {
  if (!alarms) return 0;
  uint8_t b = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    uint8_t bit = (static_cast<uint8_t>(alarms[i]) & 0x01u);
    b |= (bit << i);
  }
  return b & 0x3Fu;
}

// Desempaqueta uint8_t en bool[6] (bit 0 -> out[0], bit 5 -> out[5])
void unpack_alarms(uint8_t b, bool out[6]) {
  if (!out) return;
  b &= 0x3Fu;
  for (uint8_t i = 0; i < 6; ++i) {
    out[i] = ((b >> i) & 0x01u) != 0;
  }
}
void set_alarms(bool alarms[6]) {
  uint8_t erw_byte = pack_alarms(alarms);

  bool audible_alarm = false;
  for (uint8_t i = 0; i < 6; ++i) {
    outs::set_state(pinout_bits[i], alarms[i]);
    audible_alarm |= alarms[i];
  }

  if (ee_audible_alarm && audible_alarm) {
    outs::set_state(audible_alarm_bit, true);
  }
  else {
    outs::set_state(audible_alarm_bit, false);
  }

  //TODO: Codigo muy basura. *mejorar*
  //******** */
  EEPROM.begin(255);
  EEPROM.writeByte(2, erw_byte);
  EEPROM.commit();
  EEPROM.end();
  //******** */
}

void set_u8_alarms(uint8_t alarms) {
  bool data[6];
  bool audible_alarm = false;

  unpack_alarms(alarms, data);

  for (uint8_t i = 0; i < 6; i++) {
    audible_alarm |= data[i];
    outs::set_state(pinout_bits[i], data[i]);
  }

  if (ee_audible_alarm && audible_alarm) {
    outs::set_state(audible_alarm_bit, true);
  }
  else {
    outs::set_state(audible_alarm_bit, false);
  }
}

void reset_alarms() {
  for (uint8_t i = 0; i < 7; i++) {
    outs::set_state(i, false);
  }

  //TODO: Codigo muy basura. *mejorar*
  //******** */
  EEPROM.begin(255);
  EEPROM.writeByte(2, 0);
  EEPROM.commit();
  EEPROM.end();
  //******** */
}