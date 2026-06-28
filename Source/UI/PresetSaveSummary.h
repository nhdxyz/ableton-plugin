#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class PresetSaveSummary : public juce::Component
{
public:
    struct State
    {
        juce::String name = "Untitled patch";
        juce::String category = "User";
        juce::String author = "User";
        juce::String pack = "User Pack";
        juce::String key = "Any Key";
        juce::String bpm = "Any Tempo";
        int notesCharacters = 0;
        bool hasName = false;
        bool generated = false;
        bool overwriteExists = false;
        bool overwriteArmed = false;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static void drawPill(juce::Graphics& g,
                         juce::Rectangle<float> area,
                         const juce::String& label,
                         const juce::String& value,
                         juce::Colour accent);
};
}
