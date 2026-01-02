#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/settings_view.h"
#if USE_TDISPLAY
extern TFT_eSprite background;
extern TFT_eSPI tft;
#endif

namespace ui {
namespace view {

void settingsViewUpdate(TFT_eSPI &tft, const SettingsState &state, bool forceRedraw) {
  static SettingsView prevView = SettingsView::LIST;
  static int prevSel = -1;
  static int prevVol = -1;
  static bool headerDrawn = false;
  static WifiDetail prevWifi{};
  TFT_eSprite* draw = &background;

  if (forceRedraw || !headerDrawn || state.view != prevView) {
    draw->fillSprite(TFT_BLACK);
    draw->setCursor(0, 0);
    draw->setTextColor(TFT_ORANGE, TFT_BLACK);
    draw->setTextSize(2);
    draw->println("SETTINGS");
    draw->setTextSize(1);
    headerDrawn = true;
    prevSel = -1;
    prevVol = -1;
    prevWifi = {};
  }

  if (state.view == SettingsView::LIST) {
    if (forceRedraw || state.selection != prevSel) {
      int y = 18;
      const char* items[] = {"WiFi", "Bluetooth"};
      for (int i = 0; i < 2; ++i) {
        uint16_t bg = (i == state.selection) ? TFT_DARKGREY : TFT_BLACK;
        uint16_t fg = (i == state.selection) ? TFT_WHITE : TFT_LIGHTGREY;
        draw->fillRect(0, y, tft.width(), 12, bg);
        if (i == state.selection) {
          draw->fillCircle(6, y + 5, 3, fg);
        }
        draw->setCursor(12, y + 2);
        draw->setTextColor(fg, bg);
        draw->print(items[i]);
        y += 12;
      }
      draw->fillRect(0, 18 + 2 * 12, tft.width(), 12, TFT_BLACK);
      draw->setTextColor(TFT_WHITE, TFT_BLACK);
      draw->setCursor(0, 18 + 2 * 12 + 2);
      draw->print("Long - to enter / exit");
      prevSel = state.selection;
    }
  } else {
    // Detail views
    bool wifiChanged = (state.wifi.connected != prevWifi.connected) ||
                       (state.wifi.ssid != prevWifi.ssid) ||
                       (state.wifi.ip != prevWifi.ip) ||
                       (state.wifi.gw != prevWifi.gw) ||
                       (state.wifi.dns != prevWifi.dns) ||
                       (state.wifi.wifiEnabled != prevWifi.wifiEnabled);
    if (forceRedraw || wifiChanged || state.view != prevView) {
      draw->fillRect(0, 18, tft.width(), tft.height() - 18, TFT_BLACK);
      int y = 18;
      draw->setTextColor(TFT_WHITE, TFT_BLACK);
      if (state.view == SettingsView::WIFI) {
        draw->fillRect(0, y, tft.width(), 16, TFT_BLACK);
        draw->setCursor(0, y + 2); draw->print("WiFi");
        int toggleX = tft.width() - 40;
        ui::widgets::drawToggle(*draw, toggleX, y, state.wifi.wifiEnabled, true);
        y += 22;
        if (state.wifi.wifiEnabled && state.wifi.connected) {
          draw->fillRect(0, y, tft.width(), 16, TFT_BLACK);
          draw->setCursor(0, y + 2); draw->print("IP: "); draw->print(state.wifi.ip);
          int ipToggleX = tft.width() - 40;
          ui::widgets::drawToggle(*draw, ipToggleX, y, true, true); // placeholder DHCP toggle
          y += 20;
          draw->fillRect(0, y, tft.width(), 16, TFT_BLACK);
          draw->setCursor(0, y + 2); draw->print("GW: "); draw->print(state.wifi.gw); 
          y += 20;
          draw->fillRect(0, y, tft.width(), 16, TFT_BLACK);
          draw->setCursor(0, y + 2); draw->print("DNS: "); draw->print(state.wifi.dns); 
          y += 20;
        } else if (state.wifi.wifiEnabled) {
          draw->setCursor(0, y); draw->print("WiFi enabled, not connected"); y += 14;
        } else {
          draw->setCursor(0, y); draw->print("WiFi disabled"); y += 14;
        }
        y += 6;
        draw->setCursor(0, y); draw->print("(Stub) Add edit/connect here"); y += 12;
      } else if (state.view == SettingsView::BLUETOOTH) {
        draw->setCursor(0, y); draw->print("Bluetooth Settings"); y += 12;
        draw->setCursor(0, y); draw->print("(Stub) add BT options"); y += 12;
      }
      prevWifi = state.wifi;
    }
    // Volume line at bottom
    if (state.volPct != prevVol) {
      prevVol = state.volPct;
      draw->fillRect(0, tft.height() - 12, tft.width(), 12, TFT_BLACK);
      draw->setCursor(0, tft.height() - 12);
      draw->setTextColor(TFT_CYAN, TFT_BLACK);
      draw->printf("Volume: %d%%", state.volPct);
    }
  }

  prevView = state.view;
  draw->pushSprite(0, 0);
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
