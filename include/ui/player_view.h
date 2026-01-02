#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include <Arduino.h>

namespace ui {
namespace view {

struct PlayerInfo {
  String title;
  int bufferPct;
  int volPct;
  int rssi;
  bool paused = false;
  String status;
};

// Player screen (tape/volume/status)
void playerViewUpdate(TFT_eSPI &tft, const PlayerInfo &info, bool force=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY

