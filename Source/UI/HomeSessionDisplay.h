#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class HomeSessionDisplay : public juce::Component
{
public:
    struct State
    {
        juce::String selectedName = "No preset selected";
        juce::String selectedMeta = "Choose a sound";
        juce::String filterName = "All";
        juce::String candidateName = "No candidate";
        int visibleCount = 0;
        int totalCount = 0;
        int rating = 0;
        bool favorite = false;
        bool factory = false;
        bool generated = false;
        bool compareReady = false;
        bool candidateReady = false;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static void drawStatusPill(juce::Graphics& g,
                               juce::Rectangle<float> area,
                               const juce::String& text,
                               juce::Colour colour);
};
}
