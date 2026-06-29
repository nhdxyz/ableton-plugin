#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class PresetCrateMapDisplay : public juce::Component
{
public:
    struct State
    {
        int visibleCount = 0;
        int totalCount = 0;
        int favoriteCount = 0;
        int ratedCount = 0;
        int factoryCount = 0;
        int generatedCount = 0;
        int macroRichCount = 0;
        int styleCount = 0;
        juce::String filterName = "All";
        juce::String tagName = "All Tags";
        juce::String searchText;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float proportion(int value, int total) noexcept;
    static void drawMetric(juce::Graphics& g,
                           juce::Rectangle<float> area,
                           const juce::String& label,
                           int value,
                           int total,
                           juce::Colour accent);
};
}
