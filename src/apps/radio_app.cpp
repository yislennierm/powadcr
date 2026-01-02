#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/radio_app.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_radio {

void enter(bool force) {
  ui::view::radioViewUpdate(tft, 0, 0, 0, "", true);
}

void update(int bufferPct, int volPct, int rssi, const String &stationName, bool force) {
  ui::view::radioViewUpdate(tft, bufferPct, volPct, rssi, stationName, force);
}

}  // namespace app_radio

#endif  // USE_TDISPLAY
