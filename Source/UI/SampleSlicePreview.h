#pragma once

#include <juce_core/juce_core.h>

namespace UI::SampleSlicePreview
{
struct Settings
{
    float pitch = 0.0f;
    float gain = -6.0f;
    bool reverse = false;
    bool stutter = false;
    bool choke = false;
    int repeats = 3;
    float pan = 0.0f;
    float probability = 1.0f;
    float nudgePercent = 0.0f;
    float fade = 0.0f;
};

Settings defaults(size_t sliceIndex, int styleIndex);
juce::String panText(float pan);
juce::String chanceText(float probability);
juce::String nudgeText(float nudgePercent);
juce::String fadeText(float fade);
}
