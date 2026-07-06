#include "SourceWaveLabels.h"

namespace UI::SourceWaveLabels
{
juce::String nameForWave(int wave)
{
    switch (wave)
    {
        case 0: return "Sine";
        case 1: return "Saw";
        case 2: return "Square";
        case 3: return "Triangle";
        case 4: return "Wavetable";
        case 5: return "Organ";
        case 6: return "House Piano";
        case 7: return "Custom";
        default: return "Synth";
    }
}

juce::String bodyRoleForWave(int wave)
{
    switch (wave)
    {
        case 4: return "WT BODY";
        case 5: return "ORGAN";
        case 6: return "KEYS";
        default: return "BODY";
    }
}

juce::String characterRoleForWave(int wave)
{
    switch (wave)
    {
        case 4: return "WT";
        case 5: return "DRAW";
        case 6: return "STAB";
        default: return "CHAR";
    }
}
}
