#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class MacroPerformanceMap final : public juce::Component,
                                  public juce::TooltipClient
{
public:
    using ValueArray = std::array<float, 8>;

    std::function<void(size_t, float)> onChange;

    MacroPerformanceMap();

    void setValues(ValueArray newValues);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    ValueArray values {};
    int activeMacroIndex = -1;

    juce::Rectangle<float> getMapBounds() const;
    juce::Point<float> pointForIndex(size_t index, float amount, juce::Rectangle<float> mapBounds) const noexcept;
    size_t indexForPosition(juce::Point<float> position, juce::Rectangle<float> mapBounds) const noexcept;
    void applyMousePosition(juce::Point<float> position);
    void notifyMacroChange(size_t index);

    static float clamp01(float value) noexcept;
    static juce::Colour colourForIndex(size_t index) noexcept;
    static juce::String labelForIndex(size_t index);
    static juce::String shortLabelForIndex(size_t index);
};
}
