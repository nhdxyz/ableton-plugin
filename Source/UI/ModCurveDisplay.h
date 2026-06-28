#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class ModCurveDisplay final : public juce::Component
{
public:
    std::function<void(size_t, float)> onPointChange;

    ModCurveDisplay();

    void setValues(const std::array<float, 8>& newValues, bool shouldHighlight);
    void setPhase(float newPhase, bool shouldShowPhase);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    std::array<float, 8> values { 0.0f, 0.58f, 1.0f, 0.42f, -0.18f, -0.72f, -1.0f, -0.36f };
    bool highlighted = false;
    bool showPhase = false;
    float phase = 0.0f;
    int draggedIndex = -1;
    int hoveredIndex = -1;
    int lastEditedIndex = -1;

    juce::Rectangle<float> getPlotBounds() const;
    size_t pointIndexForX(float xPosition) const;
    float valueForPosition(juce::Point<float> position, juce::ModifierKeys modifiers) const;
    void applyPointValue(size_t index, float value);
    void updatePointFromPosition(juce::Point<float> position, juce::ModifierKeys modifiers);
};
}
