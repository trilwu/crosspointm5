#include <gtest/gtest.h>

#include "components/themes/slate/SlateTheme.h"

// Design principle 1: nothing tappable is smaller than 44 px. These are
// compile-time assertions so the floor cannot regress silently in a later edit.
static_assert(SlateMetrics::values.listRowHeight >= 44, "list rows must meet the touch floor");
static_assert(SlateMetrics::values.listWithSubtitleRowHeight >= 44, "subtitle rows must meet the touch floor");
static_assert(SlateMetrics::values.menuRowHeight >= 44, "menu rows must meet the touch floor");
static_assert(SlateMetrics::values.keyboardKeyHeight >= 44, "keyboard keys must meet the touch floor");

TEST(SlateMetrics, TappableRowsMeetTouchFloor) {
  EXPECT_GE(SlateMetrics::values.listRowHeight, 44);
  EXPECT_GE(SlateMetrics::values.listWithSubtitleRowHeight, 44);
  EXPECT_GE(SlateMetrics::values.menuRowHeight, 44);
  EXPECT_GE(SlateMetrics::values.keyboardKeyHeight, 44);
}

TEST(SlateMetrics, SubtitleRowsAreTallerThanPlainRows) {
  EXPECT_GT(SlateMetrics::values.listWithSubtitleRowHeight, SlateMetrics::values.listRowHeight);
}

// The home cover must fit above the menu on a 960 px tall portrait screen.
TEST(SlateMetrics, HomeCoverFitsPortraitHeight) {
  const int used = SlateMetrics::values.homeTopPadding + SlateMetrics::values.homeCoverTileHeight +
                   SlateMetrics::values.homeMenuTopOffset + SlateMetrics::values.menuRowHeight;
  EXPECT_LT(used, 960);
}
