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

bool configureStableSine(NateVSTAudioProcessor& processor)
{
    auto configured = setPlainParameter(processor, Parameters::ID::oscWave, 0.0f)
        && setPlainParameter(processor, Parameters::ID::oscOctave, 0.0f)
        && setPlainParameter(processor, Parameters::ID::oscTune, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerChordMemory, 0.0f)
        && setPlainParameter(processor, Parameters::ID::monoMode, 0.0f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 18000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.1f)
        && setPlainParameter(processor, Parameters::ID::filterEnvAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.01f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.01f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, -12.0f);

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        configured = setPlainParameter(processor, Parameters::ID::modMatrixSource[index], 0.0f) && configured;
        configured = setPlainParameter(processor, Parameters::ID::modMatrixDestination[index], 0.0f) && configured;
        configured = setPlainParameter(processor, Parameters::ID::modMatrixAmount[index], 0.0f) && configured;
    }

    return configured;
}

double estimateFrequency(const std::vector<float>& samples, double sampleRate)
{
    if (samples.size() < 2)
        return 0.0;

    auto risingCrossings = 0;
    for (size_t index = 1; index < samples.size(); ++index)
        if (samples[index - 1] <= 0.0f && samples[index] > 0.0f)
            ++risingCrossings;

    return static_cast<double>(risingCrossings) * sampleRate / static_cast<double>(samples.size() - 1);
}

bool verifyManualAuditionSuspendsSequencer(NateVSTAudioProcessor& processor, double sampleRate, int blockSize, int heldNote)
{
    if (! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerRoot, 48.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerOctave, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f))
        return false;

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        auto step = processor.getSequencerStep(stepIndex);
        step.enabled = false;
        processor.setSequencerStep(stepIndex, step);
    }

    auto firstStep = processor.getSequencerStep(0);
    firstStep.enabled = true;
    firstStep.noteOffset = -12;
    firstStep.probability = 1.0f;
    firstStep.length = 0.5f;
    processor.setSequencerStep(0, firstStep);

    processor.setManualKeyboardAuditionActive(true);
    processor.getMidiKeyboardState().noteOn(1, heldNote, 0.86f);

    auto noteOnCount = 0;
    auto unexpectedNoteOn = false;
    auto injectedHostOctaveEcho = false;
    const auto auditionSamples = static_cast<int>(sampleRate * 3.0);
    for (auto rendered = 0; rendered < auditionSamples; rendered += blockSize)
    {
        const auto currentBlockSize = juce::jmin(blockSize, auditionSamples - rendered);
        juce::AudioBuffer<float> buffer(2, currentBlockSize);
        buffer.clear();
        juce::MidiBuffer midi;
        if (! injectedHostOctaveEcho && rendered >= static_cast<int>(sampleRate * 0.65))
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, heldNote - 12, 0.86f), 0);
            injectedHostOctaveEcho = true;
        }
        processor.processBlock(buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (! message.isNoteOn())
                continue;

            ++noteOnCount;
            unexpectedNoteOn = unexpectedNoteOn || message.getNoteNumber() != heldNote;
        }
    }

    processor.getMidiKeyboardState().noteOff(1, heldNote, 0.0f);
    processor.setManualKeyboardAuditionActive(false);
    juce::AudioBuffer<float> resumeBuffer(2, blockSize);
    resumeBuffer.clear();
    juce::MidiBuffer resumeMidi;
    processor.processBlock(resumeBuffer, resumeMidi);

    auto sequencerResumed = false;
    for (const auto metadata : resumeMidi)
        sequencerResumed = sequencerResumed || metadata.getMessage().isNoteOn();

    const auto lowerKeyWasSuppressed = ! processor.getMidiKeyboardState().isNoteOnForChannels(0xffff, heldNote - 12);
    if (noteOnCount != 1 || unexpectedNoteOn || ! lowerKeyWasSuppressed || ! sequencerResumed)
    {
        std::cerr << "Manual audition isolation failed: note-ons " << noteOnCount
                  << ", unexpected pitch " << unexpectedNoteOn
                  << ", lower key suppressed " << lowerKeyWasSuppressed
                  << ", sequencer resumed " << sequencerResumed << '\n';
        return false;
    }

    return true;
}

bool verifyHostMidiSuspendsSequencer(double sampleRate, int blockSize, int heldNote)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);
    if (! configureStableSine(processor)
        || ! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerRoot, 48.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerOctave, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f))
        return false;

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        auto step = processor.getSequencerStep(stepIndex);
        step.enabled = false;
        processor.setSequencerStep(stepIndex, step);
    }

    auto firstStep = processor.getSequencerStep(0);
    firstStep.enabled = true;
    firstStep.noteOffset = -12;
    firstStep.probability = 1.0f;
    firstStep.length = 0.5f;
    processor.setSequencerStep(0, firstStep);

    auto noteOnCount = 0;
    auto unexpectedNoteOn = false;
    const auto auditionSamples = static_cast<int>(sampleRate * 3.0);
    for (auto rendered = 0; rendered < auditionSamples; rendered += blockSize)
    {
        const auto currentBlockSize = juce::jmin(blockSize, auditionSamples - rendered);
        juce::AudioBuffer<float> buffer(2, currentBlockSize);
        buffer.clear();
        juce::MidiBuffer midi;
        if (rendered == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, heldNote, 0.86f), 0);
        processor.processBlock(buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (! message.isNoteOn())
                continue;

            ++noteOnCount;
            unexpectedNoteOn = unexpectedNoteOn || message.getNoteNumber() != heldNote;
        }
    }

    const auto holdWasReported = processor.isHostMidiAuditionActive();
    const auto heldCountWasReported = processor.getHostMidiHeldNoteCount();
    const auto lastHostNoteWasReported = processor.getHostMidiLastNote();
    juce::AudioBuffer<float> releaseBuffer(2, blockSize);
    releaseBuffer.clear();
    juce::MidiBuffer releaseMidi;
    releaseMidi.addEvent(juce::MidiMessage::noteOff(1, heldNote), 0);
    processor.processBlock(releaseBuffer, releaseMidi);

    auto sequencerResumed = false;
    for (const auto metadata : releaseMidi)
    {
        const auto message = metadata.getMessage();
        sequencerResumed = sequencerResumed
            || (message.isNoteOn() && message.getNoteNumber() != heldNote);
    }

    if (noteOnCount != 1
        || unexpectedNoteOn
        || ! holdWasReported
        || heldCountWasReported != 1
        || lastHostNoteWasReported != heldNote
        || processor.isHostMidiAuditionActive()
        || processor.getHostMidiHeldNoteCount() != 0
        || ! sequencerResumed)
    {
        std::cerr << "Host MIDI audition isolation failed: note-ons " << noteOnCount
                  << ", unexpected pitch " << unexpectedNoteOn
                  << ", hold reported " << holdWasReported
                  << ", held count " << heldCountWasReported
                  << ", last note " << lastHostNoteWasReported
                  << ", sequencer resumed " << sequencerResumed << '\n';
        return false;
    }

    return true;
}
}

int main()
{
    static constexpr auto sampleRate = 44100.0;
    static constexpr auto blockSize = 256;
    static constexpr auto heldNote = 60;
    static constexpr auto renderSeconds = 7.0;
    static constexpr auto totalSamples = static_cast<int>(sampleRate * renderSeconds);

    NateVSTAudioProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);
    if (! configureStableSine(processor))
    {
        std::cerr << "Could not configure held-note stability patch\n";
        return 1;
    }

    processor.getMidiKeyboardState().noteOn(1, heldNote, 0.86f);

    std::vector<float> earlyWindow;
    std::vector<float> lateWindow;
    earlyWindow.reserve(static_cast<size_t>(sampleRate));
    lateWindow.reserve(static_cast<size_t>(sampleRate));

    auto noteOnCount = 0;
    auto unexpectedNoteOn = false;
    auto renderedSamples = 0;
    while (renderedSamples < totalSamples)
    {
        const auto currentBlockSize = juce::jmin(blockSize, totalSamples - renderedSamples);
        juce::AudioBuffer<float> buffer(2, currentBlockSize);
        buffer.clear();
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (! message.isNoteOn())
                continue;

            ++noteOnCount;
            unexpectedNoteOn = unexpectedNoteOn || message.getNoteNumber() != heldNote;
        }

        const auto* samples = buffer.getReadPointer(0);
        for (auto sample = 0; sample < currentBlockSize; ++sample)
        {
            const auto absoluteSample = renderedSamples + sample;
            if (absoluteSample >= static_cast<int>(sampleRate)
                && absoluteSample < static_cast<int>(sampleRate * 2.0))
                earlyWindow.push_back(samples[sample]);
            if (absoluteSample >= static_cast<int>(sampleRate * 5.0)
                && absoluteSample < static_cast<int>(sampleRate * 6.0))
                lateWindow.push_back(samples[sample]);
        }

        renderedSamples += currentBlockSize;
    }

    processor.getMidiKeyboardState().noteOff(1, heldNote, 0.0f);
    juce::AudioBuffer<float> releaseBuffer(2, blockSize);
    releaseBuffer.clear();
    juce::MidiBuffer releaseMidi;
    processor.processBlock(releaseBuffer, releaseMidi);

    const auto earlyFrequency = estimateFrequency(earlyWindow, sampleRate);
    const auto lateFrequency = estimateFrequency(lateWindow, sampleRate);
    const auto frequencyRatio = earlyFrequency > 0.0 ? lateFrequency / earlyFrequency : 0.0;

    if (noteOnCount != 1 || unexpectedNoteOn)
    {
        std::cerr << "Held keyboard note emitted unexpected MIDI: note-ons " << noteOnCount
                  << ", unexpected pitch " << unexpectedNoteOn << '\n';
        return 1;
    }

    if (earlyFrequency < 250.0 || earlyFrequency > 275.0
        || lateFrequency < 250.0 || lateFrequency > 275.0
        || std::abs(frequencyRatio - 1.0) > 0.015)
    {
        std::cerr << "Held note pitch drifted: early " << earlyFrequency
                  << " Hz, late " << lateFrequency << " Hz, ratio " << frequencyRatio << '\n';
        return 1;
    }

    if (! verifyManualAuditionSuspendsSequencer(processor, sampleRate, blockSize, heldNote))
        return 1;

    if (! verifyHostMidiSuspendsSequencer(sampleRate, blockSize, heldNote))
        return 1;

    std::cout << "Held note stability audit passed: one note-on, early " << earlyFrequency
              << " Hz, late " << lateFrequency
              << " Hz, delayed host octave echo suppressed, plugin and host MIDI audition isolated from sequencer.\n";
    return 0;
}
