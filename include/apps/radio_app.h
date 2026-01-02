#pragma once

#if USE_TDISPLAY
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ui/radio_view.h"

namespace app_radio {

void enter(bool force = true);
void update(int bufferPct, int volPct, int rssi, const String &stationName, bool force = false);

}  // namespace app_radio

#endif  // USE_TDISPLAY
