#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/player_view.h"
#include "ui/widgets.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace ui {
namespace view {

void playerViewUpdate(TFT_eSPI &tft, const PlayerInfo &info, bool force) {
  static bool init = false;
  static PlayerInfo prev{};
  static float angleDeg = 0.0f;
  static unsigned long lastDrawMs = 0;
  TFT_eSprite* draw = &background;

  bool changed = force || info.title != prev.title || info.bufferPct != prev.bufferPct ||
                 info.volPct != prev.volPct || info.rssi != prev.rssi || info.paused != prev.paused ||
                 info.status != prev.status;
  unsigned long nowMs = millis();
  if (!changed && (nowMs - lastDrawMs) < 150) {
    return;
  }

  if (!init || force) {
    draw->fillSprite(TFT_BLACK);
    init = true;
    prev = {};
  }

  draw->fillSprite(TFT_BLACK);
  draw->setTextColor(TFT_ORANGE, TFT_BLACK);
  draw->setTextSize(2);
  draw->setCursor(0, 0);
  draw->println(info.title);
  draw->setTextSize(1);

  // WiFi bars
  ui::widgets::drawWifiBars(*draw, info.rssi);
  // Volume arc
  ui::widgets::drawVolumeArc(*draw, info.volPct);
  // Status line
  ui::widgets::drawStatusLine(*draw, info.status.isEmpty() ? (info.paused ? "Paused" : "Playing") : info.status);

  // Cassette reel with custom hardcoded look
  const int kRimWidth = 4;
  const int kTeeth = 7;
  const int kToothLen = 3;
  int tapeOuter = 50;
  int tapeInner = 20;
  const uint16_t kTapeColor = tft.color565(170, 110, 60);
  const uint16_t kTapeColor2 = tft.color565(140, 90, 50);

  if (!info.paused) {
    angleDeg += 5.0f;
    if (angleDeg >= 360.0f) angleDeg -= 360.0f;
  }

  auto drawReelHalf = [&](int cx, int cy, int r, uint16_t bg) {
    draw->fillRect(max(0, cx - r), cy - r, r * 2, r * 2, bg);
    draw->drawArc(cx, cy, r, kRimWidth, 0, 360, TFT_LIGHTGREY, TFT_BLACK, true);
    draw->drawArc(cx, cy, r, kRimWidth, 0, 360, TFT_WHITE, TFT_BLACK, false);
    int innerR = r - kRimWidth;
    draw->fillCircle(cx, cy, innerR, TFT_BLACK);
    draw->drawCircle(cx, cy, innerR, TFT_WHITE);
    draw->drawArc(cx, cy, tapeOuter, tapeInner, 0, 360, kTapeColor, TFT_BLACK, true);
    draw->drawArc(cx, cy, 31, 30, 0, 360, kTapeColor2, TFT_BLACK, true);
    float step = 360.0f / kTeeth;
    for (int i = 0; i < kTeeth; ++i) {
      float a = (angleDeg + i * step) * DEG_TO_RAD;
      int baseR = innerR - kToothLen;
      int x1 = cx + int(cos(a) * baseR);
      int y1 = cy + int(sin(a) * baseR);
      int x2 = cx + int(cos(a) * innerR);
      int y2 = cy + int(sin(a) * innerR);
      draw->drawWideLine(x2, y2, x1, y1, 3, TFT_WHITE, TFT_BLACK);
      draw->fillRect(x2 - 1, y2 - 1, 3, 3, TFT_WHITE);
    }
  };

  int screenW = tft.width();
  int screenH = tft.height();
  int reelR = min(screenW, screenH) / 7;
  int reelCx = 0;
  int reelCy = screenH / 2;
  drawReelHalf(reelCx, reelCy, reelR, TFT_BLACK);

  draw->pushSprite(0, 0);
  prev = info;
  lastDrawMs = nowMs;
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
