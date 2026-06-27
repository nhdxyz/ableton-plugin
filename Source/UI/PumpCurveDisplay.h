#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class PumpCurveDisplay final : public juce::Component
{
public:
    void setState(int newCurveIndex,
                  float newDepth,
                  float newShape,
                  float newPhase,
                  int newRateIndex,
                  bool newEnabled,
                  const std::array<float, 8>& newCustomValues);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    std::function<void(size_t, float)> onPointChange;

private:
    static constexpr int customCurveIndex = 5;

    int curveIndex = 0;
    int rateIndex = 0;
    float depth = 0.35f;
    float shape = 0.45f;
    float phase = 0.0f;
    bool enabled = false;
    std::array<float, 8> customValues { 1.0f, 0.82f, 0.62f, 0.44f, 0.28f, 0.16f, 0.07f, 0.0f };
    int hoveredIndex = -1;
    int draggedIndex = -1;

    float duckAmount(float recovery) const;
    float customDuckAmount(float recovery) const;
    juce::String curveName() const;
    juce::String rateName() const;
    juce::Rectangle<float> plotBounds() const;
    juce::Point<float> pointForCustomIndex(size_t index) const;
    int nearestCustomIndex(juce::Point<float> position) const;
    void updateCustomPointFromPosition(int index, juce::Point<float> position);
};
}
