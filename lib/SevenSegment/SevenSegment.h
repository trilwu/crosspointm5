#pragma once

// Seven-segment digit encoding, for drawing clock digits far larger than any
// bundled font (the largest is 18pt and GfxRenderer cannot scale text).
// Header-only and dependency-free so it builds in the host gtest suite.
//
//      --a--
//     |     |
//     f     b
//     |     |
//      --g--
//     |     |
//     e     c
//     |     |
//      --d--

#include <cstdint>

namespace SevenSegment {

constexpr uint8_t SEG_A = 0x01;
constexpr uint8_t SEG_B = 0x02;
constexpr uint8_t SEG_C = 0x04;
constexpr uint8_t SEG_D = 0x08;
constexpr uint8_t SEG_E = 0x10;
constexpr uint8_t SEG_F = 0x20;
constexpr uint8_t SEG_G = 0x40;

// Returns the lit-segment bitmask for `digit`, or 0 when out of range.
constexpr uint8_t segmentsForDigit(int digit) {
  switch (digit) {
    case 0: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
    case 1: return SEG_B | SEG_C;
    case 2: return SEG_A | SEG_B | SEG_D | SEG_E | SEG_G;
    case 3: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_G;
    case 4: return SEG_B | SEG_C | SEG_F | SEG_G;
    case 5: return SEG_A | SEG_C | SEG_D | SEG_F | SEG_G;
    case 6: return SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
    case 7: return SEG_A | SEG_B | SEG_C;
    case 8: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
    case 9: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
    default: return 0;
  }
}

}  // namespace SevenSegment
