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
    osc1Level = parameters.getRawParameterValue(Parameters::ID::osc1Level);
    osc2Wave = parameters.getRawParameterValue(Parameters::ID::osc2Wave);
    osc2Octave = parameters.getRawParameterValue(Parameters::ID::osc2Octave);
    osc2Tune = parameters.getRawParameterValue(Parameters::ID::osc2Tune);
    osc2Level = parameters.getRawParameterValue(Parameters::ID::osc2Level);
    subLevel = parameters.getRawParameterValue(Parameters::ID::subLevel);
    noiseLevel = parameters.getRawParameterValue(Parameters::ID::noiseLevel);
    ampAttack = parameters.getRawParameterValue(Parameters::ID::ampAttack);
    ampDecay = parameters.getRawParameterValue(Parameters::ID::ampDecay);
    ampSustain = parameters.getRawParameterValue(Parameters::ID::ampSustain);
    ampRelease = parameters.getRawParameterValue(Parameters::ID::ampRelease);
    filterCutoff = parameters.getRawParameterValue(Parameters::ID::filterCutoff);
    filterResonance = parameters.getRawParameterValue(Parameters::ID::filterResonance);
    filterEnvAmount = parameters.getRawParameterValue(Parameters::ID::filterEnvAmount);
    filterMode = parameters.getRawParameterValue(Parameters::ID::filterMode);
    driveAmount = parameters.getRawParameterValue(Parameters::ID::driveAmount);
}

bool Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<Sound*>(sound) != nullptr;
}

void Voice::prepare(double sampleRate, int maximumBlockSize)
{
    oscillator.prepare(sampleRate);
    oscillator2.prepare(sampleRate);
    subOscillator.prepare(sampleRate);
    subOscillator.setWaveform(Waveform::sine);
    ampEnvelope.setSampleRate(sampleRate);
    filter.prepare(sampleRate, maximumBlockSize);
}

void Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    pitchWheelMoved(currentPitchWheelPosition);
    currentFrequencyHz = frequencyForNote(midiNoteNumber);
    oscillator.reset();
    oscillator2.reset();
    subOscillator.reset();
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

        const auto osc1Gain = readParameter(osc1Level, 1.0f);
        const auto osc2Gain = readParameter(osc2Level, 0.0f);
        const auto subGain = readParameter(subLevel, 0.0f);
        const auto noiseGain = readParameter(noiseLevel, 0.0f);
        const auto sourceWeight = osc1Gain + osc2Gain + (subGain * 0.75f) + (noiseGain * 0.45f);
        const auto sourceCompensation = 1.0f / juce::jmax(1.0f, sourceWeight);
        const auto noiseSample = (noiseRandom.nextFloat() * 2.0f) - 1.0f;

        auto sample = (oscillator.process() * osc1Gain)
            + (oscillator2.process() * osc2Gain)
            + (subOscillator.process() * subGain * 0.75f)
            + (noiseSample * noiseGain * 0.45f);
        sample *= sourceCompensation;
        sample = filter.process(sample);
        sample = distortion.process(sample, readParameter(driveAmount, 0.18f));
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
    const auto waveIndex = static_cast<int>(readParameter(oscWave, 1.0f));
    oscillator.setWaveform(static_cast<Waveform>(juce::jlimit(0, 3, waveIndex)));

    const auto octaveOffset = static_cast<int>(readParameter(oscOctave, 0.0f)) * 12.0f;
    const auto tuneOffset = readParameter(oscTune, 0.0f);
    const auto pitchRatio = std::pow(2.0f, (octaveOffset + tuneOffset + pitchBendSemitones) / 12.0f);
    oscillator.setFrequency(currentFrequencyHz * pitchRatio);

    const auto osc2WaveIndex = static_cast<int>(readParameter(osc2Wave, 1.0f));
    oscillator2.setWaveform(static_cast<Waveform>(juce::jlimit(0, 3, osc2WaveIndex)));

    const auto osc2OctaveOffset = static_cast<int>(readParameter(osc2Octave, 0.0f)) * 12.0f;
    const auto osc2TuneOffset = readParameter(osc2Tune, 0.0f);
    const auto osc2PitchRatio = std::pow(2.0f, (osc2OctaveOffset + osc2TuneOffset + pitchBendSemitones) / 12.0f);
    oscillator2.setFrequency(currentFrequencyHz * osc2PitchRatio);
    subOscillator.setFrequency(currentFrequencyHz * 0.5f);

    ampEnvelope.setParameters(
        readParameter(ampAttack, 0.01f),
        readParameter(ampDecay, 0.18f),
        readParameter(ampSustain, 0.65f),
        readParameter(ampRelease, 0.22f));

    const auto envAmount = readParameter(filterEnvAmount, 0.15f);
    const auto cutoffScale = std::pow(2.0f, envAmount * envelopeValue * 4.0f);
    const auto filterModeIndex = static_cast<int>(readParameter(filterMode, 0.0f));
    filter.setMode(static_cast<Filter::Mode>(juce::jlimit(0, 2, filterModeIndex)));
    filter.setCutoffAndResonance(readParameter(filterCutoff, 1800.0f) * cutoffScale, readParameter(filterResonance, 0.45f));
}

float Voice::frequencyForNote(int midiNoteNumber) const
{
    return static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
}

float Voice::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
