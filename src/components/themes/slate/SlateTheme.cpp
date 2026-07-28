#include "SlateTheme.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <string>

#include "CrossPointSettings.h"
#include "components/icons/book24.h"
#include "components/icons/file24.h"
#include "components/icons/folder24.h"
#include "components/icons/image24.h"
#include "components/icons/text24.h"
#include "fontIds.h"

namespace {
constexpr int kRowRadius = 16;
constexpr int kRowInsetX = 20;
constexpr int kRowGap = 6;
constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kSubtitleFontId = SMALL_FONT_ID;
constexpr int kSubtitleGap = 8;
// BaseTheme::drawBatteryRight shifts the battery icon down by this many pixels
// from the y it's given (while drawing the percentage text at the unshifted
// y). Subtract it here so the icon itself ends up centered in the header.
constexpr int kBatteryIconInternalOffsetY = 6;
constexpr int kListIconSize = 24;
constexpr int kIconTextGap = 10;

// FileBrowserActivity is the only caller that passes rowIcon today, and it only
// ever hands back Folder/Book/Text/Image/File — the 24px set covers all of those.
const uint8_t* iconBitmapFor(const UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return Folder24Icon;
    case UIIcon::Book:
      return Book24Icon;
    case UIIcon::Text:
      return Text24Icon;
    case UIIcon::Image:
      return Image24Icon;
    case UIIcon::File:
      return File24Icon;
    default:
      return nullptr;
  }
}

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
      renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius, Color::LightGray);
    }
    // Unselected rows are left unfilled. Whitespace separates them instead of
    // rules, which is what keeps the screen quiet on e-ink.

    int textAreaWidth = rowWidth - kRowInsetX * 2;
    int textStartX = rowX + kRowInsetX;

    if (rowIcon) {
      const uint8_t* iconBitmap = iconBitmapFor(rowIcon(i));
      if (iconBitmap) {
        const int iconY = rowY + (rowHeight - kListIconSize) / 2;
        renderer.drawIcon(iconBitmap, textStartX, iconY, kListIconSize);
        // Only reserve the icon slot when a bitmap actually exists; otherwise
        // the gutter is blank and the title loses that width for nothing.
        textStartX += kListIconSize + kIconTextGap;
        textAreaWidth = std::max(0, textAreaWidth - kListIconSize - kIconTextGap);
      }
    }

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
                            rowY + (rowHeight - titleLineHeight) / 2, truncated.c_str(), true,
                            EpdFontFamily::REGULAR);
          textAreaWidth = std::max(0, textAreaWidth - valueW - kRowGap);
        }
      }
    }

    const auto titleStyle = isDimmed ? EpdFontFamily::REGULAR : EpdFontFamily::BOLD;
    const std::string title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, titleStyle);

    if (!hasSubtitle) {
      renderer.drawText(kTitleFontId, textStartX, rowY + (rowHeight - titleLineHeight) / 2, title.c_str(),
                        true, titleStyle);
      continue;
    }

    const std::string subtitleRaw = rowSubtitle(i);
    if (subtitleRaw.empty()) {
      renderer.drawText(kTitleFontId, textStartX, rowY + (rowHeight - titleLineHeight) / 2, title.c_str(),
                        true, titleStyle);
      continue;
    }

    const int blockHeight = titleLineHeight + kSubtitleGap + subtitleLineHeight;
    const int titleY = rowY + (rowHeight - blockHeight) / 2;
    const std::string subtitle =
        renderer.truncatedText(kSubtitleFontId, subtitleRaw.c_str(), textAreaWidth, EpdFontFamily::REGULAR);
    renderer.drawText(kTitleFontId, textStartX, titleY, title.c_str(), true, titleStyle);
    renderer.drawText(kSubtitleFontId, textStartX, titleY + titleLineHeight + kSubtitleGap, subtitle.c_str(),
                      true, EpdFontFamily::REGULAR);
  }

  drawSlateScrollBar(renderer, rect, itemCount, pageStartIndex, pageItems);
}

void SlateTheme::drawHeader(const GfxRenderer& renderer, const Rect rect, const char* title,
                            const char* subtitle) const {
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

  // A fresh device with no recent books passes a null title (see HomeActivity); the
  // battery above must still be drawn in that case, so this check comes after it.
  if (!title) return;

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
      renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius, Color::LightGray);
    }
    // Unselected rows are left unfilled, matching drawList's visual language.

    const std::string label = buttonLabel(i);
    const int maxLabelWidth = std::max(0, rowWidth - kRowInsetX * 2);
    const std::string truncatedLabel =
        renderer.truncatedText(kTitleFontId, label.c_str(), maxLabelWidth, EpdFontFamily::BOLD);
    const int textY = rowY + (rowHeight - lineHeight) / 2;
    renderer.drawText(kTitleFontId, rowX + kRowInsetX, textY, truncatedLabel.c_str(), true,
                      EpdFontFamily::BOLD);
  }
}

void SlateTheme::drawSubHeader(const GfxRenderer& renderer, const Rect rect, const char* label,
                               const char* rightLabel) const {
  constexpr int kMaxRightLabelWidth = 200;
  const int sidePadding = SlateMetrics::values.contentSidePadding;
  const int labelLineHeight = renderer.getLineHeight(kTitleFontId);
  const int labelY = rect.y + std::max(0, (rect.height - labelLineHeight) / 2);

  int rightSpace = sidePadding;
  if (rightLabel) {
    const std::string truncatedRightLabel =
        renderer.truncatedText(kSubtitleFontId, rightLabel, kMaxRightLabelWidth, EpdFontFamily::REGULAR);
    const int rightLabelWidth = renderer.getTextWidth(kSubtitleFontId, truncatedRightLabel.c_str());
    const int rightLineHeight = renderer.getLineHeight(kSubtitleFontId);
    const int rightY = rect.y + std::max(0, (rect.height - rightLineHeight) / 2);
    renderer.drawText(kSubtitleFontId, rect.x + rect.width - sidePadding - rightLabelWidth, rightY,
                      truncatedRightLabel.c_str());
    rightSpace += rightLabelWidth + kRowGap;
  }

  const std::string truncatedLabel = renderer.truncatedText(
      kTitleFontId, label, std::max(0, rect.width - sidePadding - rightSpace), EpdFontFamily::REGULAR);
  renderer.drawText(kTitleFontId, rect.x + sidePadding, labelY, truncatedLabel.c_str(), true, EpdFontFamily::REGULAR);
}

void SlateTheme::drawTabBar(const GfxRenderer& renderer, const Rect rect, const std::vector<TabInfo>& tabs,
                            const bool selected) const {
  constexpr int underlineHeight = 2;  // Height of selection underline
  constexpr int underlineGap = 4;     // Gap between text and underline

  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);

  // SlateTheme does not override tabIndexFromPoint, so taps are still hit-tested by
  // BaseTheme::tabIndexFromPoint using BaseMetrics::values.contentSidePadding/tabSpacing.
  // Mirror those exact x constants here (instead of SlateMetrics::values) so the drawn
  // tab boundaries land exactly where the inherited hit test expects them. Only the
  // vertical position changes, to center the label block within the taller rect.height
  // that Slate's tabBarHeight metric produces -- that doesn't affect tabIndexFromPoint,
  // which only checks y against rect.y/rect.height, not the exact glyph baseline.
  int currentX = rect.x + BaseMetrics::values.contentSidePadding;
  const int contentHeight = lineHeight + underlineGap + underlineHeight;
  const int topY = rect.y + std::max(0, (rect.height - contentHeight) / 2);

  for (const auto& tab : tabs) {
    const int textWidth =
        renderer.getTextWidth(UI_12_FONT_ID, tab.label, tab.selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    if (tab.selected) {
      if (selected) {
        renderer.fillRect(currentX - 3, topY, textWidth + 6, lineHeight + underlineGap);
      } else {
        renderer.fillRect(currentX, topY + lineHeight + underlineGap, textWidth, underlineHeight);
      }
    }

    renderer.drawText(UI_12_FONT_ID, currentX, topY, tab.label, !(tab.selected && selected),
                      tab.selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    currentX += textWidth + BaseMetrics::values.tabSpacing;
  }
}

Rect SlateTheme::drawPopup(const GfxRenderer& renderer, const char* message) const {
  const auto& metrics = SlateMetrics::values;
  const int marginX = metrics.popupMarginX;
  const int marginY = metrics.popupMarginY;
  const int frameThickness = metrics.popupFrameThickness;
  const EpdFontFamily::Style popupFontFamily = metrics.popupTextBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  // Scale y position proportionally to screen height, matching BaseTheme::drawPopup.
  const int y = static_cast<int>(renderer.getScreenHeight() * metrics.popupTopOffsetRatio);
  const int textWidth = renderer.getTextWidth(kTitleFontId, message, popupFontFamily);
  const int textHeight = renderer.getLineHeight(kTitleFontId);
  const int w = textWidth + marginX * 2;
  const int h = textHeight + marginY * 2;
  const int x = (renderer.getScreenWidth() - w) / 2;

  // Task 1's convention is that no Slate surface inverts to solid black. BaseTheme's
  // rounded branch always fills the card black regardless of popupTextInverted, so it
  // can't be reused here -- a white rounded card with a black outline stands in for it.
  // popupTextInverted=false means "not inverted", i.e. normal black text on the light
  // card, so the glyph-black flag is the negation of the metric.
  renderer.fillRoundedRect(x, y, w, h, metrics.popupCornerRadius, Color::White);
  renderer.drawRoundedRect(x, y, w, h, frameThickness, metrics.popupCornerRadius, true);

  const int textX = x + (w - textWidth) / 2;
  const int textY = y + marginY + metrics.popupTextBaselineOffsetY;
  renderer.drawText(kTitleFontId, textX, textY, message, !metrics.popupTextInverted, popupFontFamily);
  renderer.displayBuffer();
  // The inner card rect (excludes the frame outline), matching BaseTheme::drawPopup's
  // contract -- callers such as fillPopupProgress position a progress bar inside it.
  return Rect{x, y, w, h};
}

void SlateTheme::drawOptionPopup(const GfxRenderer& renderer, const char* title,
                                 const std::vector<std::string>& options, const int selectedIndex) const {
  const auto& metrics = SlateMetrics::values;
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  const int optionFontId = metrics.optionPopupUseSmallFont ? kSubtitleFontId : kTitleFontId;
  const EpdFontFamily::Style optionStyle =
      metrics.optionPopupOptionFontBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

  const int itemSpacing = metrics.optionPopupItemSpacing;
  const int innerPadding = metrics.optionPopupInnerPadding;
  const int selectionHPadding = metrics.optionPopupSelectionHPadding;
  const int selectionVPadding = metrics.optionPopupSelectionVPadding;

  const int optionLineHeight = renderer.getLineHeight(optionFontId);
  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  // Comfortable, not minimum: option rows must clear the 60px floor from the design
  // language, which is why optionPopupSelectionVPadding was raised in Task 2 (see report).
  const int rowHeight = optionLineHeight + selectionVPadding * 2;

  int maxTextWidth = renderer.getTextWidth(kTitleFontId, title, EpdFontFamily::BOLD);
  for (const auto& opt : options) {
    const int w = renderer.getTextWidth(optionFontId, opt.c_str(), optionStyle);
    if (w > maxTextWidth) maxTextWidth = w;
  }

  const int optionCount = static_cast<int>(options.size());
  const int listHeight = rowHeight * optionCount + itemSpacing * std::max(0, optionCount - 1);
  const int dialogW = std::min((maxTextWidth + innerPadding * 2 + selectionHPadding * 2) * 12 / 10,
                               pageWidth - metrics.optionPopupDialogSideMargin * 2);
  const int contentHeight = titleLineHeight + metrics.optionPopupTitleGap + listHeight;
  const int dialogH = contentHeight + innerPadding * 2;
  const int dialogX = (pageWidth - dialogW) / 2;
  const int dialogY = (pageHeight - dialogH) / 2;

  const int frameThickness = metrics.popupFrameThickness;
  const int frameRadius = metrics.popupCornerRadius;

  // Same white-card-with-outline convention as drawPopup -- never a solid black dialog.
  renderer.fillRoundedRect(dialogX, dialogY, dialogW, dialogH, frameRadius, Color::White);
  renderer.drawRoundedRect(dialogX, dialogY, dialogW, dialogH, frameThickness, frameRadius, true);

  int y = dialogY + innerPadding;

  renderer.drawCenteredText(kTitleFontId, y, title, true, EpdFontFamily::BOLD);
  y += titleLineHeight;

  // No title separator rule (optionPopupTitleSeparator is false) -- spacing alone
  // separates the title from the option rows, per the design language's "whitespace,
  // not rules" principle.
  y += metrics.optionPopupTitleGap;

  const int itemRectX = dialogX + innerPadding;
  const int itemRectW = dialogW - innerPadding * 2;
  const int selectionRadius = metrics.optionPopupSelectionRadius;

  for (int i = 0; i < optionCount; i++) {
    const int itemY = y + i * (rowHeight + itemSpacing);
    const bool selected = (i == selectedIndex);
    const char* labelText = options[i].c_str();

    if (selected) {
      // LightGray fill only -- Task 1's convention, never an inverted (black) row.
      renderer.fillRoundedRect(itemRectX, itemY, itemRectW, rowHeight, selectionRadius, Color::LightGray);
    }
    // Unselected rows stay unfilled; whitespace separates them, matching drawList
    // and drawButtonMenu elsewhere in this theme.

    const int textW = renderer.getTextWidth(optionFontId, labelText, optionStyle);
    const int textY = itemY + (rowHeight - optionLineHeight) / 2;
    const int textX = itemRectX + (itemRectW - textW) / 2;
    // Text is always black: the card is always white/light, so it never needs inverting.
    renderer.drawText(optionFontId, textX, textY, labelText, true, optionStyle);
  }
}

void SlateTheme::drawTextField(const GfxRenderer& renderer, const Rect rect, const int textWidth,
                               const bool cursorMode, const int contentStartX, const int contentWidth) const {
  const auto& metrics = SlateMetrics::values;
  const int lineHeight = renderer.getLineHeight(kTitleFontId);
  const int lineY = rect.y + rect.height + lineHeight + metrics.verticalSpacing;
  const int thickness = cursorMode ? metrics.textFieldCursorThickness : metrics.textFieldNormalThickness;
  const int radius = std::max(1, thickness / 2);

  int lineStart;
  int lineW;
  if (contentWidth > 0) {
    // KeyboardEntryActivity passes contentStartX/contentWidth for multi-line fields;
    // both carry real position/size and must be honoured, as RoundedRaffTheme does.
    lineStart = rect.x + contentStartX;
    lineW = contentWidth + metrics.textFieldLineEndOffset;
  } else {
    lineW = textWidth + metrics.textFieldHorizontalPadding * 2;
    lineStart = rect.x + (rect.width - lineW) / 2;
    lineW += metrics.textFieldLineEndOffset;
  }
  if (lineW <= 0) return;

  // A rounded bar replaces BaseTheme's hard-edged drawLine, so the underline follows
  // the "generous corners" rule the rest of Slate uses instead of a sharp rule.
  renderer.fillRoundedRect(lineStart, lineY, lineW, thickness, radius, Color::Black);
}
