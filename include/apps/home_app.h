#pragma once

#if USE_TDISPLAY
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ui/home_view.h"

namespace app_home {

using HomeData = ui::view::HomeInfo;

void enter(bool force = true);
void update(const HomeData &data, bool force = false);

}  // namespace app_home

#endif  // USE_TDISPLAY
