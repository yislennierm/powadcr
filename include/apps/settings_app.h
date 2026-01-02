#pragma once

#if USE_TDISPLAY
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ui/settings_view.h"

namespace app_settings {

void enter(bool force = true);
void selectNext();
void selectPrev();
void toggleDetail();

// Update settings UI with current data.
void update(int volPct, bool wifiEnabled, bool wifiConnected, const String &ssid, const String &ip, const String &gw, const String &dns, bool force = false);

// Expose current view for logic if needed
ui::view::SettingsView currentView();

}  // namespace app_settings

#endif  // USE_TDISPLAY
