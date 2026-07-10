#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}

struct RenderStats
{
    std::vector<float> samples;
    float rms = 0.0f;
    float peak = 0.0f;
    bool finite = true;
};

bool configurePatch(NateVSTAudioProcessor& processor, int mode)
{
    const std::pair<const char*, float> parameters[] {
        { Parameters::ID::oscWave, 1.0f },
        { Parameters::ID::osc1Level, 0.72f },
        { Parameters::ID::osc2Wave, 0.0f },
        { Parameters::ID::osc2Octave, 1.0f },
        { Parameters::ID::osc2Tune, 7.0f },
        { Parameters::ID::osc2Level, 0.0f },
        { Parameters::ID::oscCrossModMode, static_cast<float>(mode) },
        { Parameters::ID::oscCrossModAmount, mode == 0 ? 0.0f : 0.72f },
        { Parameters::ID::subLevel, 0.0f },
        { Parameters::ID::noiseLevel, 0.0f },
        { Parameters::ID::ampAttack, 0.001f },
        { Parameters::ID::ampDecay, 1.0f },
        { Parameters::ID::ampSustain, 1.0f },
        { Parameters::ID::ampRelease, 0.1f },
        { Parameters::ID::filterCutoff, 20000.0f },
        { Parameters::ID::filterResonance, 0.1f },
        { Parameters::ID::filterEnvAmount, 0.0f },
        { Parameters::ID::driveAmount, 0.0f },
        { Parameters::ID::outputGain, -6.0f },
        { Parameters::ID::sampleEnabled, 0.0f },
        { Parameters::ID::sequencerEnabled, 0.0f },
        { Parameters::ID::fxDistortionEnabled, 0.0f },
        { Parameters::ID::fxBitcrushEnabled, 0.0f },
        { Parameters::ID::fxPumpEnabled, 0.0f },
        { Parameters::ID::fxTremoloEnabled, 0.0f },
        { Parameters::ID::fxRingEnabled, 0.0f },
        { Parameters::ID::fxCombEnabled, 0.0f },
        { Parameters::ID::fxChorusEnabled, 0.0f },
        { Parameters::ID::fxDelayEnabled, 0.0f },
        { Parameters::ID::fxReverbEnabled, 0.0f },
        { Parameters::ID::fxWidthEnabled, 0.0f },
        { Parameters::ID::fxToneEnabled, 0.0f },
        { Parameters::ID::fxEqEnabled, 0.0f },
        { Parameters::ID::fxPhaserEnabled, 0.0f },
        { Parameters::ID::fxFlangerEnabled, 0.0f },
        { Parameters::ID::fxGuardEnabled, 0.0f }
    };

    for (const auto& [id, value] : parameters)
        if (! setPlainParameter(processor, id, value))
            return false;

    return true;
}

RenderStats renderMode(int mode)
{
    constexpr auto blockSize = 512;
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, blockSize);

    RenderStats stats;
    if (! configurePatch(processor, mode))
    {
        stats.finite = false;
        return stats;
    }

    juce::AudioBuffer<float> buffer(2, blockSize);
    stats.samples.reserve(16 * blockSize);
    double sumSquares = 0.0;

    for (auto blockIndex = 0; blockIndex < 20; ++blockIndex)
    {
        buffer.clear();
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 48, static_cast<juce::uint8>(110)), 0);

        processor.processBlock(buffer, midi);
        if (blockIndex < 4)
            continue;

        const auto* channel = buffer.getReadPointer(0);
        for (auto sampleIndex = 0; sampleIndex < blockSize; ++sampleIndex)
        {
            const auto sample = channel[sampleIndex];
            if (! std::isfinite(sample))
                stats.finite = false;
            stats.peak = std::max(stats.peak, std::abs(sample));
            sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
            stats.samples.push_back(sample);
        }
    }

    if (! stats.samples.empty())
        stats.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(stats.samples.size())));
    return stats;
}

float meanAbsoluteDifference(const RenderStats& first, const RenderStats& second)
{
    const auto count = std::min(first.samples.size(), second.samples.size());
    if (count == 0)
        return 0.0f;

    double sum = 0.0;
    for (size_t index = 0; index < count; ++index)
        sum += std::abs(first.samples[index] - second.samples[index]);
    return static_cast<float>(sum / static_cast<double>(count));
}
}

int main()
{
    const auto choices = Parameters::oscCrossModModeChoices();
    if (choices != juce::StringArray { "Off", "FM", "Phase", "AM", "Ring" })
    {
        std::cerr << "Oscillator cross-mod choices changed unexpectedly\n";
        return 1;
    }

    std::array<RenderStats, 5> renders;
    for (auto mode = 0; mode < static_cast<int>(renders.size()); ++mode)
    {
        renders[static_cast<size_t>(mode)] = renderMode(mode);
        const auto& stats = renders[static_cast<size_t>(mode)];
        if (! stats.finite || stats.rms <= 0.001f || stats.peak <= 0.005f || stats.peak > 1.1f)
        {
            std::cerr << choices[mode] << " rendered outside the audible/safe range: rms "
                      << stats.rms << " peak " << stats.peak << '\n';
            return 1;
        }
    }

    for (auto mode = 1; mode < static_cast<int>(renders.size()); ++mode)
    {
        const auto difference = meanAbsoluteDifference(renders[0], renders[static_cast<size_t>(mode)]);
        if (difference <= 0.0025f)
        {
            std::cerr << choices[mode] << " was not audibly distinct from Off: mean difference "
                      << difference << '\n';
            return 1;
        }
    }

    if (meanAbsoluteDifference(renders[1], renders[2]) <= 0.0025f)
    {
        std::cerr << "FM and Phase rendered too similarly\n";
        return 1;
    }

    std::cout << "Oscillator cross-mod audit passed for hidden modulation, mode contrast, and output safety.\n";
    return 0;
}
