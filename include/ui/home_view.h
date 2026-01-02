#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include <Arduino.h>

namespace ui {
namespace view {

struct HomeInfo {
  String stationName;
  int bufferPct;
  int volPct;
  bool wifiConnected;
  String ssid;
  String ip;
  int rssi;
  bool sdMounted;
  float sdUsedMB;
  float sdTotalMB;
  size_t stationCount;
};

// Render home dashboard; uses internal caching to avoid flicker. force=true resets cache and clears screen.
void homeViewUpdate(TFT_eSPI &tft, const HomeInfo &info, bool force=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
