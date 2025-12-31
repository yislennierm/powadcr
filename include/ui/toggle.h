#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace ui {
namespace widgets {

// Simple toggle switch: slot + circular knob.
// x,y is top-left of the slot.
void drawToggle(TFT_eSPI &tft, int x, int y, bool on, bool active = true);

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
