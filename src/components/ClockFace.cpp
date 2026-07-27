#include "components/ClockFace.h"

#include <BoardConfig.h>
#include <ClockMath.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalPowerManager.h>
#include <SevenSegment.h>

#include <cstdio>

#include "CrossPointSettings.h"
#include "fontIds.h"

namespace {

// Draws one seven-segment digit inside (x, y, w, h) with stroke thickness t.
void drawDigit(const GfxRenderer& renderer, int digit, int x, int y, int w, int h, int t) {
  const uint8_t segs = SevenSegment::segmentsForDigit(digit);
  const int halfH = h / 2;
  if (segs & SevenSegment::SEG_A) renderer.fillRect(x, y, w, t);
  if (segs & SevenSegment::SEG_B) renderer.fillRect(x + w - t, y, t, halfH);
  if (segs & SevenSegment::SEG_C) renderer.fillRect(x + w - t, y + halfH, t, halfH);
  if (segs & SevenSegment::SEG_D) renderer.fillRect(x, y + h - t, w, t);
  if (segs & SevenSegment::SEG_E) renderer.fillRect(x, y + halfH, t, halfH);
  if (segs & SevenSegment::SEG_F) renderer.fillRect(x, y, t, halfH);
  if (segs & SevenSegment::SEG_G) renderer.fillRect(x, y + halfH - t / 2, w, t);
}

const char* monthAbbrev(uint8_t month) {
  static const char* kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return (month >= 1 && month <= 12) ? kMonths[month - 1] : "";
}

}  // namespace

bool ClockFace::draw(const GfxRenderer& renderer, const bool fullRefresh) {
  if (!BoardConfig::hasRtc()) return false;

  ClockMath::Date now;
  uint8_t second = 0;
  if (!halClock.getLocalDateTime(now, second, SETTINGS.clockUtcOffsetQ)) return false;

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  // 12/24h presentation. The date has already had the UTC offset applied.
  int displayHour = now.hour;
  const bool use12Hour = SETTINGS.clockFormat == 1;
  bool pm = false;
  if (use12Hour) {
    pm = displayHour >= 12;
    displayHour = displayHour % 12;
    if (displayHour == 0) displayHour = 12;
  }

  // Digit geometry is derived from the panel width and assumes a portrait-ish
  // aspect; it does not adapt to orientation.
  const int dw = pageWidth / 6;
  const int dh = dw * 16 / 9;
  const int t = dw / 6;
  const int gap = dw / 6;
  const int colonW = dw / 3;

  const int h10 = displayHour / 10;
  const int h1 = displayHour % 10;
  const int m10 = now.minute / 10;
  const int m1 = now.minute % 10;
  // In 12-hour mode a leading zero hour reads as "1:05", not "01:05". Decide this
  // before the geometry so the narrower glyph run stays centred: budgeting a slot
  // we never draw would push the digits ~52px right of the text lines below.
  // Consequence: the block shifts left/right by one digit slot at the 9->10 and
  // 12->1 transitions, so those two minute-boundary repaints touch more pixels
  // than the rest.
  const bool hideLeadingZero = use12Hour && h10 == 0;

  // One gap per slot, not one fewer: the draw sequence below advances by a gap
  // after every digit slot and after the colon.
  const int slots = hideLeadingZero ? 3 : 4;
  const int totalW = slots * dw + slots * gap + colonW;
  int x = (pageWidth - totalW) / 2;
  // Clamp the origin so a landscape panel (wider than ~1.69:1) clips nothing.
  int y = pageHeight / 2 - dh;
  if (y < 0) y = 0;

  renderer.clearScreen();

  if (!hideLeadingZero) {
    drawDigit(renderer, h10, x, y, dw, dh, t);
    x += dw + gap;
  }
  drawDigit(renderer, h1, x, y, dw, dh, t);
  x += dw + gap;

  // Colon: two square pips on the segment-G line thirds.
  renderer.fillRect(x + (colonW - t) / 2, y + dh / 3 - t / 2, t, t);
  renderer.fillRect(x + (colonW - t) / 2, y + 2 * dh / 3 - t / 2, t, t);
  x += colonW + gap;

  drawDigit(renderer, m10, x, y, dw, dh, t);
  x += dw + gap;
  drawDigit(renderer, m1, x, y, dw, dh, t);

  // AM/PM, date, and battery below the digits.
  int textY = y + dh + dh / 6;
  if (use12Hour) {
    renderer.drawCenteredText(UI_12_FONT_ID, textY, pm ? "PM" : "AM");
    textY += dh / 8;
  }

  char dateBuf[32] = {};
  snprintf(dateBuf, sizeof(dateBuf), "%u %s %u", static_cast<unsigned>(now.day), monthAbbrev(now.month),
           static_cast<unsigned>(now.year));
  renderer.drawCenteredText(UI_12_FONT_ID, textY, dateBuf);

  char batteryBuf[16] = {};
  snprintf(batteryBuf, sizeof(batteryBuf), "%u%%", static_cast<unsigned>(powerManager.getBatteryPercentage()));
  renderer.drawCenteredText(SMALL_FONT_ID, textY + dh / 6, batteryBuf);

  // HALF, not FULL, for the clean-refresh path. Every other sleep screen in
  // SleepActivity.cpp picks HALF deliberately (see the comment there about a
  // FULL/GC waveform parking pixels in a different charge state). On this board's
  // LgfxEpd driver Full and Half resolve to the same waveform, so FULL buys
  // nothing; but on the SSD1677-class boards built from this same source FULL
  // selects a multi-flash GC sequence, which would make the clock the only sleep
  // screen that flashes on entry. HALF is a genuine clean refresh in its own
  // right and clears the ghosting the periodic FAST repaints leave behind.
  renderer.displayBuffer(fullRefresh ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH);
  return true;
}
