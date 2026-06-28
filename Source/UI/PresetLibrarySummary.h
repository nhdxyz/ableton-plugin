#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class PresetLibrarySummary : public juce::Component
{
public:
    struct State
    {
        int visibleCount = 0;
        int totalCount = 0;
        int favoriteCount = 0;
        int ratedCount = 0;
        int factoryCount = 0;
        int userCount = 0;
        bool hasSelection = false;
        bool selectedFavorite = false;
        bool selectedFactory = false;
        juce::String selectedName = "No preset selected";
        juce::String selectedCategory = "-";
        juce::String selectedPack = "-";
        juce::String selectedKey = "Any Key";
        juce::String selectedBpm = "Any Tempo";
        juce::String selectedRating = "-";
        juce::String selectedSource = "-";
        juce::String selectedNotes;
        std::array<float, 8> macroValues {};
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float clamp01(float value) noexcept;
    static juce::Colour accentForIndex(size_t index);
    static void drawStat(juce::Graphics& g,
                         juce::Rectangle<float> area,
                         const juce::String& label,
                         const juce::String& value,
                         juce::Colour accent);
};
}
