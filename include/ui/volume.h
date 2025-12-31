#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace ui {
namespace widgets {

// Simple arc showing volume percent (0..100).
void drawVolumeArc(TFT_eSPI &tft, int percent, int xCenter = -1, int yCenter = 6);

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
