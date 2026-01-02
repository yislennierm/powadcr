#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/toggle.h"

namespace ui {
namespace widgets {

void drawToggle(TFT_eSPI &tft, int x, int y, bool on, bool active) {
  int w = 34;
  int h = 16;
  int r = h / 2;
  uint16_t slotColor = active ? TFT_DARKGREY : TFT_LIGHTGREY;
  uint16_t knobColor = on ? TFT_GREEN : TFT_WHITE;
  uint16_t borderColor = on ? TFT_GREEN : TFT_WHITE;

  // Slot
  tft.fillRoundRect(x, y, w, h, r, slotColor);
  tft.drawRoundRect(x, y, w, h, r, borderColor);

  // Knob
  int knobX = on ? (x + w - h) : x;
  tft.fillRoundRect(knobX, y, h, h, r, knobColor);
  tft.drawRoundRect(knobX, y, h, h, r, borderColor);
}

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
