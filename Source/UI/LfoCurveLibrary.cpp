#include "LfoCurveLibrary.h"

#include <cmath>

namespace UI::LfoCurveLibrary
{
juce::StringArray presetChoices()
{
    return {
        "Manual",
        "Garage Push",
        "Tight Duck",
        "Offbeat Skank",
        "Riser",
        "Fall",
        "Gate Steps",
        "Wobble",
        "UKG Swing",
        "Minimal Pulse",
        "Techno Ramp",
        "House Chug",
        "Flat"
    };
}

std::array<float, 8> presetValues(int presetId)
{
    switch (presetId)
    {
        case 2: return { -0.10f, 0.38f, 0.98f, 0.54f, 0.08f, -0.30f, -0.78f, -0.22f };
        case 3: return { -1.00f, -0.65f, -0.22f, 0.34f, 0.76f, 1.00f, 0.42f, -0.12f };
        case 4: return { -0.36f, 0.14f, 0.82f, 0.26f, -0.22f, 0.58f, 1.00f, -0.48f };
        case 5: return { -1.00f, -0.72f, -0.45f, -0.12f, 0.18f, 0.48f, 0.76f, 1.00f };
        case 6: return { 1.00f, 0.72f, 0.45f, 0.12f, -0.18f, -0.48f, -0.76f, -1.00f };
        case 7: return { 1.00f, 1.00f, -0.65f, -0.65f, 0.80f, 0.80f, -1.00f, -1.00f };
        case 8: return { 0.00f, 1.00f, 0.00f, -1.00f, 0.00f, 0.72f, 0.00f, -0.72f };
        case 9: return { -0.54f, 0.82f, -0.18f, 0.58f, -0.70f, 1.00f, -0.30f, 0.34f };
        case 10: return { -1.00f, -1.00f, 0.85f, -1.00f, -1.00f, -1.00f, 0.42f, -1.00f };
        case 11: return { -0.80f, -0.58f, -0.24f, 0.18f, 0.56f, 0.88f, 1.00f, -0.20f };
        case 12: return { -0.25f, 0.48f, 0.28f, -0.18f, 0.42f, 0.10f, -0.36f, 0.18f };
        case 13: return { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f };
        default: return { 0.00f, 0.58f, 1.00f, 0.42f, -0.18f, -0.72f, -1.00f, -0.36f };
    }
}

double cyclesPerBeatForRateIndex(int rateIndex)
{
    switch (rateIndex)
    {
        case 1: return 2.0;
        case 2: return 3.0;
        case 3: return 4.0;
        default: return 1.0;
    }
}

float evaluateCurve(const std::array<float, 8>& values, float phase)
{
    constexpr auto pointCount = 8;
    const auto scaledPhase = juce::jlimit(0.0f, 0.999999f, phase) * static_cast<float>(pointCount);
    const auto leftIndex = static_cast<int>(std::floor(scaledPhase)) % pointCount;
    const auto rightIndex = (leftIndex + 1) % pointCount;
    const auto fraction = scaledPhase - std::floor(scaledPhase);

    return juce::jlimit(-1.0f,
                        1.0f,
                        values[static_cast<size_t>(leftIndex)]
                            + ((values[static_cast<size_t>(rightIndex)] - values[static_cast<size_t>(leftIndex)]) * fraction));
}

float shapeValue(int shapeIndex, float phase, const std::array<float, 8>& curveValues)
{
    phase = std::fmod(phase + 1.0f, 1.0f);

    switch (shapeIndex)
    {
        case 1:
            return phase < 0.25f ? phase * 4.0f
                : phase < 0.75f ? 2.0f - (phase * 4.0f)
                : (phase * 4.0f) - 4.0f;

        case 2:
            return (phase * 2.0f) - 1.0f;

        case 3:
            return phase < 0.5f ? 1.0f : -1.0f;

        case 4:
            return phase < 0.25f ? 1.0f
                : phase < 0.5f ? -0.35f
                : phase < 0.75f ? 0.55f
                : -1.0f;

        case 5:
            return evaluateCurve(curveValues, phase);

        default:
            return std::sin(juce::MathConstants<float>::twoPi * phase);
    }
}
}
