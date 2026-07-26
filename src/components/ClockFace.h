#pragma once

class GfxRenderer;

// Full-screen wall clock: large seven-segment HH:MM, a date line, and battery
// percentage. Shared by the CLOCK sleep screen and by the light-sleep loop that
// repaints it once a minute, so the two can never drift apart.
namespace ClockFace {

// Draws the clock and pushes it to the panel.
// fullRefresh: true for a clean full-panel refresh (screen entry, or periodically
// to clear ghosting); false for the cheaper fast refresh used each minute.
// Returns false without drawing when no RTC is present or the read failed.
bool draw(const GfxRenderer& renderer, bool fullRefresh);

}  // namespace ClockFace
