#include <gtest/gtest.h>

#include "lib/SevenSegment/SevenSegment.h"

using SevenSegment::segmentsForDigit;

// Segment bits: a=0x01 b=0x02 c=0x04 d=0x08 e=0x10 f=0x20 g=0x40
TEST(SegmentsForDigit, EncodesEachDigit) {
  EXPECT_EQ(segmentsForDigit(0), 0x3F);  // a b c d e f
  EXPECT_EQ(segmentsForDigit(1), 0x06);  // b c
  EXPECT_EQ(segmentsForDigit(2), 0x5B);  // a b d e g
  EXPECT_EQ(segmentsForDigit(3), 0x4F);  // a b c d g
  EXPECT_EQ(segmentsForDigit(4), 0x66);  // b c f g
  EXPECT_EQ(segmentsForDigit(5), 0x6D);  // a c d f g
  EXPECT_EQ(segmentsForDigit(6), 0x7D);  // a c d e f g
  EXPECT_EQ(segmentsForDigit(7), 0x07);  // a b c
  EXPECT_EQ(segmentsForDigit(8), 0x7F);  // all
  EXPECT_EQ(segmentsForDigit(9), 0x6F);  // a b c d f g
}

TEST(SegmentsForDigit, EightLightsEverySegment) {
  // Every segment used by any digit must be present in 8.
  for (int d = 0; d <= 9; ++d) {
    EXPECT_EQ(segmentsForDigit(d) & segmentsForDigit(8), segmentsForDigit(d)) << "digit " << d;
  }
}

TEST(SegmentsForDigit, OutOfRangeIsBlank) {
  EXPECT_EQ(segmentsForDigit(-1), 0x00);
  EXPECT_EQ(segmentsForDigit(10), 0x00);
}
