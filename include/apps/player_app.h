#pragma once

#if USE_TDISPLAY
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ui/player_view.h"

namespace app_player {

using PlayerData = ui::view::PlayerInfo;

void enter(bool force = true);
void update(const PlayerData &data, bool force = false);

}  // namespace app_player

#endif  // USE_TDISPLAY
