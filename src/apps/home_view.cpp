#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/home_view.h"
#if USE_TDISPLAY
extern TFT_eSprite background;
extern TFT_eSPI tft;
#endif

namespace ui {
namespace view {

void homeViewUpdate(TFT_eSPI &tft, const HomeInfo &info, bool force) {
  static bool init = false;
  static HomeInfo prev{};
  TFT_eSprite* draw = &background;

  if (!init || force) {
    draw->fillSprite(TFT_BLACK);
    draw->setCursor(0, 0);
    draw->setTextColor(TFT_CYAN, TFT_BLACK);
    draw->setTextSize(2);
    draw->println("HOME");
    draw->setTextSize(1);
    init = true;
    prev = HomeInfo{}; // reset cache
  }

  auto clearLine = [&](int y) { draw->fillRect(0, y, tft.width(), 10, TFT_BLACK); };
  int y = 18;
  draw->setTextSize(1);
  draw->setTextColor(TFT_WHITE, TFT_BLACK);

  if (info.stationName != prev.stationName) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("Station: %s", info.stationName.c_str());
    prev.stationName = info.stationName;
  }
  y += 10;

  if (info.bufferPct != prev.bufferPct || info.volPct != prev.volPct) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("Vol: %d%% Buf: %d%%", info.volPct, info.bufferPct);
    prev.bufferPct = info.bufferPct;
    prev.volPct = info.volPct;
  }
  y += 10;

  if (info.wifiConnected != prev.wifiConnected || info.ssid != prev.ssid) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("WiFi: %s", info.wifiConnected ? info.ssid.c_str() : "disconnected");
    prev.wifiConnected = info.wifiConnected;
    prev.ssid = info.ssid;
  }
  y += 10;
  if (info.wifiConnected && info.ip != prev.ip) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("IP: %s", info.ip.c_str());
    prev.ip = info.ip;
  }
  y += 10;
  if (info.wifiConnected && info.rssi != prev.rssi) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("RSSI: %d dBm", info.rssi);
    prev.rssi = info.rssi;
  }
  y += 10;

  if (info.sdMounted != prev.sdMounted || info.sdUsedMB != prev.sdUsedMB || info.sdTotalMB != prev.sdTotalMB) {
    clearLine(y);
    if (info.sdMounted) {
      draw->setCursor(0, y); draw->printf("SD: %.1f/%.1f MB", info.sdUsedMB, info.sdTotalMB);
    } else {
      draw->setCursor(0, y); draw->print("SD: not mounted");
    }
    prev.sdMounted = info.sdMounted;
    prev.sdUsedMB = info.sdUsedMB;
    prev.sdTotalMB = info.sdTotalMB;
  }
  y += 10;

  if (info.stationCount != prev.stationCount) {
    clearLine(y);
    draw->setCursor(0, y); draw->printf("Stations: %u", (unsigned)info.stationCount);
    prev.stationCount = info.stationCount;
  }
  y += 10;

  clearLine(y);
  draw->setCursor(0, y); draw->print("Hold + to switch");

  draw->pushSprite(0, 0);
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
