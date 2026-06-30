#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class RandomMorphPad final : public juce::Component,
                             public juce::TooltipClient
{
public:
    struct State
    {
        float x = 0.50f;
        float y = 0.50f;
        float amount = 0.45f;
        float chaos = 0.25f;
        float brightness = 0.0f;
        float drive = 0.0f;
        float motion = 0.0f;
        std::array<float, 7> sectionIntensities {};
        juce::String recipe = "House";
        juce::String scope = "All";
    };

    std::function<void(float, float)> onChange;
    std::function<void(float, float)> onCommit;

    RandomMorphPad();

    void setState(State newState);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    State state;
    bool mouseMoved = false;

    juce::Rectangle<float> getPadBounds() const;
    void applyMousePosition(juce::Point<float> position, bool notify);
    static float clamp01(float value) noexcept;
    static juce::Colour sectionColour(size_t index) noexcept;
};
}
