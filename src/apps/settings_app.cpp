#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/settings_app.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_settings {

static ui::view::SettingsView sView = ui::view::SettingsView::LIST;
static int sSelection = 0;

void enter(bool force) {
  ui::view::SettingsState st{};
  st.view = sView = ui::view::SettingsView::LIST;
  st.selection = sSelection = 0;
  st.volPct = 0;
  st.wifi.wifiEnabled = false;
  st.wifi.connected = false;
  ui::view::settingsViewUpdate(tft, st, true);
}

void selectNext() {
  sSelection = (sSelection + 1) % 2;
}

void selectPrev() {
  sSelection = (sSelection - 1 + 2) % 2;
}

void toggleDetail() {
  if (sView == ui::view::SettingsView::LIST) {
    sView = (sSelection == 0) ? ui::view::SettingsView::WIFI : ui::view::SettingsView::BLUETOOTH;
  } else {
    sView = ui::view::SettingsView::LIST;
  }
}

ui::view::SettingsView currentView() {
  return sView;
}

void update(int volPct, bool wifiEnabled, bool wifiConnected, const String &ssid, const String &ip, const String &gw, const String &dns, bool force) {
  ui::view::SettingsState st{};
  st.view = sView;
  st.selection = sSelection;
  st.volPct = volPct;
  st.wifi.wifiEnabled = wifiEnabled;
  st.wifi.connected = wifiConnected;
  st.wifi.ssid = wifiConnected ? ssid : "";
  st.wifi.ip = wifiConnected ? ip : "";
  st.wifi.gw = wifiConnected ? gw : "";
  st.wifi.dns = wifiConnected ? dns : "";
  ui::view::settingsViewUpdate(tft, st, force);
}

}  // namespace app_settings

#endif  // USE_TDISPLAY
