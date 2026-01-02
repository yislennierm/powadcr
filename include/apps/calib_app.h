#pragma once

#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/joystick.h"

namespace app_calib {

struct CalibData {
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
};

void enter(bool force = false);
void update(const CalibData& data, bool force = false);

}  // namespace app_calib
#endif  // USE_TDISPLAY
