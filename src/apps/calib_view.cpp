#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/calib_view.h"
#include "ui/widgets.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace ui {
namespace view {

static const char* dirToStr(SimpleJoystick::Dir d) {
  switch (d) {
    case SimpleJoystick::Dir::Up: return "Up";
    case SimpleJoystick::Dir::Down: return "Down";
    case SimpleJoystick::Dir::Left: return "Left";
    case SimpleJoystick::Dir::Right: return "Right";
    default: return "Center";
  }
}

void calibViewUpdate(TFT_eSPI &tft, const CalibInfo &info, bool force) {
  static bool init = false;
  TFT_eSprite* draw = &background;

  if (!init || force) {
    draw->fillSprite(TFT_BLACK);
    init = true;
  }

  draw->fillRect(0, 0, draw->width(), draw->height(), TFT_BLACK);
  draw->setTextColor(TFT_WHITE, TFT_BLACK);
  draw->setTextSize(1);
  draw->setCursor(0, 0);
  draw->println("CALIBRATION");
  draw->printf("X: %4d  Y: %4d\n", info.rawX, info.rawY);
  draw->printf("Dir: %s\n", dirToStr(info.dir));
  draw->printf("SW: %s (%d)\n", info.swPressed ? "Pressed" : "Released", info.swRaw);
  draw->printf("Deadzone: %d  MaxADC: %d\n", info.deadzone, info.maxAdc);
  draw->println("Adjust JOY_DEADZONE/JOY_REPEAT_MS in config.h");
  draw->printf("Calib center: %d,%d dz: %d\n", info.centerX, info.centerY, info.calibDeadzone);
  draw->printf("Min/Max X: %d/%d  Y: %d/%d\n", info.minX, info.maxX, info.minY, info.maxY);
  draw->printf("Calibrating: %s  t=%lu ms\n", info.calibrating ? "YES" : "NO", (unsigned long)info.calibElapsedMs);

  // Simple bar visualization
  int w = draw->width() - 20;
  int h = 10;
  int cx = w / 2;
  // X bar
  int xNorm = map(info.rawX, 0, info.maxAdc, 0, w);
  draw->drawRect(10, 80, w, h, TFT_DARKGREY);
  draw->fillRect(10 + xNorm - 2, 80, 4, h, TFT_CYAN);
  // Y bar
  int yNorm = map(info.rawY, 0, info.maxAdc, 0, w);
  draw->drawRect(10, 100, w, h, TFT_DARKGREY);
  draw->fillRect(10 + yNorm - 2, 100, 4, h, TFT_ORANGE);

  draw->pushSprite(0, 0);
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
