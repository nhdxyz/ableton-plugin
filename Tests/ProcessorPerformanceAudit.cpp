#include "../Source/PluginProcessor.h"

#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr auto sampleRate = 44100.0;
constexpr auto blockSize = 512;

bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}

bool configureBasePatch(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::oscWave, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Wave, 4.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 0.86f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.62f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.28f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.04f)
        && setPlainParameter(processor, Parameters::ID::unisonVoices, 5.0f)
        && setPlainParameter(processor, Parameters::ID::unisonDetune, 0.48f)
        && setPlainParameter(processor, Parameters::ID::unisonBlend, 0.72f)
        && setPlainParameter(processor, Parameters::ID::unisonSpread, 0.64f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.004f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.16f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 0.68f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.24f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 3200.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.42f)
        && setPlainParameter(processor, Parameters::ID::filterCharacter, 2.0f)
        && setPlainParameter(processor, Parameters::ID::filterSlope, 1.0f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.24f)
        && setPlainParameter(processor, Parameters::ID::lfo1Depth, 0.34f)
        && setPlainParameter(processor, Parameters::ID::lfo2Depth, 0.18f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, -10.0f);
}

bool configureFxRack(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::fxDistortionEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionAmount, 0.38f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionBassSafe, 0.76f)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushBits, 12.0f)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushDownsample, 2.0f)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushMix, 0.18f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpDepth, 0.32f)
        && setPlainParameter(processor, Parameters::ID::fxTremoloEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxTremoloDepth, 0.24f)
        && setPlainParameter(processor, Parameters::ID::fxRingEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxRingDepth, 0.12f)
        && setPlainParameter(processor, Parameters::ID::fxRingMix, 0.18f)
        && setPlainParameter(processor, Parameters::ID::fxCombEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxCombFeedback, 0.34f)
        && setPlainParameter(processor, Parameters::ID::fxCombMix, 0.22f)
        && setPlainParameter(processor, Parameters::ID::fxChorusEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxChorusMix, 0.22f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayFeedback, 0.28f)
        && setPlainParameter(processor, Parameters::ID::fxDelayMix, 0.16f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbSize, 0.42f)
        && setPlainParameter(processor, Parameters::ID::fxReverbMix, 0.14f)
        && setPlainParameter(processor, Parameters::ID::fxWidthEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxWidthAmount, 0.58f)
        && setPlainParameter(processor, Parameters::ID::fxToneEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxToneTilt, 0.18f)
        && setPlainParameter(processor, Parameters::ID::fxEqEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxEqLowGain, 1.5f)
        && setPlainParameter(processor, Parameters::ID::fxEqMidGain, -0.8f)
        && setPlainParameter(processor, Parameters::ID::fxEqHighGain, 1.2f)
        && setPlainParameter(processor, Parameters::ID::fxPhaserEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxPhaserDepth, 0.32f)
        && setPlainParameter(processor, Parameters::ID::fxPhaserMix, 0.22f)
        && setPlainParameter(processor, Parameters::ID::fxFlangerEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxFlangerDepth, 0.24f)
        && setPlainParameter(processor, Parameters::ID::fxFlangerFeedback, 0.22f)
        && setPlainParameter(processor, Parameters::ID::fxFlangerMix, 0.18f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardGlue, 0.42f)
        && setPlainParameter(processor, Parameters::ID::fxGuardPunch, 0.36f)
        && setPlainParameter(processor, Parameters::ID::fxGuardClipMix, 0.72f);
}

Sequencer::Step performanceStep(int noteOffset, float velocity, float timing, float length, int ratchet = 1, bool slide = false)
{
    Sequencer::Step step;
    step.enabled = true;
    step.noteOffset = noteOffset;
    step.velocity = velocity;
    step.probability = 1.0f;
    step.timing = timing;
    step.length = length;
    step.lock = 0.35f;
    step.ratchet = ratchet;
    step.condition = 0;
    step.slide = slide;
    return step;
}

bool configureSequencerScenario(NateVSTAudioProcessor& processor)
{
    if (! configureBasePatch(processor)
        || ! configureFxRack(processor)
        || ! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerRate, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerRoot, 48.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerGate, 0.62f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerSwing, 0.12f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 4.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordStrum, 0.18f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDestination, 7.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDepth, 0.54f))
    {
        return false;
    }

    processor.setSequencerStep(0, performanceStep(0, 0.92f, 0.0f, 0.85f));
    processor.setSequencerStep(2, performanceStep(7, 0.62f, 0.12f, 0.72f, 2));
    processor.setSequencerStep(4, performanceStep(3, 0.74f, 0.0f, 0.78f));
    processor.setSequencerStep(7, performanceStep(10, 0.58f, 0.38f, 0.55f, 3));
    processor.setSequencerStep(8, performanceStep(0, 0.88f, 0.0f, 0.86f));
    processor.setSequencerStep(11, performanceStep(-5, 0.68f, 0.18f, 0.64f, 1, true));
    processor.setSequencerStep(12, performanceStep(3, 0.82f, 0.0f, 0.78f));
    processor.setSequencerStep(15, performanceStep(7, 0.58f, 0.42f, 0.52f, 2));
    return true;
}

struct Scenario
{
    const char* name = "";
    int blocks = 0;
    double maxRealtimeRatio = 1.0;
    float minRms = 0.0f;
    std::function<bool(NateVSTAudioProcessor&)> configure;
    std::function<void(int, juce::MidiBuffer&)> fillMidi;
};

struct Metrics
{
    double elapsedSeconds = 0.0;
    double audioSeconds = 0.0;
    double realtimeRatio = 0.0;
    float peak = 0.0f;
    double rms = 0.0;
    bool finite = true;
};

Metrics runScenario(const Scenario& scenario)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    if (scenario.configure && ! scenario.configure(processor))
        throw std::runtime_error(std::string("Could not configure scenario: ") + scenario.name);

    juce::AudioBuffer<float> buffer(2, blockSize);
    auto sumSquares = 0.0;
    auto sampleCount = 0;
    Metrics metrics;
    metrics.audioSeconds = static_cast<double>(scenario.blocks * blockSize) / sampleRate;

    const auto started = std::chrono::steady_clock::now();
    for (auto block = 0; block < scenario.blocks; ++block)
    {
        buffer.clear();
        juce::MidiBuffer midi;
        if (scenario.fillMidi)
            scenario.fillMidi(block, midi);

        processor.processBlock(buffer, midi);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const auto value = samples[sample];
                metrics.finite = metrics.finite && std::isfinite(value);
                metrics.peak = juce::jmax(metrics.peak, std::abs(value));
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;
            }
        }
    }

    metrics.elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    metrics.realtimeRatio = metrics.audioSeconds > 0.0 ? metrics.elapsedSeconds / metrics.audioSeconds : 0.0;
    metrics.rms = sampleCount > 0 ? std::sqrt(sumSquares / static_cast<double>(sampleCount)) : 0.0;
    return metrics;
}
}

int main()
{
    const std::array<int, 6> chordNotes { 36, 43, 48, 52, 55, 60 };
    const std::vector<Scenario> scenarios {
        {
            "idle",
            220,
            0.75,
            0.0f,
            [] (NateVSTAudioProcessor& processor)
            {
                return configureBasePatch(processor)
                    && setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
                    && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
                    && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
                    && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
                    && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
                    && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f);
            },
            nullptr
        },
        {
            "poly_synth_fx",
            320,
            3.0,
            0.0002f,
            [] (NateVSTAudioProcessor& processor)
            {
                return configureBasePatch(processor) && configureFxRack(processor);
            },
            [&chordNotes] (int block, juce::MidiBuffer& midi)
            {
                if (block == 0)
                {
                    for (const auto note : chordNotes)
                        midi.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(104)), 0);
                }
                else if (block == 220)
                {
                    for (const auto note : chordNotes)
                        midi.addEvent(juce::MidiMessage::noteOff(1, note), 0);
                }
            }
        },
        {
            "sequencer_chords_fx",
            360,
            3.0,
            0.0002f,
            configureSequencerScenario,
            nullptr
        }
    };

    for (const auto& scenario : scenarios)
    {
        Metrics metrics;
        try
        {
            metrics = runScenario(scenario);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << '\n';
            return 1;
        }

        std::cout << scenario.name << ": rendered " << metrics.audioSeconds << "s audio in "
                  << metrics.elapsedSeconds << "s, realtime ratio " << metrics.realtimeRatio
                  << ", peak " << metrics.peak << ", rms " << metrics.rms << '\n';

        if (! metrics.finite || metrics.peak > 1.5f || metrics.rms < scenario.minRms)
        {
            std::cerr << "Processor performance scenario rendered invalid output: " << scenario.name
                      << " finite " << metrics.finite
                      << " peak " << metrics.peak
                      << " rms " << metrics.rms << '\n';
            return 1;
        }

        if (metrics.realtimeRatio > scenario.maxRealtimeRatio)
        {
            std::cerr << "Processor performance scenario exceeded realtime ratio budget: " << scenario.name
                      << " ratio " << metrics.realtimeRatio
                      << " budget " << scenario.maxRealtimeRatio << '\n';
            return 1;
        }
    }

    std::cout << "Processor performance audit passed.\n";
    return 0;
}
