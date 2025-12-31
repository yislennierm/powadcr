#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace ui {
namespace widgets {

// Draw a single-line status message (small text) near the bottom-left.
// If y < 0, it auto-places near the bottom.
void drawStatusLine(TFT_eSPI &tft, const String &msg, int y = -1, uint16_t fg = TFT_WHITE, uint16_t bg = TFT_BLACK);

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
