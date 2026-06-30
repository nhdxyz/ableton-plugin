#include "../Source/PluginProcessor.h"

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

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

bool configureHeavyPatch(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::oscWave, 4.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Wave, 6.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.9f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.25f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.08f)
        && setPlainParameter(processor, Parameters::ID::unisonVoices, 7.0f)
        && setPlainParameter(processor, Parameters::ID::unisonDetune, 0.65f)
        && setPlainParameter(processor, Parameters::ID::unisonBlend, 0.82f)
        && setPlainParameter(processor, Parameters::ID::unisonSpread, 0.72f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 4200.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.55f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.35f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.08f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 0.82f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.38f)
        && setPlainParameter(processor, Parameters::ID::lfo1Depth, 0.38f)
        && setPlainParameter(processor, Parameters::ID::lfo2Depth, 0.24f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardGlue, 0.35f)
        && setPlainParameter(processor, Parameters::ID::fxGuardClipMix, 0.85f)
        && setPlainParameter(processor, Parameters::ID::outputGain, -8.0f);
}
}

int main()
{
    static constexpr auto sampleRate = 44100.0;
    static constexpr auto blockSize = 512;
    static constexpr auto totalBlocks = 260;
    static constexpr std::array<int, 12> notes { 36, 38, 41, 43, 45, 48, 50, 53, 55, 57, 60, 62 };

    NateVSTAudioProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    if (! configureHeavyPatch(processor))
    {
        std::cerr << "Could not configure heavy keyboard-spam audit patch\n";
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, blockSize);
    auto peak = 0.0f;
    auto sumSquares = 0.0;
    auto sampleCount = 0;
    auto finite = true;

    const auto started = std::chrono::steady_clock::now();
    for (auto block = 0; block < totalBlocks; ++block)
    {
        buffer.clear();
        juce::MidiBuffer midi;

        if (block < 130)
        {
            for (auto event = 0; event < 4; ++event)
            {
                const auto note = notes[static_cast<size_t>((block * 4 + event) % static_cast<int>(notes.size()))];
                const auto offset = event * 96;
                midi.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(112)), offset);
                midi.addEvent(juce::MidiMessage::noteOff(1, note), juce::jmin(blockSize - 1, offset + 48));
            }
        }

        processor.processBlock(buffer, midi);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const auto value = samples[sample];
                finite = finite && std::isfinite(value);
                peak = juce::jmax(peak, std::abs(value));
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;
            }
        }
    }

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    const auto rms = sampleCount > 0 ? std::sqrt(sumSquares / static_cast<double>(sampleCount)) : 0.0;

    if (! finite || peak > 1.35f || rms < 0.00025)
    {
        std::cerr << "Keyboard spam audit rendered invalid audio: finite " << finite
                  << " peak " << peak << " rms " << rms << '\n';
        return 1;
    }

    if (elapsed > 5.0)
    {
        std::cerr << "Keyboard spam audit exceeded CPU safety window: " << elapsed << "s\n";
        return 1;
    }

    std::cout << "Keyboard spam CPU audit passed in " << elapsed << "s, peak " << peak
              << ", rms " << rms << ".\n";
    return 0;
}
