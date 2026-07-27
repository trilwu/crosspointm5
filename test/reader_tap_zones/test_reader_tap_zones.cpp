#include <gtest/gtest.h>

#include "lib/ReaderTapZones/ReaderTapZones.h"

using ReaderTapZones::Action;
using ReaderTapZones::actionForX;

// Portrait is the M5Paper S3's default logical orientation (540 wide).
TEST(ActionForX, PortraitLeftThirdIsPrevious) {
  EXPECT_EQ(actionForX(0, 540), Action::Previous);
  EXPECT_EQ(actionForX(179, 540), Action::Previous);
}

TEST(ActionForX, PortraitCenterThirdIsNeutral) {
  EXPECT_EQ(actionForX(180, 540), Action::None);
  EXPECT_EQ(actionForX(359, 540), Action::None);
}

TEST(ActionForX, PortraitRightThirdIsNext) {
  EXPECT_EQ(actionForX(360, 540), Action::Next);
  EXPECT_EQ(actionForX(539, 540), Action::Next);
}

// The reader can be rotated by user setting, so the same split must hold at 960.
TEST(ActionForX, RotatedLeftThirdIsPrevious) {
  EXPECT_EQ(actionForX(0, 960), Action::Previous);
  EXPECT_EQ(actionForX(319, 960), Action::Previous);
}

TEST(ActionForX, RotatedCenterThirdIsNeutral) {
  EXPECT_EQ(actionForX(320, 960), Action::None);
  EXPECT_EQ(actionForX(639, 960), Action::None);
}

TEST(ActionForX, RotatedRightThirdIsNext) {
  EXPECT_EQ(actionForX(640, 960), Action::Next);
  EXPECT_EQ(actionForX(959, 960), Action::Next);
}

TEST(ActionForX, OutOfRangeIsNeutral) {
  EXPECT_EQ(actionForX(-1, 540), Action::None);
  EXPECT_EQ(actionForX(540, 540), Action::None);
  EXPECT_EQ(actionForX(10, 0), Action::None);
}

// The center zone must be genuinely reachable at every plausible width, including
// widths that do not divide evenly by three.
TEST(ActionForX, CenterZoneExistsAtAwkwardWidths) {
  for (const int w : {100, 271, 479, 540, 800, 960, 1001}) {
    bool sawNeutral = false;
    for (int x = 0; x < w; ++x) {
      if (actionForX(x, w) == Action::None) {
        sawNeutral = true;
        break;
      }
    }
    EXPECT_TRUE(sawNeutral) << "no neutral zone at width " << w;
  }
}
