#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/buffer.h"

namespace ui {
namespace widgets {

void drawBufferBar(TFT_eSPI &tft, int percent, int y, int height) {
  percent = constrain(percent, 0, 100);
  int fullW = tft.width();
  int barWidth = map(percent, 0, 100, 0, fullW);
  // only update the filled portion to reduce flicker
  tft.fillRect(0, y, barWidth, height, TFT_GREEN);
  tft.fillRect(barWidth, y, fullW - barWidth, height, TFT_DARKGREY);
  tft.drawRect(0, y, fullW, height, TFT_WHITE);
  tft.setCursor(0, y + height + 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.printf("Buf: %d%%", percent);
}

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
