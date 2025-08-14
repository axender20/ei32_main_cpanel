#include "alarms.h"
#include "nmspc_out.h"
#include "EEPROM.h"

bool enable_audible_alarm = true;

// Empaqueta 6 bools en un uint8_t (alarms[0] -> bit 0, alarms[5] -> bit 5)
uint8_t pack_alarms(const bool alarms[6]) {
  uint8_t b = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    if (alarms[i]) {
      b |= (1 << i);
    }
  }
  return b & 0x3F; // asegurar que solo 6 bits estén usados
}

// Desempaqueta uint8_t en bool[6]
void unpack_alarms(uint8_t b, bool out[6]) {
  b &= 0x3F; // solo 6 bits relevantes
  for (uint8_t i = 0; i < 6; ++i) {
    out[i] = ((b >> i) & 0x01) != 0;
  }
}

void set_alarms(bool alarms[6]) {
  // Empaquetar los 6 bools
  uint8_t byte = pack_alarms(alarms);

  // Aplicar salidas y calcular audible
  bool audible_alarm = false;
  for (uint8_t i = 0; i < 6; ++i) {
    outs::set_state(i, alarms[i]);
    audible_alarm |= alarms[i];
  }

  // Salida audible (reset cuando no hay alarma)
  if (enable_audible_alarm && audible_alarm) {
    outs::set_state(6, true);
  }
  else {
    outs::set_state(6, false);
  }

  //TODO: Codigo muy basura. *mejorar*
  //******** */
  EEPROM.begin(255);
  EEPROM.writeByte(2, byte);
  EEPROM.commit();
  EEPROM.end();
  //******** */
}

void set_u8_alarms(uint8_t alarms) {
  // Aceptar solo 6 bits (0..63). Si viene >63, lo mascaramos a 0..63
  alarms &= 0x3F;

  bool data[6];
  bool audible_alarm = false;

  // Desempaquetar y aplicar salidas
  for (uint8_t i = 0; i < 6; ++i) {
    data[i] = ((alarms >> i) & 0x01) != 0;
    audible_alarm |= data[i];
    outs::set_state(i, data[i]);
  }

  if (enable_audible_alarm && audible_alarm) {
    outs::set_state(6, true);
  }
  else {
    outs::set_state(6, false);
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
