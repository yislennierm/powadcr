#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/explorer_view.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace ui {
namespace view {

void explorerViewUpdate(TFT_eSPI &tft, const ExplorerInfo &info, bool force) {
  static bool init = false;
  static ExplorerInfo prev{};
  TFT_eSprite* draw = &background;

  bool changed = force || info.title != prev.title || info.entryCount != prev.entryCount ||
                 info.selected != prev.selected || info.status != prev.status;
  if (!changed && !force) return;

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
  draw->setTextColor(TFT_WHITE, TFT_BLACK);
  int rows = min(info.entryCount, 6);
  for (int i = 0; i < rows; ++i) {
    int y = 20 + i * 18;
    if (i == info.selected) {
      draw->fillRect(0, y - 2, draw->width(), 18, TFT_DARKGREY);
      draw->setTextColor(TFT_BLACK, TFT_DARKGREY);
    } else {
      draw->setTextColor(TFT_WHITE, TFT_BLACK);
    }
    draw->setCursor(4, y);
    draw->println(info.entries[i]);
  }
  draw->setTextColor(TFT_YELLOW, TFT_BLACK);
  draw->setCursor(0, draw->height() - 12);
  draw->println(info.status);

  draw->pushSprite(0, 0);
  prev = info;
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
