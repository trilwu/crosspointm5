#include <gtest/gtest.h>

#include "lib/ClockMath/ClockMath.h"

using ClockMath::Date;

static Date mk(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi) { return Date{y, mo, d, h, mi}; }

TEST(ApplyOffsetMinutes, NoOffsetLeavesDateUnchanged) {
  Date d = mk(2026, 7, 26, 12, 30);
  ClockMath::applyOffsetMinutes(d, 0);
  EXPECT_EQ(d.year, 2026);
  EXPECT_EQ(d.month, 7);
  EXPECT_EQ(d.day, 26);
  EXPECT_EQ(d.hour, 12);
  EXPECT_EQ(d.minute, 30);
}

TEST(ApplyOffsetMinutes, PositiveOffsetRollsForwardOverMidnight) {
  // 2026-07-26 23:30 UTC + 7h (UTC+7) -> 2026-07-27 06:30
  Date d = mk(2026, 7, 26, 23, 30);
  ClockMath::applyOffsetMinutes(d, 7 * 60);
  EXPECT_EQ(d.day, 27);
  EXPECT_EQ(d.hour, 6);
  EXPECT_EQ(d.minute, 30);
}

TEST(ApplyOffsetMinutes, NegativeOffsetRollsBackOverMidnight) {
  // 2026-07-26 02:00 UTC - 5h -> 2026-07-25 21:00
  Date d = mk(2026, 7, 26, 2, 0);
  ClockMath::applyOffsetMinutes(d, -5 * 60);
  EXPECT_EQ(d.day, 25);
  EXPECT_EQ(d.hour, 21);
}

TEST(ApplyOffsetMinutes, RollsOverMonthEnd) {
  // 2026-07-31 23:00 +2h -> 2026-08-01 01:00
  Date d = mk(2026, 7, 31, 23, 0);
  ClockMath::applyOffsetMinutes(d, 2 * 60);
  EXPECT_EQ(d.month, 8);
  EXPECT_EQ(d.day, 1);
  EXPECT_EQ(d.hour, 1);
}

TEST(ApplyOffsetMinutes, RollsOverYearEnd) {
  // 2026-12-31 23:00 +2h -> 2027-01-01 01:00
  Date d = mk(2026, 12, 31, 23, 0);
  ClockMath::applyOffsetMinutes(d, 2 * 60);
  EXPECT_EQ(d.year, 2027);
  EXPECT_EQ(d.month, 1);
  EXPECT_EQ(d.day, 1);
}

TEST(ApplyOffsetMinutes, RollsBackwardOverYearStart) {
  // 2027-01-01 00:30 -1h -> 2026-12-31 23:30
  Date d = mk(2027, 1, 1, 0, 30);
  ClockMath::applyOffsetMinutes(d, -60);
  EXPECT_EQ(d.year, 2026);
  EXPECT_EQ(d.month, 12);
  EXPECT_EQ(d.day, 31);
  EXPECT_EQ(d.hour, 23);
}

TEST(ApplyOffsetMinutes, HandlesLeapDay) {
  // 2028 is a leap year: 2028-02-28 23:00 +2h -> 2028-02-29 01:00
  Date d = mk(2028, 2, 28, 23, 0);
  ClockMath::applyOffsetMinutes(d, 2 * 60);
  EXPECT_EQ(d.month, 2);
  EXPECT_EQ(d.day, 29);
}

TEST(ApplyOffsetMinutes, NonLeapYearSkipsFeb29) {
  // 2026 is not a leap year: 2026-02-28 23:00 +2h -> 2026-03-01 01:00
  Date d = mk(2026, 2, 28, 23, 0);
  ClockMath::applyOffsetMinutes(d, 2 * 60);
  EXPECT_EQ(d.month, 3);
  EXPECT_EQ(d.day, 1);
}

TEST(ApplyOffsetMinutes, HandlesQuarterHourOffset) {
  // UTC+5:45 (Nepal): 2026-07-26 20:30 + 345min -> 2026-07-27 02:15
  Date d = mk(2026, 7, 26, 20, 30);
  ClockMath::applyOffsetMinutes(d, 345);
  EXPECT_EQ(d.day, 27);
  EXPECT_EQ(d.hour, 2);
  EXPECT_EQ(d.minute, 15);
}

TEST(ApplyOffsetMinutes, RollsBackwardIntoLeapDay) {
  // Pins the backward path's daysInMonth() lookup: without it, "assume 31 days"
  // would still pass every other test (Jan 1 -> Dec 31 is also 31).
  // 2028-03-01 00:30 -1h -> 2028-02-29 23:30
  Date d = mk(2028, 3, 1, 0, 30);
  ClockMath::applyOffsetMinutes(d, -60);
  EXPECT_EQ(d.month, 2);
  EXPECT_EQ(d.day, 29);
  EXPECT_EQ(d.hour, 23);
}

TEST(IsLeapYear, HandlesCenturyRule) {
  // The %100 / %400 clauses are invisible to the other tests, which all pass
  // under a naive "year % 4 == 0".
  EXPECT_FALSE(ClockMath::isLeapYear(1900));
  EXPECT_TRUE(ClockMath::isLeapYear(2000));
  EXPECT_FALSE(ClockMath::isLeapYear(2100));
}

TEST(OffsetMinutesFromBiasedQuarters, DecodesBiasedValue) {
  EXPECT_EQ(ClockMath::offsetMinutesFromBiasedQuarters(48), 0);      // UTC+0
  EXPECT_EQ(ClockMath::offsetMinutesFromBiasedQuarters(76), 7 * 60); // UTC+7
  EXPECT_EQ(ClockMath::offsetMinutesFromBiasedQuarters(0), -12 * 60); // UTC-12
  EXPECT_EQ(ClockMath::offsetMinutesFromBiasedQuarters(104), 14 * 60); // UTC+14
}

TEST(OffsetMinutesFromBiasedQuarters, ClampsCorruptedValue) {
  // Values above 104 are clamped, matching HalClock::formatTime's guard.
  EXPECT_EQ(ClockMath::offsetMinutesFromBiasedQuarters(200), 14 * 60);
}
