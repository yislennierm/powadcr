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
  return _prefs.getFloat("vol_pct", 90.0f);
}

void setVolumePct(float pct) {
  pct = constrain(pct, 0.0f, 100.0f);
  _prefs.putFloat("vol_pct", pct);
}

}  // namespace prefs
