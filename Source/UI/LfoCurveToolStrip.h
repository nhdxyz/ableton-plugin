#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class LfoCurveToolStrip final : public juce::Component
{
public:
    enum class Tool
    {
        invert,
        reverse,
        smooth,
        quantize,
        randomize,
        garage
    };

    LfoCurveToolStrip();

    std::function<void(Tool)> onToolSelected;

    void resized() override;

private:
    static constexpr size_t toolCount = 6;

    std::array<juce::TextButton, toolCount> buttons;

    static juce::String labelForTool(Tool tool);
    static juce::String tooltipForTool(Tool tool);
    static Tool toolForIndex(size_t index) noexcept;
};
}
