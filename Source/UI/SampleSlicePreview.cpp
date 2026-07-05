#include "SampleSlicePreview.h"

#include <array>
#include <cmath>

namespace UI::SampleSlicePreview
{
namespace
{
constexpr std::array<float, 8> defaultPitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
constexpr std::array<float, 8> defaultGaragePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };
}

Settings defaults(size_t sliceIndex, int styleIndex)
{
    const auto safeIndex = juce::jlimit<size_t>(0, defaultPitchLadder.size() - 1, sliceIndex);
    const auto slicePosition = static_cast<int>(safeIndex);
    Settings settings;

    switch (juce::jlimit(0, 4, styleIndex))
    {
        case 1:
            settings.pitch = defaultPitchLadder[safeIndex];
            settings.gain = -7.0f + static_cast<float>(slicePosition % 3);
            break;

        case 2:
            settings.reverse = (slicePosition % 2) != 0;
            settings.pitch = defaultPitchLadder[static_cast<size_t>((slicePosition + 2) % 8)];
            settings.gain = -7.5f;
            break;

        case 3:
            settings.pitch = defaultGaragePitch[static_cast<size_t>((slicePosition + 1) % 8)] * 0.5f;
            settings.gain = -8.0f;
            settings.stutter = true;
            settings.choke = true;
            settings.repeats = 2 + (slicePosition % 4);
            break;

        case 4:
            settings.reverse = slicePosition == 2 || slicePosition == 6;
            settings.pitch = defaultGaragePitch[safeIndex];
            settings.gain = -8.5f + static_cast<float>(slicePosition % 4) * 0.8f;
            settings.stutter = slicePosition == 3 || slicePosition == 7;
            settings.choke = true;
            settings.repeats = slicePosition == 7 ? 5 : 3;
            break;

        case 0:
        default:
            break;
    }

    return settings;
}

juce::String panText(float pan)
{
    pan = juce::jlimit(-1.0f, 1.0f, pan);
    if (pan < -0.05f)
        return "L" + juce::String(juce::roundToInt(std::abs(pan) * 100.0f));
    if (pan > 0.05f)
        return "R" + juce::String(juce::roundToInt(pan * 100.0f));

    return "C";
}

juce::String chanceText(float probability)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0f, 1.0f, probability) * 100.0f)) + "%";
}

juce::String nudgeText(float nudgePercent)
{
    nudgePercent = juce::jlimit(-5.0f, 5.0f, nudgePercent);
    if (std::abs(nudgePercent) < 0.01f)
        return "N0";

    return juce::String(nudgePercent > 0.0f ? "N+" : "N") + juce::String(nudgePercent, 1) + "%";
}

juce::String fadeText(float fade)
{
    fade = juce::jlimit(0.0f, 1.0f, fade);
    if (fade < 0.12f)
        return "tight";
    if (fade < 0.55f)
        return "soft";

    return "smooth";
}
}
