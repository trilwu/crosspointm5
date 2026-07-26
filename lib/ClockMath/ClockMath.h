#pragma once

// Pure calendar arithmetic for clock display. Header-only and free of Arduino/SDK
// dependencies so it builds in the host gtest suite.
//
// HalClock::formatTime() applies the UTC offset to hours/minutes only and wraps
// modulo 24h, which is fine for a time-only status bar but wrong for anything that
// also shows a date: an offset can cross midnight and must roll the calendar.

#include <cstdint>

namespace ClockMath {

struct Date {
  uint16_t year = 2000;
  uint8_t month = 1;  // 1-12
  uint8_t day = 1;    // 1-31
  uint8_t hour = 0;   // 0-23
  uint8_t minute = 0;
};

constexpr bool isLeapYear(uint16_t y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

constexpr uint8_t daysInMonth(uint16_t year, uint8_t month) {
  switch (month) {
    case 2:
      return isLeapYear(year) ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    default:
      return 31;
  }
}

// Settings store the UTC offset as biased quarter-hours (48 = UTC+0, 0 = UTC-12,
// 104 = UTC+14). Values above 104 are clamped, matching HalClock::formatTime().
inline int offsetMinutesFromBiasedQuarters(uint8_t biased) {
  if (biased > 104) biased = 104;
  return (static_cast<int>(biased) - 48) * 15;
}

// Shift `d` by `offsetMinutes`, rolling day/month/year as needed.
inline void applyOffsetMinutes(Date& d, int offsetMinutes) {
  int total = static_cast<int>(d.hour) * 60 + static_cast<int>(d.minute) + offsetMinutes;

  // Carry whole days out of the minute-of-day value, keeping `total` in [0, 1440).
  int dayDelta = 0;
  while (total < 0) {
    total += 1440;
    --dayDelta;
  }
  while (total >= 1440) {
    total -= 1440;
    ++dayDelta;
  }

  d.hour = static_cast<uint8_t>(total / 60);
  d.minute = static_cast<uint8_t>(total % 60);

  while (dayDelta > 0) {
    const uint8_t dim = daysInMonth(d.year, d.month);
    if (d.day < dim) {
      ++d.day;
    } else {
      d.day = 1;
      if (d.month == 12) {
        d.month = 1;
        ++d.year;
      } else {
        ++d.month;
      }
    }
    --dayDelta;
  }
  while (dayDelta < 0) {
    if (d.day > 1) {
      --d.day;
    } else {
      if (d.month == 1) {
        d.month = 12;
        --d.year;
      } else {
        --d.month;
      }
      d.day = daysInMonth(d.year, d.month);
    }
    ++dayDelta;
  }
}

}  // namespace ClockMath
