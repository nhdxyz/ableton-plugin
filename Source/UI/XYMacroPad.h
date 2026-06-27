#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class XYMacroPad final : public juce::Component,
                         public juce::TooltipClient
{
public:
    std::function<void(float, float)> onChange;

    XYMacroPad();

    void setValues(float newX, float newY);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    float x = 0.0f;
    float y = 0.0f;

    juce::Rectangle<float> getPadBounds() const;
    void applyMousePosition(juce::Point<float> position);
};
}
