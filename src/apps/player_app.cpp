#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/player_app.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_player {

void enter(bool force) {
  ui::view::PlayerInfo empty{};
  empty.title = "Player";
  ui::view::playerViewUpdate(tft, empty, true);
}

void update(const PlayerData &data, bool force) {
  ui::view::playerViewUpdate(tft, data, force);
}

}  // namespace app_player

#endif  // USE_TDISPLAY
