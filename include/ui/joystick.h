#pragma once

#include <Arduino.h>
#include <functional>

namespace ui {

// Simple analog joystick helper: reads X/Y ADC pins, applies deadzone, emits directions.
class SimpleJoystick {
public:
  enum class Dir { Center, Up, Down, Left, Right };
  using Callback = std::function<void(Dir)>;

  SimpleJoystick(int pinX, int pinY, int deadzone = 200, int maxAdc = 4095);

  void begin();
  void setDeadzone(int dz);
  void setCenter(int cx, int cy);
  void setRepeatMs(uint32_t ms);
  void onDirection(Callback cb);

  // Poll joystick; returns current direction.
  Dir sample();
  // Call often; fires callback on direction change or repeat timeout.
  void tick();

private:
  int _pinX;
  int _pinY;
  int _deadzone;
  int _maxAdc;
  int _centerX;
  int _centerY;
  Callback _cb;
  Dir _lastDir = Dir::Center;
  uint32_t _lastEventMs = 0;
  uint32_t _repeatMs = 250; // ms between repeats while held
};

// Simple analog switch helper (for joystick SW read as ADC)
class AnalogSwitch {
public:
  using ClickCb = std::function<void(void)>;

  AnalogSwitch(int pin, int pressThreshold = 500, int maxAdc = 4095);

  void begin();
  void setDebounceMs(uint32_t ms);
  void setClickMs(uint32_t ms);
  void setLongPressMs(uint32_t ms);
  void onClick(ClickCb cb);
  void onDoubleClick(ClickCb cb);
  void onLongPress(ClickCb cb);

  void tick();

private:
  int _pin;
  int _threshold;
  int _maxAdc;
  bool _lastState = false;
  uint32_t _lastChange = 0;
  uint32_t _lastRelease = 0;
  uint8_t _clickCount = 0;
  uint32_t _debounceMs = 30;
  uint32_t _clickMs = 700;
  uint32_t _longMs = 1200;
  uint32_t _pressedAt = 0;
  ClickCb _cbClick;
  ClickCb _cbDouble;
  ClickCb _cbLong;
};

}  // namespace ui
