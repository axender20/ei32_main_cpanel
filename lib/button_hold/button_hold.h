#pragma once
#ifndef _BUTTON_HOLD_H_
#define _BUTTON_HOLD_H_

#include "Arduino.h"

class ButtonHold {
public:
  // Constructor: pin, tiempo requerido en ms, modoPin (INPUT, INPUT_PULLUP, INPUT_PULLDOWN)
  ButtonHold(uint8_t pin, unsigned long timeHoldMs, uint8_t modoPin = INPUT_PULLUP)
    : _pin(pin),
    _time_req(timeHoldMs),
    _mode_pin(modoPin),
    _pressed(false),
    _already_triggered(false),
    _init_time(0),
    _active_state(HIGH) {
  }

  void begin() {
    pinMode(_pin, _mode_pin);

    // Determinar estado activo según el modo
    if (_mode_pin == INPUT_PULLUP) {
      _active_state = LOW;   // Botón a GND → activo cuando lectura es LOW
    }
#ifdef INPUT_PULLDOWN
    else if (_mode_pin == INPUT_PULLDOWN) {
      _active_state = HIGH;  // Botón a VCC → activo cuando lectura es HIGH
    }
#endif
    else {
      _active_state = HIGH;  // INPUT normal → activo en HIGH
    }
  }

  // Cambiar tiempo requerido después de iniciar
  void setTimeHold(unsigned long timeMs) {
    _time_req = timeMs;
  }

  // Retorna true solo UNA vez cuando se mantiene presionado el tiempo requerido
  bool pressHold() {
    bool read_active = (digitalRead(_pin) == _active_state);

    if (read_active) {
      if (!_pressed) {
        _pressed = true;
        _already_triggered = false; // Nuevo ciclo de pulsación
        _init_time = millis();
      }
      else if (!_already_triggered && (millis() - _init_time >= _time_req)) {
        _already_triggered = true;  // Marcamos como ya ejecutado
        return true; // Dispara solo una vez por pulsación
      }
    }
    else {
      _pressed = false; // Resetea para próxima pulsación
    }
    return false;
  }

private:
  uint8_t _pin;
  unsigned long _time_req;
  uint8_t _mode_pin;
  bool _pressed;
  bool _already_triggered;
  unsigned long _init_time;
  int _active_state; // HIGH o LOW
};

#endif
