#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;

// Slate: a touch-first theme for button-less boards such as the M5Paper S3.
// Every tappable control clears a 44 px floor, and the layout targets the
// 540x960 portrait logical screen.
namespace SlateMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 18,
                                 .batteryHeight = 14,
                                 .topPadding = 0,
                                 .batteryBarHeight = 24,
                                 .headerHeight = 56,
                                 .verticalSpacing = 12,
                                 .previewPadding = 14,
                                 .previewHeightPercent = 30,
                                 .contentSidePadding = 24,
                                 .listRowHeight = 48,
                                 .listWithSubtitleRowHeight = 72,
                                 .menuRowHeight = 56,
                                 .menuSpacing = 8,
                                 .tabSpacing = 12,
                                 .tabBarHeight = 56,
                                 .scrollBarWidth = 5,
                                 .scrollBarRightOffset = 6,
                                 .homeTopPadding = 40,
                                 .homeCoverHeight = 400,
                                 .homeCoverTileHeight = 400,
                                 .homeRecentBooksCount = 1,
                                 .homeContinueReadingInMenu = true,
                                 .homeMenuTopOffset = 24,
                                 .buttonHintsHeight = 40,
                                 .sideButtonHintsWidth = 30,
                                 .progressBarHeight = 16,
                                 .progressBarMarginTop = 1,
                                 .statusBarHorizontalMargin = 8,
                                 .statusBarVerticalMargin = 20,
                                 .keyboardKeyHeight = 52,
                                 .keyboardKeySpacing = 8,
                                 .keyboardCenteredText = true,
                                 .keyboardVerticalOffset = 0,
                                 .keyboardTextFieldWidthPercent = 88,
                                 .keyboardWidthPercent = 96,
                                 .popupTopOffsetRatio = 0.12f,
                                 .popupMarginX = 24,
                                 .popupMarginY = 20,
                                 .popupFrameThickness = 2,
                                 .popupCornerRadius = 16,
                                 .popupTextBold = true,
                                 .popupTextInverted = false,
                                 .popupTextBaselineOffsetY = -2,
                                 .popupProgressBarHeight = 6,
                                 .popupProgressDrawOutline = true,
                                 .popupProgressClampPercent = true,
                                 .popupProgressFillInverted = false,
                                 .popupProgressOutlineInverted = false,
                                 .optionPopupItemSpacing = 8,
                                 .optionPopupInnerPadding = 24,
                                 .optionPopupSelectionHPadding = 20,
                                 .optionPopupSelectionVPadding = 14,
                                 .optionPopupTitleGap = 16,
                                 .optionPopupUseSmallFont = false,
                                 .optionPopupOptionFontBold = false,
                                 .optionPopupSelectionRadius = 24,
                                 .optionPopupSelectionLight = false,
                                 .optionPopupDrawAllRows = true,
                                 .optionPopupDialogSideMargin = 24,
                                 .optionPopupTitleSeparator = false,
                                 .textFieldHorizontalPadding = 10,
                                 .textFieldNormalThickness = 2,
                                 .textFieldCursorThickness = 3,
                                 .textFieldLineEndOffset = -1};
}  // namespace SlateMetrics

class SlateTheme : public BaseTheme {
 public:
  int getListRowStep(bool hasSubtitle) const override;
  int getListPageItems(int contentHeight, bool hasSubtitle) const override;
  void drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                const std::function<std::string(int index)>& rowTitle,
                const std::function<std::string(int index)>& rowSubtitle = nullptr,
                const std::function<UIIcon(int index)>& rowIcon = nullptr,
                const std::function<std::string(int index)>& rowValue = nullptr, bool highlightValue = false,
                const std::function<bool(int index)>& rowDimmed = nullptr) const override;
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle = nullptr) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  bool showsFileIcons() const override { return true; }
};
