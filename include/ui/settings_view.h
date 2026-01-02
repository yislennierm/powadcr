#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include <Arduino.h>
#include "ui/widgets.h"

namespace ui {
namespace view {

enum class SettingsView { LIST, WIFI, BLUETOOTH };

struct WifiDetail {
  bool wifiEnabled;
  bool connected;
  String ssid;
  String ip;
  String gw;
  String dns;
};

struct SettingsState {
  SettingsView view;
  int selection; // index in menu
  int volPct;
  WifiDetail wifi;
};

// Render settings UI; keeps internal cache to minimize redraw.
void settingsViewUpdate(TFT_eSPI &tft, const SettingsState &state, bool forceRedraw=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
