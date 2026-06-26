#include "Voice.h"

#include <cmath>

namespace Synth
{
Voice::Voice(Parameters::APVTS& state)
    : parameters(state)
{
    oscWave = parameters.getRawParameterValue(Parameters::ID::oscWave);
    oscOctave = parameters.getRawParameterValue(Parameters::ID::oscOctave);
    oscTune = parameters.getRawParameterValue(Parameters::ID::oscTune);
    ampAttack = parameters.getRawParameterValue(Parameters::ID::ampAttack);
    ampDecay = parameters.getRawParameterValue(Parameters::ID::ampDecay);
    ampSustain = parameters.getRawParameterValue(Parameters::ID::ampSustain);
    ampRelease = parameters.getRawParameterValue(Parameters::ID::ampRelease);
    filterCutoff = parameters.getRawParameterValue(Parameters::ID::filterCutoff);
    filterResonance = parameters.getRawParameterValue(Parameters::ID::filterResonance);
    filterEnvAmount = parameters.getRawParameterValue(Parameters::ID::filterEnvAmount);
    driveAmount = parameters.getRawParameterValue(Parameters::ID::driveAmount);
}

bool Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<Sound*>(sound) != nullptr;
}

void Voice::prepare(double sampleRate, int maximumBlockSize)
{
    oscillator.prepare(sampleRate);
    ampEnvelope.setSampleRate(sampleRate);
    filter.prepare(sampleRate, maximumBlockSize);
}

void Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    pitchWheelMoved(currentPitchWheelPosition);
    currentFrequencyHz = frequencyForNote(midiNoteNumber);
    oscillator.reset();
    filter.reset();
    ampEnvelope.reset();
    ampEnvelope.noteOn();
}

void Voice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
        return;
    }

    clearCurrentNote();
    ampEnvelope.reset();
}

void Voice::pitchWheelMoved(int newPitchWheelValue)
{
    const auto normalised = (static_cast<float>(newPitchWheelValue) - 8192.0f) / 8192.0f;
    pitchBendSemitones = juce::jlimit(-1.0f, 1.0f, normalised) * 2.0f;
}

void Voice::controllerMoved(int, int)
{
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto envelopeValue = ampEnvelope.process();
        updateVoiceParameters(envelopeValue);

        auto sample = oscillator.process();
        sample = filter.process(sample);
        sample = distortion.process(sample, driveAmount->load());
        sample *= envelopeValue * noteVelocity * 0.28f;

        for (auto channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addSample(channel, startSample + sampleIndex, sample);

        if (! ampEnvelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

void Voice::updateVoiceParameters(float envelopeValue)
{
    const auto waveIndex = static_cast<int>(oscWave->load());
    oscillator.setWaveform(static_cast<Waveform>(juce::jlimit(0, 3, waveIndex)));

    const auto octaveOffset = static_cast<int>(oscOctave->load()) * 12.0f;
    const auto tuneOffset = oscTune->load();
    const auto pitchRatio = std::pow(2.0f, (octaveOffset + tuneOffset + pitchBendSemitones) / 12.0f);
    oscillator.setFrequency(currentFrequencyHz * pitchRatio);

    ampEnvelope.setParameters(
        ampAttack->load(),
        ampDecay->load(),
        ampSustain->load(),
        ampRelease->load());

    const auto envAmount = filterEnvAmount->load();
    const auto cutoffScale = std::pow(2.0f, envAmount * envelopeValue * 4.0f);
    filter.setCutoffAndResonance(filterCutoff->load() * cutoffScale, filterResonance->load());
}

float Voice::frequencyForNote(int midiNoteNumber) const
{
    return static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
}
}
