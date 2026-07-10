#include "../Source/PluginProcessor.h"

#include <array>
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

struct BandEnergy
{
    float lowRms = 0.0f;
    float highRms = 0.0f;
    float peak = 0.0f;
};

BandEnergy measureBandEnergy(const juce::AudioBuffer<float>& buffer)
{
    constexpr auto sampleRate = 44100.0f;
    constexpr auto cutoffHz = 165.0f;
    const auto alpha = 1.0f - std::exp((-juce::MathConstants<float>::twoPi * cutoffHz) / sampleRate);
    double lowSquares = 0.0;
    double highSquares = 0.0;
    auto peak = 0.0f;
    auto sampleCount = 0;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto lowState = 0.0f;
        const auto* samples = buffer.getReadPointer(channel);

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto sample = samples[sampleIndex];
            lowState += alpha * (sample - lowState);
            const auto high = sample - lowState;
            lowSquares += static_cast<double>(lowState) * static_cast<double>(lowState);
            highSquares += static_cast<double>(high) * static_cast<double>(high);
            peak = juce::jmax(peak, std::abs(sample));
            ++sampleCount;
        }
    }

    if (sampleCount <= 0)
        return {};

    return {
        static_cast<float>(std::sqrt(lowSquares / static_cast<double>(sampleCount))),
        static_cast<float>(std::sqrt(highSquares / static_cast<double>(sampleCount))),
        peak
    };
}

bool configureProcessor(NateVSTAudioProcessor& processor, float driveAmount, float bassSafe)
{
    return setPlainParameter(processor, Parameters::ID::oscWave, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.24f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 20000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.1f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.02f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.1f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, -5.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionEnabled, driveAmount > 0.0f ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionAmount, driveAmount)
        && setPlainParameter(processor, Parameters::ID::fxDistortionBassSafe, bassSafe)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxTremoloEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxRingEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxCombEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPhaserEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxFlangerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxChorusEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendDelay, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendReverb, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxWidthEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxToneEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxEqEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f);
}

BandEnergy renderFixture(float driveAmount, float bassSafe)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 4096);

    if (! configureProcessor(processor, driveAmount, bassSafe))
        return {};

    juce::AudioBuffer<float> buffer(2, 4096);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, static_cast<juce::uint8>(120)), 0);
    processor.processBlock(buffer, midi);
    return measureBandEnergy(buffer);
}

BandEnergy renderMultibandFixture(float lowBand, float midBand, float highBand, float mix)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 4096);

    if (! configureProcessor(processor, 0.72f, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxDistortionMode, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxDistortionLowBand, lowBand)
        || ! setPlainParameter(processor, Parameters::ID::fxDistortionMidBand, midBand)
        || ! setPlainParameter(processor, Parameters::ID::fxDistortionHighBand, highBand)
        || ! setPlainParameter(processor, Parameters::ID::fxDistortionMix, mix))
        return {};

    juce::AudioBuffer<float> buffer(2, 4096);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, static_cast<juce::uint8>(120)), 0);
    processor.processBlock(buffer, midi);
    return measureBandEnergy(buffer);
}
}

int main()
{
    const auto clean = renderFixture(0.0f, 0.0f);
    const auto fullBand = renderFixture(0.72f, 0.0f);
    const auto bassSafe = renderFixture(0.72f, 1.0f);

    if (clean.lowRms <= 0.0001f || clean.highRms <= 0.0001f)
    {
        std::cerr << "Clean fixture did not contain both low and high energy\n";
        return 1;
    }

    const auto fullLowError = std::abs(fullBand.lowRms - clean.lowRms);
    const auto bassSafeLowError = std::abs(bassSafe.lowRms - clean.lowRms);
    if (bassSafeLowError >= fullLowError * 0.82f)
    {
        std::cerr << "Bass Safe Drive did not keep the low band closer to clean than full-band drive: clean "
                  << clean.lowRms << " full " << fullBand.lowRms
                  << " safe " << bassSafe.lowRms << '\n';
        return 1;
    }

    if (bassSafe.lowRms < clean.lowRms * 0.82f || bassSafe.lowRms > clean.lowRms * 1.55f)
    {
        std::cerr << "Bass Safe Drive moved low energy too far from the clean reference: clean "
                  << clean.lowRms << " safe " << bassSafe.lowRms << '\n';
        return 1;
    }

    if (bassSafe.highRms <= clean.highRms * 1.08f)
    {
        std::cerr << "Bass Safe Drive did not add enough high-band saturation: clean "
                  << clean.highRms << " safe " << bassSafe.highRms << '\n';
        return 1;
    }

    if (bassSafe.peak > 1.0f)
    {
        std::cerr << "Bass Safe Drive exceeded unity peak before Guard: "
                  << bassSafe.peak << '\n';
        return 1;
    }

    const auto lowBand = renderMultibandFixture(1.0f, 0.0f, 0.0f, 1.0f);
    const auto midBand = renderMultibandFixture(0.0f, 1.0f, 0.0f, 1.0f);
    const auto highBand = renderMultibandFixture(0.0f, 0.0f, 1.0f, 1.0f);
    const auto parallelDry = renderMultibandFixture(1.0f, 1.0f, 1.0f, 0.0f);

    const auto lowMidContrast = std::abs(lowBand.lowRms - midBand.lowRms)
        + std::abs(lowBand.highRms - midBand.highRms);
    const auto midHighContrast = std::abs(midBand.lowRms - highBand.lowRms)
        + std::abs(midBand.highRms - highBand.highRms);
    if (lowMidContrast <= 0.004f || midHighContrast <= 0.004f)
    {
        std::cerr << "Multiband drive controls did not produce distinct band balances: low/mid "
                  << lowMidContrast << " mid/high " << midHighContrast << '\n';
        return 1;
    }

    if (std::abs(parallelDry.lowRms - clean.lowRms) > clean.lowRms * 0.015f
        || std::abs(parallelDry.highRms - clean.highRms) > clean.highRms * 0.015f)
    {
        std::cerr << "Multiband drive at zero mix did not preserve the dry signal: clean "
                  << clean.lowRms << '/' << clean.highRms << " dry "
                  << parallelDry.lowRms << '/' << parallelDry.highRms << '\n';
        return 1;
    }

    for (const auto* stats : { &lowBand, &midBand, &highBand, &parallelDry })
    {
        if (stats->peak > 1.0f || stats->lowRms <= 0.0001f || stats->highRms <= 0.0001f)
        {
            std::cerr << "Multiband drive rendered outside the audible/safe range\n";
            return 1;
        }
    }

    std::cout << "Drive audit passed for bass safety, independent three-band saturation, parallel mix, and output safety.\n";
    return 0;
}
