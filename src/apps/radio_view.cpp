#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "ui/widgets.h"
#include "ui/radio_view.h"
#if USE_TDISPLAY
extern TFT_eSprite background;
extern TFT_eSPI tft;
#endif

namespace ui {
namespace view {

void radioViewUpdate(TFT_eSPI &tft, int bufferPct, int volPct, int rssi, const String &stationName, bool force) {
  static int prevBuf = -1;
  static int prevVol = -1;
  static int prevRssi = 0;
  static String prevStation;
  TFT_eSprite* draw = &background;

  if (force) {
    prevBuf = -1;
    prevVol = -1;
    prevRssi = 0;
    prevStation = "";
    draw->fillSprite(TFT_BLACK);
  }

  // Buffer bar
  if (bufferPct != prevBuf) {
    ui::widgets::drawBufferBar(*draw, bufferPct);
    prevBuf = bufferPct;
  }

  // WiFi bars
  if (rssi != prevRssi) {
    ui::widgets::drawWifiBars(*draw, rssi);
    prevRssi = rssi;
  }

  // Volume arc
  if (volPct != prevVol) {
    ui::widgets::drawVolumeArc(*draw, volPct);
    prevVol = volPct;
  }

  // Station name (bottom status)
  if (stationName != prevStation) {
    ui::widgets::drawStatusLine(*draw, "Playing: " + stationName);
    prevStation = stationName;
  }

  draw->pushSprite(0, 0);
}

}  // namespace view
}  // namespace ui

#endif  // USE_TDISPLAY
