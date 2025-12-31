#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/status.h"

namespace ui {
namespace widgets {

void drawStatusLine(TFT_eSPI &tft, const String &msg, int y, uint16_t fg, uint16_t bg) {
  int lineHeight = 10; // small font height
  if (y < 0) {
    y = tft.height() - lineHeight - 2;
  }
  // clear the line area
  tft.fillRect(0, y, tft.width(), lineHeight + 2, bg);
  tft.setTextColor(fg, bg);
  tft.setTextSize(1);
  tft.setCursor(0, y + 1);
  tft.print(msg);
}

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
