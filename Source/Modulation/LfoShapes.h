#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cmath>

namespace Modulation::LfoShapes
{
inline constexpr size_t curvePointCount = 8;

using CurveValues = std::array<float, curvePointCount>;
using AtomicCurveValues = std::array<std::atomic<float>*, curvePointCount>;

inline float normalisePhase(float phase) noexcept
{
    phase = std::fmod(phase, 1.0f);
    if (phase < 0.0f)
        phase += 1.0f;

    return phase;
}

inline CurveValues readCurveValues(const AtomicCurveValues& points, float fallback = 0.0f) noexcept
{
    CurveValues values {};
    for (size_t index = 0; index < values.size(); ++index)
        values[index] = points[index] != nullptr ? points[index]->load(std::memory_order_relaxed) : fallback;

    return values;
}

inline float evaluateCurve(const CurveValues& values, float phase) noexcept
{
    const auto scaledPhase = juce::jlimit(0.0f, 0.999999f, normalisePhase(phase)) * static_cast<float>(values.size());
    const auto leftIndex = static_cast<int>(std::floor(scaledPhase)) % static_cast<int>(values.size());
    const auto rightIndex = (leftIndex + 1) % static_cast<int>(values.size());
    const auto fraction = scaledPhase - std::floor(scaledPhase);

    return juce::jlimit(-1.0f,
                        1.0f,
                        values[static_cast<size_t>(leftIndex)]
                            + ((values[static_cast<size_t>(rightIndex)] - values[static_cast<size_t>(leftIndex)]) * fraction));
}

inline float evaluateCurve(const AtomicCurveValues& values, float phase, float fallback = 0.0f) noexcept
{
    return evaluateCurve(readCurveValues(values, fallback), phase);
}

inline float previewStepValue(float phase) noexcept
{
    phase = normalisePhase(phase);
    return phase < 0.25f ? 1.0f
        : phase < 0.5f ? -0.35f
        : phase < 0.75f ? 0.55f
        : -1.0f;
}

inline float shapeValue(int shapeIndex, float phase, float heldStepValue = 0.0f) noexcept
{
    phase = normalisePhase(phase);

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
            return juce::jlimit(-1.0f, 1.0f, heldStepValue);

        default:
            return std::sin(juce::MathConstants<float>::twoPi * phase);
    }
}

inline float shapeValueWithCurve(int shapeIndex,
                                 float phase,
                                 const CurveValues& curveValues,
                                 float heldStepValue = 0.0f) noexcept
{
    if (shapeIndex == 5)
        return evaluateCurve(curveValues, phase);

    return shapeValue(shapeIndex, phase, heldStepValue);
}

inline float shapeValueWithCurve(int shapeIndex,
                                 float phase,
                                 const AtomicCurveValues& curveValues,
                                 float heldStepValue = 0.0f,
                                 float fallback = 0.0f) noexcept
{
    if (shapeIndex == 5)
        return evaluateCurve(curveValues, phase, fallback);

    return shapeValue(shapeIndex, phase, heldStepValue);
}
}
