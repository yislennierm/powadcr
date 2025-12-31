#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace ui {
namespace widgets {

// Classic 4-level WiFi bars (stair-step).
void drawWifiBars(TFT_eSPI &tft, int rssi, int xRight = -1, int y = 0);

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
