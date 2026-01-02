#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/calib_app.h"
#include "ui/calib_view.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_calib {

void enter(bool force) {
  ui::view::CalibInfo info{};
  info.rawX = info.rawY = 0;
  info.dir = ui::SimpleJoystick::Dir::Center;
  info.swPressed = false;
  info.deadzone = JOY_DEADZONE;
  info.maxAdc = 4095;
  ui::view::calibViewUpdate(tft, info, true);
}

void update(const CalibData& data, bool force) {
  ui::view::CalibInfo info{};
  info.rawX = data.rawX;
  info.rawY = data.rawY;
  info.dir = data.dir;
  info.swPressed = data.swPressed;
  info.swRaw = data.swRaw;
  info.centerX = data.centerX;
  info.centerY = data.centerY;
  info.minX = data.minX;
  info.maxX = data.maxX;
  info.minY = data.minY;
  info.maxY = data.maxY;
  info.calibDeadzone = data.calibDeadzone;
  info.calibrating = data.calibrating;
  info.calibElapsedMs = data.calibElapsedMs;
  info.deadzone = JOY_DEADZONE;
  info.maxAdc = 4095;
  ui::view::calibViewUpdate(tft, info, force);
}

}  // namespace app_calib

#endif  // USE_TDISPLAY
