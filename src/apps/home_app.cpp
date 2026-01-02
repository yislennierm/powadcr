#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/home_app.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_home {

void enter(bool force) {
  ui::view::HomeInfo empty{};
  ui::view::homeViewUpdate(tft, empty, true);
}

void update(const HomeData &data, bool force) {
  ui::view::homeViewUpdate(tft, data, force);
}

}  // namespace app_home

#endif  // USE_TDISPLAY
