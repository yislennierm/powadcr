#pragma once

#include "config.h"

#if USE_TDISPLAY
#include <TFT_eSPI.h>

namespace app_explorer {

struct ExplorerState {
  String entries[8];
  String paths[8];
  int count = 0;
  int selected = 0;
  String status;
  String title;
};

void enter(bool force=false);
void update(const ExplorerState& st, bool force=false);

}  // namespace app_explorer

#endif  // USE_TDISPLAY

