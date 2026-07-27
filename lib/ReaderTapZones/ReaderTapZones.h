#pragma once

#include <cstdint>

// Reader page-turn tap zones: three even columns with an unbound center.
//
// The center third is deliberately inert. It absorbs incidental contact from
// picking up or repositioning the device, which is the common source of
// accidental page turns on a button-less, touch-only board.
//
// Pure and hardware-free so the split can be host-tested at every orientation
// the reader supports.
namespace ReaderTapZones {

enum class Action : uint8_t { None, Previous, Next };

// x is a logical screen coordinate; screenWidth is renderer.getScreenWidth().
// Callers must pass the live width rather than a constant — the reader rotates.
constexpr Action actionForX(const int x, const int screenWidth) {
  if (screenWidth <= 0 || x < 0 || x >= screenWidth) return Action::None;
  // Integer division floors, so the center band absorbs the remainder and is
  // never narrower than the outer two. That keeps a neutral zone at every width.
  const int third = screenWidth / 3;
  if (third <= 0) return Action::None;
  if (x < third) return Action::Previous;
  if (x >= screenWidth - third) return Action::Next;
  return Action::None;
}

}  // namespace ReaderTapZones
