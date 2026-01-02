#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/signal.h"

namespace ui {
namespace widgets {

void drawWifiBars(TFT_eSPI &tft, int rssi, int xRight, int y) {
  int bars = 0;
  if (rssi > -50) bars = 4;
  else if (rssi > -60) bars = 3;
  else if (rssi > -70) bars = 2;
  else if (rssi > -80) bars = 1;
  else bars = 0;

  int w = 3;
  int heights[4] = {3, 6, 9, 12};
  if (xRight < 0) xRight = tft.width();
  int x = xRight - (4 * (w + 1));

  tft.fillRect(x, y, 4 * (w + 1) + 1, 12, TFT_BLACK);
  for (int i = 0; i < 4; i++) {
    uint16_t col = (i < bars) ? TFT_GREEN : TFT_DARKGREY;
    int barH = heights[i];
    tft.fillRect(x + i * (w + 1), y + (12 - barH), w, barH, col);
  }
}

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
