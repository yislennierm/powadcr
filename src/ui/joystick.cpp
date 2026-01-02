#include "ui/joystick.h"

namespace ui {

SimpleJoystick::SimpleJoystick(int pinX, int pinY, int deadzone, int maxAdc)
    : _pinX(pinX), _pinY(pinY), _deadzone(deadzone), _maxAdc(maxAdc),
      _centerX(maxAdc/2), _centerY(maxAdc/2) {}

void SimpleJoystick::begin() {
  pinMode(_pinX, INPUT);
  pinMode(_pinY, INPUT);
}

void SimpleJoystick::setDeadzone(int dz) { _deadzone = dz; }
void SimpleJoystick::setCenter(int cx, int cy) { _centerX = cx; _centerY = cy; }

void SimpleJoystick::setRepeatMs(uint32_t ms) { _repeatMs = ms; }

void SimpleJoystick::onDirection(Callback cb) { _cb = std::move(cb); }

SimpleJoystick::Dir SimpleJoystick::sample() {
  int x = analogRead(_pinX);
  int y = analogRead(_pinY);
  int dx = x - _centerX;
  int dy = y - _centerY;

  // Deadzone check
  if (abs(dx) < _deadzone && abs(dy) < _deadzone) {
    return Dir::Center;
  }

  // Decide dominant axis
  if (abs(dx) > abs(dy)) {
    return (dx > 0) ? Dir::Right : Dir::Left;
  } else {
    return (dy > 0) ? Dir::Down : Dir::Up;
  }
}

void SimpleJoystick::tick() {
  Dir dir = sample();
  uint32_t now = millis();
  bool changed = (dir != _lastDir);
  bool repeat = (!changed && dir != Dir::Center && (now - _lastEventMs) >= _repeatMs);

  if ((changed || repeat) && _cb) {
    _cb(dir);
    _lastEventMs = now;
  }
  _lastDir = dir;
}

// AnalogSwitch
AnalogSwitch::AnalogSwitch(int pin, int pressThreshold, int maxAdc)
  : _pin(pin), _threshold(pressThreshold), _maxAdc(maxAdc) {}

void AnalogSwitch::begin() {
  pinMode(_pin, INPUT);
}

void AnalogSwitch::setDebounceMs(uint32_t ms) { _debounceMs = ms; }
void AnalogSwitch::setClickMs(uint32_t ms) { _clickMs = ms; }
void AnalogSwitch::setLongPressMs(uint32_t ms) { _longMs = ms; }

void AnalogSwitch::onClick(ClickCb cb) { _cbClick = std::move(cb); }
void AnalogSwitch::onDoubleClick(ClickCb cb) { _cbDouble = std::move(cb); }
void AnalogSwitch::onLongPress(ClickCb cb) { _cbLong = std::move(cb); }

void AnalogSwitch::tick() {
  int v = analogRead(_pin);
  bool pressed = (v < _threshold); // active low
  uint32_t now = millis();

  // Debounce state changes
  if (pressed != _lastState && (now - _lastChange) >= _debounceMs) {
    _lastChange = now;
    _lastState = pressed;
    if (pressed) {
      _pressedAt = now;
    } else {
      // released
      uint32_t pressDuration = now - _pressedAt;
      if (pressDuration >= _longMs) {
        if (_cbLong) _cbLong();
        _clickCount = 0;
      } else {
        _clickCount++;
        _lastRelease = now;
        if (_clickCount == 2) {
          if (_cbDouble) _cbDouble();
          _clickCount = 0;
        }
      }
    }
  }

  // Single-click timeout
  if (!pressed && _clickCount == 1 && (now - _lastRelease) > _clickMs) {
    if (_cbClick) _cbClick();
    _clickCount = 0;
  }
}

}  // namespace ui
