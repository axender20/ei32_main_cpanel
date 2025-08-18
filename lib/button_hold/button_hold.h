#pragma once
#ifndef _BUTTON_HOLD_H_
#define _BUTTON_HOLD_H_

#include <Arduino.h>

class ButtonHold {
public:
  enum ContactType { NO = 0, NC = 1 };

  // pin, tiempo requerido en ms, modoPin (INPUT, INPUT_PULLUP, INPUT_PULLDOWN),
  // tipo de contacto (NO/NC), debounce en ms
  ButtonHold(uint8_t pin,
             unsigned long timeHoldMs,
             uint8_t modoPin = INPUT_PULLUP,
             ContactType contact = NO,
             unsigned long debounceMs = 20)
    : _pin(pin),
      _time_req(timeHoldMs),
      _mode_pin(modoPin),
      _contact(contact),
      _debounce_ms(debounceMs),
      _pressed(false),
      _already_triggered(false),
      _init_time(0),
      _last_raw(LOW),
      _stable_state(LOW),
      _last_change(0),
      _active_state(HIGH)
  {}

  // Inicializar (llamar en setup)
  void begin() {
    pinMode(_pin, _mode_pin);
    // Lectura inicial
    _last_raw = digitalRead(_pin);
    _stable_state = _last_raw;
    _last_change = millis();

    // Determinar estado lógico "activo" (nivel que representa PRESIONADO)
    // Basado en modo (pullup/pulldown/INPUT) y tipo de contacto (NO/NC)
    if (_mode_pin == INPUT_PULLUP) {
      // Pull-up: NO -> PRESionado = LOW, NC -> PRESionado = HIGH
      _active_state = (_contact == NO) ? LOW : HIGH;
    }
#ifdef INPUT_PULLDOWN
    else if (_mode_pin == INPUT_PULLDOWN) {
      // Pull-down: NO -> PRESionado = HIGH, NC -> PRESionado = LOW
      _active_state = (_contact == NO) ? HIGH : LOW;
    }
#endif
    else {
      // INPUT sin pull interno. Asumimos lógica estándar: NO -> PRESionado = HIGH
      // Si tu hardware usa otra cosa (resistencia externa), puedes ajustar con setContactType() o setModePin().
      _active_state = (_contact == NO) ? HIGH : LOW;
    }
  }

  // Cambiar tiempo requerido después de iniciar
  void setTimeHold(unsigned long timeMs) { _time_req = timeMs; }

  // Cambiar debounce
  void setDebounce(unsigned long debounceMs) { _debounce_ms = debounceMs; }

  // Cambiar tipo de contacto (NO / NC) en tiempo de ejecución
  void setContactType(ContactType contact) {
    _contact = contact;
    // recalcular estado activo para nuevas condiciones
    if (_mode_pin == INPUT_PULLUP) {
      _active_state = (_contact == NO) ? LOW : HIGH;
    }
#ifdef INPUT_PULLDOWN
    else if (_mode_pin == INPUT_PULLDOWN) {
      _active_state = (_contact == NO) ? HIGH : LOW;
    }
#endif
    else {
      _active_state = (_contact == NO) ? HIGH : LOW;
    }
  }

  // Cambiar modo pin (repite la lógica de begin sin tocar pinMode automáticamente)
  void setModePin(uint8_t modoPin) {
    _mode_pin = modoPin;
    // recalcular _active_state
    setContactType(_contact);
  }

  // Resetea el estado interno (útil si quieres reiniciar la detección)
  void reset() {
    _pressed = false;
    _already_triggered = false;
    _init_time = 0;
    _last_raw = digitalRead(_pin);
    _stable_state = _last_raw;
    _last_change = millis();
  }

  // Retorna true solo UNA vez cuando se mantiene presionado el tiempo requerido
  // Debe llamarse periódicamente (loop)
  bool pressHold() {
    int stable = readDebounced(); // lectura con debounce

    bool is_active = (stable == _active_state);

    if (is_active) {
      if (!_pressed) {
        // nueva pulsación establecida
        _pressed = true;
        _already_triggered = false;
        _init_time = millis();
      } else if (!_already_triggered && (millis() - _init_time >= _time_req)) {
        // se cumple el tiempo de "hold" y aún no habíamos disparado
        _already_triggered = true;
        return true;
      }
    } else {
      // liberado -> resetear para siguiente ciclo
      _pressed = false;
      _already_triggered = false;
      _init_time = 0;
    }
    return false;
  }

  // Para debugging / lecturas directas (estado lógico presionado sin debounce)
  bool rawIsActive() const {
    return (digitalRead(_pin) == _active_state);
  }

  // Devuelve si actualmente se considera "presionado" (estado estable con debounce)
  bool isPressed() {
    return (readDebounced() == _active_state);
  }

private:
  uint8_t _pin;
  unsigned long _time_req;
  uint8_t _mode_pin;
  ContactType _contact;
  unsigned long _debounce_ms;

  // Estado interno para pressHold()
  bool _pressed;
  bool _already_triggered;
  unsigned long _init_time;

  // Debounce
  int _last_raw;
  int _stable_state;
  unsigned long _last_change;

  int _active_state; // HIGH o LOW que representa PRESIONADO

  // Lectura con debounce (no bloqueante)
  int readDebounced() {
    int raw = digitalRead(_pin);
    unsigned long now = millis();

    if (raw != _last_raw) {
      // cambio detectado, reiniciar temporizador
      _last_change = now;
      _last_raw = raw;
    }

    if ((now - _last_change) >= _debounce_ms) {
      // suficiente tiempo estable -> aceptar nuevo estado
      if (raw != _stable_state) {
        _stable_state = raw;
      }
    }
    return _stable_state;
  }
};

#endif
