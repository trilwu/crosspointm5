#include "SlateTheme.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <string>

#include "CrossPointSettings.h"
#include "fontIds.h"

namespace {
constexpr int kRowRadius = 12;
constexpr int kRowInsetX = 20;
constexpr int kRowGap = 6;
constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kSubtitleFontId = SMALL_FONT_ID;
constexpr int kSubtitleGap = 4;
// BaseTheme::drawBatteryRight shifts the battery icon down by this many pixels
// from the y it's given (while drawing the percentage text at the unshifted
// y). Subtract it here so the icon itself ends up centered in the header.
constexpr int kBatteryIconInternalOffsetY = 6;

void drawSlateScrollBar(const GfxRenderer& renderer, const Rect rect, const int itemCount, const int pageStartIndex,
                        const int pageItems) {
  if (itemCount <= 0 || pageItems <= 0 || itemCount <= pageItems) return;

  const int barW = SlateMetrics::values.scrollBarWidth;
  const int barX = rect.x + rect.width - SlateMetrics::values.scrollBarRightOffset - barW;
  const int barH = rect.height;
  const int thumbH = std::max(16, (barH * pageItems) / itemCount);
  const int maxStart = std::max(1, itemCount - pageItems);
  const int maxTravel = std::max(1, barH - thumbH);
  const int clampedStart = std::clamp(pageStartIndex, 0, maxStart);
  const int thumbY = rect.y + (clampedStart * maxTravel) / maxStart;

  renderer.fillRoundedRect(barX, thumbY, barW, thumbH, barW / 2, Color::Black);
}
}  // namespace

int SlateTheme::getListRowStep(const bool hasSubtitle) const {
  const int rowHeight =
      hasSubtitle ? SlateMetrics::values.listWithSubtitleRowHeight : SlateMetrics::values.listRowHeight;
  return rowHeight + kRowGap;
}

int SlateTheme::getListPageItems(const int contentHeight, const bool hasSubtitle) const {
  const int rowStep = getListRowStep(hasSubtitle);
  if (rowStep <= 0) return 1;
  return std::max(1, contentHeight / rowStep);
}

void SlateTheme::drawList(const GfxRenderer& renderer, const Rect rect, const int itemCount, const int selectedIndex,
                          const std::function<std::string(int index)>& rowTitle,
                          const std::function<std::string(int index)>& rowSubtitle,
                          const std::function<UIIcon(int index)>& rowIcon,
                          const std::function<std::string(int index)>& rowValue, const bool highlightValue,
                          const std::function<bool(int index)>& rowDimmed) const {
  (void)rowIcon;
  (void)highlightValue;

  const bool hasSubtitle = static_cast<bool>(rowSubtitle);
  const int rowHeight =
      hasSubtitle ? SlateMetrics::values.listWithSubtitleRowHeight : SlateMetrics::values.listRowHeight;
  const int rowStep = rowHeight + kRowGap;
  const int pageItems = std::max(1, rect.height / rowStep);
  const int pageStartIndex = std::max(0, selectedIndex / pageItems) * pageItems;

  const int sidePadding = SlateMetrics::values.contentSidePadding;
  const int rowX = rect.x + sidePadding;
  const int rowWidth = rect.width - sidePadding * 2;

  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const int subtitleLineHeight = renderer.getLineHeight(kSubtitleFontId);

  for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + pageItems; i++) {
    const int rowY = rect.y + (i % pageItems) * rowStep;
    const bool isSelected = i == selectedIndex;
    const bool isDimmed = rowDimmed && rowDimmed(i);

    if (isSelected) {
      renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius, Color::Black);
    }
    // Unselected rows are left unfilled. Whitespace separates them instead of
    // rules, which is what keeps the screen quiet on e-ink.

    const bool inkOnLight = !isSelected;
    int textAreaWidth = rowWidth - kRowInsetX * 2;

    if (rowValue) {
      const std::string valueText = rowValue(i);
      if (!valueText.empty()) {
        constexpr int kMinTitleWidth = 48;
        const int maxValueWidth = std::max(0, textAreaWidth - kRowGap - kMinTitleWidth);
        if (maxValueWidth > 0) {
          const std::string truncated =
              renderer.truncatedText(kTitleFontId, valueText.c_str(), maxValueWidth, EpdFontFamily::REGULAR);
          const int valueW = renderer.getTextWidth(kTitleFontId, truncated.c_str(), EpdFontFamily::REGULAR);
          renderer.drawText(kTitleFontId, rowX + rowWidth - kRowInsetX - valueW,
                            rowY + (rowHeight - titleLineHeight) / 2, truncated.c_str(), inkOnLight,
                            EpdFontFamily::REGULAR);
          textAreaWidth = std::max(0, textAreaWidth - valueW - kRowGap);
        }
      }
    }

    const auto titleStyle = isDimmed ? EpdFontFamily::REGULAR : EpdFontFamily::BOLD;
    const std::string title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, titleStyle);

    if (!hasSubtitle) {
      renderer.drawText(kTitleFontId, rowX + kRowInsetX, rowY + (rowHeight - titleLineHeight) / 2, title.c_str(),
                        inkOnLight, titleStyle);
      continue;
    }

    const std::string subtitleRaw = rowSubtitle(i);
    if (subtitleRaw.empty()) {
      renderer.drawText(kTitleFontId, rowX + kRowInsetX, rowY + (rowHeight - titleLineHeight) / 2, title.c_str(),
                        inkOnLight, titleStyle);
      continue;
    }

    const int blockHeight = titleLineHeight + kSubtitleGap + subtitleLineHeight;
    const int titleY = rowY + (rowHeight - blockHeight) / 2;
    const std::string subtitle =
        renderer.truncatedText(kSubtitleFontId, subtitleRaw.c_str(), textAreaWidth, EpdFontFamily::REGULAR);
    renderer.drawText(kTitleFontId, rowX + kRowInsetX, titleY, title.c_str(), inkOnLight, titleStyle);
    renderer.drawText(kSubtitleFontId, rowX + kRowInsetX, titleY + titleLineHeight + kSubtitleGap, subtitle.c_str(),
                      inkOnLight, EpdFontFamily::REGULAR);
  }

  drawSlateScrollBar(renderer, rect, itemCount, pageStartIndex, pageItems);
}

void SlateTheme::drawHeader(const GfxRenderer& renderer, const Rect rect, const char* title,
                            const char* subtitle) const {
  if (!title) return;

  const int sidePadding = SlateMetrics::values.contentSidePadding;
  const int textX = rect.x + sidePadding;

  const bool showBatteryPercentage =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
  const int batteryIconX = rect.x + rect.width - sidePadding - SlateMetrics::values.batteryWidth;

  // Reserve space for the widest possible percentage text ("100%") so the title truncation
  // width accounts for it, avoiding overlap when the digit count changes (e.g. 100% -> 99%).
  int batteryGroupLeftX = batteryIconX;
  int maxPercentTextWidth = 0;
  if (showBatteryPercentage) {
    maxPercentTextWidth = renderer.getTextWidth(SMALL_FONT_ID, "100%");
    batteryGroupLeftX -= maxPercentTextWidth + batteryPercentSpacing;
  }

  constexpr int kBatteryTitleGap = 20;
  const int maxWidth = std::max(0, batteryGroupLeftX - kBatteryTitleGap - textX);
  if (maxWidth <= 0) return;

  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const std::string truncatedTitle =
      renderer.truncatedText(kTitleFontId, title, maxWidth, EpdFontFamily::BOLD);

  if (!subtitle || *subtitle == '\0') {
    renderer.drawText(kTitleFontId, textX, rect.y + (rect.height - titleLineHeight) / 2, truncatedTitle.c_str(), true,
                      EpdFontFamily::BOLD);
  } else {
    const int subtitleLineHeight = renderer.getLineHeight(kSubtitleFontId);
    const int blockHeight = titleLineHeight + kSubtitleGap + subtitleLineHeight;
    const int titleY = rect.y + (rect.height - blockHeight) / 2;
    const std::string truncatedSubtitle =
        renderer.truncatedText(kSubtitleFontId, subtitle, maxWidth, EpdFontFamily::REGULAR);

    renderer.drawText(kTitleFontId, textX, titleY, truncatedTitle.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawText(kSubtitleFontId, textX, titleY + titleLineHeight + kSubtitleGap, truncatedSubtitle.c_str(),
                      true, EpdFontFamily::REGULAR);
  }

  // Clear the battery region (icon + widest possible percentage text) before drawing, so a
  // partial refresh going from e.g. 100% -> 99% doesn't leave a stale third glyph behind.
  const int clearX = batteryIconX - (showBatteryPercentage ? maxPercentTextWidth + batteryPercentSpacing : 0);
  const int clearW = batteryIconX + SlateMetrics::values.batteryWidth - clearX;
  renderer.fillRect(clearX, rect.y, clearW, rect.height, false);

  drawBatteryRight(renderer,
                   Rect{batteryIconX,
                        rect.y + (rect.height - SlateMetrics::values.batteryHeight) / 2 -
                            kBatteryIconInternalOffsetY,
                        SlateMetrics::values.batteryWidth, SlateMetrics::values.batteryHeight},
                   showBatteryPercentage);
}

void SlateTheme::drawButtonMenu(GfxRenderer& renderer, const Rect rect, const int buttonCount,
                                const int selectedIndex, const std::function<std::string(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon) const {
  (void)rowIcon;

  // HomeActivity::onInput hit-tests this menu itself (it doesn't call back into the
  // theme), so the drawn geometry here must match its math exactly:
  //   menuTop = homeTopPadding + homeCoverTileHeight + homeMenuTopOffset
  //   rowTouch(top = menuTop, rowStep = menuRowHeight + menuSpacing, rowHeight = menuRowHeight)
  // rowTouch computes row = (y - menuTop) / rowStep, valid when (y - menuTop) % rowStep < rowHeight.
  // HomeActivity passes rect.y == menuTop (same expression), so row i's tappable band is
  // [rect.y + i*rowStep, rect.y + i*rowStep + menuRowHeight). Rows must therefore start
  // at rect.y with NO extra offset -- adding verticalSpacing here (as BaseTheme's
  // unrelated draw does with its own metrics) would shift every drawn row down and off
  // the tap band it's supposed to occupy.
  const int sidePadding = SlateMetrics::values.contentSidePadding;
  const int rowX = rect.x + sidePadding;
  const int rowWidth = rect.width - sidePadding * 2;
  const int rowHeight = SlateMetrics::values.menuRowHeight;
  const int rowStep = rowHeight + SlateMetrics::values.menuSpacing;
  const int lineHeight = renderer.getLineHeight(kTitleFontId);

  for (int i = 0; i < buttonCount; ++i) {
    const int rowY = rect.y + i * rowStep;
    const bool isSelected = selectedIndex == i;

    if (isSelected) {
      renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius, Color::Black);
    }
    // Unselected rows are left unfilled, matching drawList's visual language.

    const std::string label = buttonLabel(i);
    const int maxLabelWidth = std::max(0, rowWidth - kRowInsetX * 2);
    const std::string truncatedLabel =
        renderer.truncatedText(kTitleFontId, label.c_str(), maxLabelWidth, EpdFontFamily::BOLD);
    const int textY = rowY + (rowHeight - lineHeight) / 2;
    renderer.drawText(kTitleFontId, rowX + kRowInsetX, textY, truncatedLabel.c_str(), !isSelected,
                      EpdFontFamily::BOLD);
  }
}
