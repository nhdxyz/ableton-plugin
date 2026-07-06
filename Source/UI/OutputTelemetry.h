#pragma once

#include "../PluginProcessor.h"
#include "OutputSpectrumDisplay.h"

#include <array>

namespace UI::OutputTelemetry
{
juce::String outputSafetySummary(float peak);
juce::String guardSafetySummary(float peak, float guardReduction, bool guardActive);
float smoothMeterValue(float current, float target);

const OutputSpectrumDisplay::BandArray& spectrumBandFrequencies() noexcept;
float goertzelBandLevel(const std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize>& samples,
                        double sampleRate,
                        float frequencyHz);
}
