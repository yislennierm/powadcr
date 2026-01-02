#include "config.h"
#if USE_TDISPLAY
#include <TFT_eSPI.h>
#include "apps/explorer_app.h"
#include "ui/explorer_view.h"
#if USE_TDISPLAY
extern TFT_eSPI tft;
extern TFT_eSprite background;
#endif

namespace app_explorer {

void enter(bool force) {
  ExplorerState st{};
  st.status = "Select";
  st.title = "EXPLORER";
  ui::view::ExplorerInfo info{};
  info.status = st.status;
  info.title = st.title;
  ui::view::explorerViewUpdate(tft, info, true);
}

void update(const ExplorerState& st, bool force) {
  ui::view::ExplorerInfo info{};
  info.entryCount = st.count;
  info.selected = st.selected;
  info.status = st.status;
  info.title = st.title;
  for (int i = 0; i < min(st.count, 8); ++i) info.entries[i] = st.entries[i];
  ui::view::explorerViewUpdate(tft, info, force);
}

}  // namespace app_explorer

#endif  // USE_TDISPLAY
