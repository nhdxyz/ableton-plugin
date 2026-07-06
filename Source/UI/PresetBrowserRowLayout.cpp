#include "PresetBrowserRowLayout.h"

namespace UI::PresetBrowserRowLayout
{
namespace
{
constexpr auto compactRowBreakpoint = 420;
}

Metrics forSize(int width, int height)
{
    auto row = juce::Rectangle<int>(0, 0, juce::jmax(0, width), juce::jmax(0, height)).reduced(9, 4);
    Metrics layout;
    layout.paddedWidth = juce::jmax(0, row.getWidth());
    if (layout.paddedWidth <= 0)
        return layout;

    layout.compact = layout.paddedWidth < compactRowBreakpoint;
    layout.compact = layout.compact || height <= 34;
    if (layout.compact)
    {
        constexpr auto gap = 4;
        layout.previewWidth = juce::jlimit(60, 92, layout.paddedWidth / 5);
        layout.infoWidth = juce::jlimit(52, 78, layout.paddedWidth / 6);
        layout.macroWidth = 0;
        layout.showsMacroStrip = false;
        layout.nameWidth = juce::jmax(0, layout.paddedWidth - layout.previewWidth - layout.infoWidth - (gap * 2));
        return layout;
    }

    auto remainingWidth = layout.paddedWidth;
    layout.infoWidth = juce::jlimit(78, 118, remainingWidth / 5);
    remainingWidth -= layout.infoWidth + 6;
    layout.previewWidth = juce::jlimit(78, 118, remainingWidth / 5);
    remainingWidth -= layout.previewWidth + 6;
    layout.macroWidth = 0;
    layout.showsMacroStrip = false;
    layout.nameWidth = juce::jmax(0, remainingWidth);
    return layout;
}
}
