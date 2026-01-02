#include "config.h"
#include "apps/prefs.h"
#include <Preferences.h>

namespace {
Preferences _prefs;
const char* NS = "powadcr";
}

namespace prefs {

bool init() {
  return _prefs.begin(NS, false);
}

bool wifiEnabled() {
  return _prefs.getBool("wifi_en", true);
}

void setWifiEnabled(bool enabled) {
  _prefs.putBool("wifi_en", enabled);
}

float volumePct() {
  return _prefs.getFloat("vol_pct", DEFAULT_VOLUME_PCT);
}

void setVolumePct(float pct) {
  pct = constrain(pct, 0.0f, 100.0f);
  _prefs.putFloat("vol_pct", pct);
}

int joyCenterX() {
  return _prefs.getInt("joy_cx", -1);
}

int joyCenterY() {
  return _prefs.getInt("joy_cy", -1);
}

int joyDeadzone() {
  return _prefs.getInt("joy_dz", JOY_DEADZONE);
}

void setJoyCalibration(int cx, int cy, int dz) {
  _prefs.putInt("joy_cx", cx);
  _prefs.putInt("joy_cy", cy);
  _prefs.putInt("joy_dz", dz);
}

}  // namespace prefs
