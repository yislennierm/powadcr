#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include <Arduino.h>

namespace ui {
namespace view {

struct ExplorerInfo {
  String title;
  String entries[8];
  int entryCount = 0;
  int selected = 0;
  String status;
};

void explorerViewUpdate(TFT_eSPI &tft, const ExplorerInfo &info, bool force=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY

