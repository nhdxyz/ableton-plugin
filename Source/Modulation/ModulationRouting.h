#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <optional>

namespace Modulation
{
inline constexpr int stepLfoSourceIndex = 20;

inline float readParameter(std::atomic<float>* parameter, float fallback) noexcept
{
    return parameter != nullptr ? parameter->load(std::memory_order_relaxed) : fallback;
}

inline double cyclesPerBeatForSyncRate(int rateIndex) noexcept
{
    switch (rateIndex)
    {
        case 1: return 2.0; // 1/8
        case 2: return 3.0; // 1/8 triplet
        case 3: return 4.0; // 1/16
        default: return 1.0; // 1/4
    }
}

inline std::optional<float> phaseFromPpq(std::optional<double> ppqPosition, double cyclesPerBeat) noexcept
{
    if (! ppqPosition.has_value())
        return std::nullopt;

    auto phase = std::fmod(*ppqPosition * cyclesPerBeat, 1.0);
    if (phase < 0.0)
        phase += 1.0;

    return static_cast<float>(phase);
}

inline bool isSynthDestination(int destinationIndex) noexcept
{
    switch (destinationIndex)
    {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 17:
        case 18:
        case 19:
            return true;

        default:
            return false;
    }
}

inline bool isFxDestination(int destinationIndex) noexcept
{
    switch (destinationIndex)
    {
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 20:
        case 21:
            return true;

        default:
            return false;
    }
}

inline bool isSampleDestination(int destinationIndex) noexcept
{
    switch (destinationIndex)
    {
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            return true;

        default:
            return false;
    }
}

inline float smoothingAlpha(float amount, int numSamples, double sampleRate, float maximumSeconds) noexcept
{
    const auto safeAmount = juce::jlimit(0.0f, 1.0f, amount);
    if (safeAmount <= 0.001f)
        return 1.0f;

    const auto seconds = 0.004f + (safeAmount * juce::jmax(0.004f, maximumSeconds));
    const auto safeSamples = static_cast<float>(juce::jmax(1, numSamples));
    const auto safeSampleRate = static_cast<float>(juce::jmax(1.0, sampleRate));
    return juce::jlimit(0.0f, 1.0f, 1.0f - std::exp(-safeSamples / (safeSampleRate * seconds)));
}

inline float applyPolarity(float value, int polarityIndex) noexcept
{
    switch (polarityIndex)
    {
        case 1:
            return juce::jlimit(0.0f, 1.0f, (value + 1.0f) * 0.5f);

        case 2:
            return -juce::jlimit(0.0f, 1.0f, (value + 1.0f) * 0.5f);

        case 3:
            return -value;

        case 0:
        default:
            return value;
    }
}

inline float applyCurve(float value, int curveIndex) noexcept
{
    const auto sign = value < 0.0f ? -1.0f : 1.0f;
    const auto magnitude = juce::jlimit(0.0f, 1.0f, std::abs(value));

    switch (curveIndex)
    {
        case 1:
            return sign * std::sqrt(magnitude);

        case 2:
            return sign * std::pow(magnitude, 1.75f);

        case 3:
            return sign * std::pow(magnitude, 3.0f);

        case 4:
            return magnitude >= 0.5f ? sign : 0.0f;

        case 0:
        default:
            return value;
    }
}

inline float processRouteValue(float rawValue,
                               std::atomic<float>* polarity,
                               std::atomic<float>* curve,
                               std::atomic<float>* rangeMin,
                               std::atomic<float>* rangeMax,
                               std::atomic<float>* slew,
                               float& smoothedValue,
                               int numSamples,
                               double sampleRate) noexcept
{
    const auto polarityIndex = juce::roundToInt(readParameter(polarity, 0.0f));
    const auto curveIndex = juce::roundToInt(readParameter(curve, 0.0f));
    auto minimum = readParameter(rangeMin, -1.0f);
    auto maximum = readParameter(rangeMax, 1.0f);
    if (minimum > maximum)
        std::swap(minimum, maximum);

    auto shapedValue = rawValue;
    if (polarityIndex != 0)
        shapedValue = applyPolarity(shapedValue, polarityIndex);
    if (curveIndex != 0)
        shapedValue = applyCurve(shapedValue, curveIndex);
    shapedValue = juce::jlimit(juce::jlimit(-1.0f, 1.0f, minimum),
                               juce::jlimit(-1.0f, 1.0f, maximum),
                               juce::jlimit(-1.0f, 1.0f, shapedValue));

    const auto slewAmount = readParameter(slew, 0.0f);
    if (slewAmount <= 0.001f)
    {
        smoothedValue = shapedValue;
        return shapedValue;
    }

    const auto alpha = smoothingAlpha(slewAmount, numSamples, sampleRate, 0.45f);
    smoothedValue += (shapedValue - smoothedValue) * alpha;
    return smoothedValue;
}

inline float processStepLfo(std::atomic<float>* sync,
                            std::atomic<float>* syncRate,
                            std::atomic<float>* rate,
                            std::atomic<float>* depth,
                            std::atomic<float>* slew,
                            const std::array<std::atomic<float>*, 8>& values,
                            float& phase,
                            float& smoothedValue,
                            int numSamples,
                            double sampleRate,
                            double bpm,
                            std::optional<double> ppqPosition = std::nullopt) noexcept
{
    const auto syncEnabled = readParameter(sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = juce::roundToInt(readParameter(syncRate, 3.0f));
    const auto cyclesPerBeat = cyclesPerBeatForSyncRate(syncRateIndex);

    if (syncEnabled)
    {
        if (const auto syncedPhase = phaseFromPpq(ppqPosition, cyclesPerBeat))
            phase = *syncedPhase;
    }

    const auto stepPhase = juce::jlimit(0.0f, 0.999999f, phase);
    const auto stepIndex = static_cast<size_t>(juce::jlimit(0,
                                                            static_cast<int>(values.size() - 1),
                                                            static_cast<int>(std::floor(stepPhase * static_cast<float>(values.size())))));
    const auto target = juce::jlimit(-1.0f, 1.0f, readParameter(values[stepIndex], 0.0f))
        * juce::jlimit(0.0f, 1.0f, readParameter(depth, 0.55f));

    const auto slewAmount = readParameter(slew, 0.0f);
    if (slewAmount <= 0.001f)
    {
        smoothedValue = target;
    }
    else
    {
        const auto alpha = smoothingAlpha(slewAmount, numSamples, sampleRate, 0.28f);
        smoothedValue += (target - smoothedValue) * alpha;
    }

    const auto rateHz = syncEnabled
        ? static_cast<float>((juce::jlimit(20.0, 300.0, bpm) / 60.0) * cyclesPerBeat)
        : juce::jlimit(0.05f, 40.0f, readParameter(rate, 2.0f));
    phase += (rateHz * static_cast<float>(juce::jmax(1, numSamples)))
        / static_cast<float>(juce::jmax(1.0, sampleRate));
    if (phase >= 1.0f)
        phase -= std::floor(phase);

    return juce::jlimit(-1.0f, 1.0f, smoothedValue);
}
}
