#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/volume.h"

namespace ui {
namespace widgets {

void drawVolumeArc(TFT_eSPI &tft, int percent, int xCenter, int yCenter) {
  percent = constrain(percent, 0, 100);
  if (xCenter < 0) xCenter = tft.width() - 30;
  int rOuter = 6;
  int rInner = 3;
  // Erase area behind the arc (cover full circle footprint)
  int boxSize = rOuter * 2 + 4;
  tft.fillRect(xCenter - rOuter - 2, yCenter - rOuter - 2, boxSize, boxSize, TFT_BLACK);
  // Map percent to sweep angle (0..360 degrees for full circle at 100%)
  int sweep = map(percent, 0, 100, 0, 360);
  tft.drawArc(xCenter, yCenter, rOuter, rInner, 0, sweep, TFT_CYAN, TFT_BLACK);
}

}  // namespace widgets
}  // namespace ui

#endif  // USE_TDISPLAY
