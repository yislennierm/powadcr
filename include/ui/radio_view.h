#pragma once

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include <Arduino.h>

namespace ui {
namespace view {

// Render the radio status (buffer bar, WiFi bars, volume arc, station name).
// Uses internal caching to minimize redraw. Set force=true to clear caches and redraw fully.
void radioViewUpdate(TFT_eSPI &tft, int bufferPct, int volPct, int rssi, const String &stationName, bool force=false);

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
