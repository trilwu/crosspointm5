#include <gtest/gtest.h>

#include "components/themes/slate/SlateTheme.h"

// v2 targets comfortable phone-scale rows, not the 44px accessibility floor.
static_assert(SlateMetrics::values.listRowHeight >= 60, "list rows must be comfortable, not minimal");
static_assert(SlateMetrics::values.listWithSubtitleRowHeight >= 76, "subtitle rows must be comfortable");
static_assert(SlateMetrics::values.menuRowHeight >= 64, "menu rows must be comfortable");
static_assert(SlateMetrics::values.keyboardKeyHeight >= 52, "keyboard keys must meet the touch floor");

TEST(SlateMetrics, TappableRowsAreComfortable) {
  EXPECT_GE(SlateMetrics::values.listRowHeight, 60);
  EXPECT_GE(SlateMetrics::values.listWithSubtitleRowHeight, 76);
  EXPECT_GE(SlateMetrics::values.menuRowHeight, 64);
  EXPECT_GE(SlateMetrics::values.keyboardKeyHeight, 52);
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
