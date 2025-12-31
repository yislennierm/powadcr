#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace ui {
namespace widgets {

// Draw a horizontal buffer bar filling the full screen width at the given y.
void drawBufferBar(TFT_eSPI &tft, int percent, int y = 60, int height = 8);

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
