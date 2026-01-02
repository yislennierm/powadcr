#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/joystick.h"

namespace ui {
namespace view {

struct CalibInfo {
  int rawX;
  int rawY;
  ui::SimpleJoystick::Dir dir;
  bool swPressed;
  int swRaw;
  int centerX;
  int centerY;
  int minX;
  int maxX;
  int minY;
  int maxY;
  int calibDeadzone;
  bool calibrating;
  uint32_t calibElapsedMs;
  int deadzone;
  int maxAdc;
};

void calibViewUpdate(TFT_eSPI &tft, const CalibInfo &info, bool force=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
