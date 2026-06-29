#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class HomeSignalFlowDisplay : public juce::Component
{
public:
    struct Node
    {
        juce::String label;
        juce::String detail;
        float amount = 0.0f;
        bool active = false;
    };

    struct State
    {
        std::array<Node, 6> nodes {
            Node { "SOURCE", "Synth", 1.0f, true },
            Node { "FILTER", "LP", 0.5f, true },
            Node { "MOTION", "Idle", 0.0f, false },
            Node { "FX", "Dry", 0.0f, false },
            Node { "GUARD", "Off", 0.0f, false },
            Node { "OUT", "SAFE", 0.0f, true }
        };
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float clamp01(float value) noexcept;
    static juce::Colour nodeColour(size_t index, bool active);
    static void drawNode(juce::Graphics& g, juce::Rectangle<float> area, const Node& node, juce::Colour colour);
};
}
