#include "OutputTelemetry.h"

#include <cmath>

namespace UI::OutputTelemetry
{
juce::String outputSafetySummary(float peak)
{
    const auto safePeak = juce::jlimit(0.0f, 2.0f, peak);
    const auto peakDb = safePeak <= 0.000001f ? -60.0f
                                              : juce::Decibels::gainToDecibels(safePeak);

    if (safePeak >= 0.995f)
        return "clip risk " + juce::String(peakDb, 1) + " dB";

    if (peakDb >= -3.0f)
        return "hot peak " + juce::String(peakDb, 1) + " dB";

    if (peakDb <= -30.0f)
        return "low peak " + juce::String(peakDb, 1) + " dB";

    return "safe peak " + juce::String(peakDb, 1) + " dB";
}

juce::String guardSafetySummary(float peak, float guardReduction, bool guardActive)
{
    if (guardActive && guardReduction > 0.005f)
        return "guard -" + juce::String(juce::roundToInt(guardReduction * 100.0f)) + "% | " + outputSafetySummary(peak);

    return outputSafetySummary(peak);
}

float smoothMeterValue(float current, float target)
{
    target = juce::jlimit(0.0f, 2.0f, target);
    const auto coefficient = target > current ? 0.65f : 0.18f;
    return current + ((target - current) * coefficient);
}

const OutputSpectrumDisplay::BandArray& spectrumBandFrequencies() noexcept
{
    static constexpr OutputSpectrumDisplay::BandArray frequencies {
        45.0f,
        70.0f,
        105.0f,
        160.0f,
        250.0f,
        390.0f,
        620.0f,
        1000.0f,
        1700.0f,
        3100.0f,
        5800.0f,
        11000.0f
    };

    return frequencies;
}

float goertzelBandLevel(const std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize>& samples,
                        double sampleRate,
                        float frequencyHz)
{
    if (sampleRate <= 0.0 || frequencyHz <= 0.0f)
        return 0.0f;

    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double>(frequencyHz) / sampleRate;
    const auto coefficient = 2.0 * std::cos(omega);
    auto q1 = 0.0;
    auto q2 = 0.0;

    for (const auto sample : samples)
    {
        const auto q0 = (coefficient * q1) - q2 + static_cast<double>(sample);
        q2 = q1;
        q1 = q0;
    }

    const auto real = q1 - (q2 * std::cos(omega));
    const auto imaginary = q2 * std::sin(omega);
    const auto magnitude = std::sqrt((real * real) + (imaginary * imaginary))
        / static_cast<double>(samples.size());
    const auto decibels = juce::Decibels::gainToDecibels(static_cast<float>(magnitude) + 0.000001f);

    return juce::jlimit(0.0f, 1.0f, juce::jmap(decibels, -78.0f, -18.0f, 0.0f, 1.0f));
}
}
