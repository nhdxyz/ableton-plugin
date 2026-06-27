#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class ModCurveDisplay final : public juce::Component
{
public:
    void setValues(const std::array<float, 8>& newValues, bool shouldHighlight);
    void paint(juce::Graphics& g) override;

private:
    std::array<float, 8> values { 0.0f, 0.58f, 1.0f, 0.42f, -0.18f, -0.72f, -1.0f, -0.36f };
    bool highlighted = false;
};
}
