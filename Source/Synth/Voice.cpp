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
    monoMode = parameters.getRawParameterValue(Parameters::ID::monoMode);
    unisonVoices = parameters.getRawParameterValue(Parameters::ID::unisonVoices);
    unisonDetune = parameters.getRawParameterValue(Parameters::ID::unisonDetune);
    unisonBlend = parameters.getRawParameterValue(Parameters::ID::unisonBlend);
    unisonSpread = parameters.getRawParameterValue(Parameters::ID::unisonSpread);
    macroTone = parameters.getRawParameterValue(Parameters::ID::macroTone);
    macroDirt = parameters.getRawParameterValue(Parameters::ID::macroDirt);
    macroMotion = parameters.getRawParameterValue(Parameters::ID::macroMotion);
}

bool Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<Sound*>(sound) != nullptr;
}

void Voice::prepare(double sampleRate, int maximumBlockSize)
{
    for (auto& oscillator : oscillators)
        oscillator.prepare(sampleRate);

    for (auto& oscillator : oscillators2)
        oscillator.prepare(sampleRate);

    subOscillator.prepare(sampleRate);
    subOscillator.setWaveform(Waveform::sine);
    ampEnvelope.setSampleRate(sampleRate);
    leftFilter.prepare(sampleRate, maximumBlockSize);
    rightFilter.prepare(sampleRate, maximumBlockSize);
}

void Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    pitchWheelMoved(currentPitchWheelPosition);
    currentFrequencyHz = frequencyForNote(midiNoteNumber);
    for (auto& oscillator : oscillators)
        oscillator.reset();

    for (auto& oscillator : oscillators2)
        oscillator.reset();

    subOscillator.reset();
    leftFilter.reset();
    rightFilter.reset();
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
        const auto unisonSample = renderUnisonStack(osc1Gain, osc2Gain);
        const auto centeredSample = (subOscillator.process() * subGain * 0.75f)
            + (noiseSample * noiseGain * 0.45f);

        auto leftSample = (unisonSample.left + centeredSample) * sourceCompensation;
        auto rightSample = (unisonSample.right + centeredSample) * sourceCompensation;
        leftSample = leftFilter.process(leftSample);
        rightSample = rightFilter.process(rightSample);
        const auto dirt = readParameter(macroDirt, 0.0f);
        const auto macroDrive = juce::jlimit(0.0f, 0.95f, readParameter(driveAmount, 0.18f) + (dirt * 0.55f));
        leftSample = distortion.process(leftSample, macroDrive);
        rightSample = distortion.process(rightSample, macroDrive);
        leftSample *= envelopeValue * noteVelocity * 0.28f;
        rightSample *= envelopeValue * noteVelocity * 0.28f;

        if (outputBuffer.getNumChannels() == 1)
        {
            outputBuffer.addSample(0, startSample + sampleIndex, (leftSample + rightSample) * 0.5f);
        }
        else
        {
            outputBuffer.addSample(0, startSample + sampleIndex, leftSample);
            outputBuffer.addSample(1, startSample + sampleIndex, rightSample);

            for (auto channel = 2; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.addSample(channel, startSample + sampleIndex, (leftSample + rightSample) * 0.5f);
        }

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
    const auto waveform = static_cast<Waveform>(juce::jlimit(0, 3, waveIndex));

    const auto octaveOffset = static_cast<int>(readParameter(oscOctave, 0.0f)) * 12.0f;
    const auto tuneOffset = readParameter(oscTune, 0.0f);
    const auto pitchRatio = std::pow(2.0f, (octaveOffset + tuneOffset + pitchBendSemitones) / 12.0f);

    const auto osc2WaveIndex = static_cast<int>(readParameter(osc2Wave, 1.0f));
    const auto osc2Waveform = static_cast<Waveform>(juce::jlimit(0, 3, osc2WaveIndex));

    const auto osc2OctaveOffset = static_cast<int>(readParameter(osc2Octave, 0.0f)) * 12.0f;
    const auto motion = readParameter(macroMotion, 0.0f);
    const auto osc2TuneOffset = readParameter(osc2Tune, 0.0f) + (motion * 5.0f);
    const auto osc2PitchRatio = std::pow(2.0f, (osc2OctaveOffset + osc2TuneOffset + pitchBendSemitones) / 12.0f);

    const auto activeUnisonVoices = getUnisonVoiceCount();
    const auto detuneCents = readParameter(unisonDetune, 0.0f) * 24.0f;
    for (auto voiceIndex = 0; voiceIndex < activeUnisonVoices; ++voiceIndex)
    {
        const auto detuneRatio = std::pow(2.0f, (getUnisonPosition(voiceIndex, activeUnisonVoices) * detuneCents) / 1200.0f);
        oscillators[static_cast<size_t>(voiceIndex)].setWaveform(waveform);
        oscillators[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * pitchRatio * detuneRatio);
        oscillators2[static_cast<size_t>(voiceIndex)].setWaveform(osc2Waveform);
        oscillators2[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * osc2PitchRatio * detuneRatio);
    }

    subOscillator.setFrequency(currentFrequencyHz * 0.5f);

    ampEnvelope.setParameters(
        readParameter(ampAttack, 0.01f),
        readParameter(ampDecay, 0.18f),
        readParameter(ampSustain, 0.65f),
        readParameter(ampRelease, 0.22f));

    const auto tone = readParameter(macroTone, 0.0f);
    const auto envAmount = juce::jlimit(-1.0f, 1.0f, readParameter(filterEnvAmount, 0.15f) + (motion * 0.35f));
    const auto cutoffScale = std::pow(2.0f, envAmount * envelopeValue * 4.0f);
    const auto toneCutoffScale = std::pow(2.0f, tone * 2.5f);
    const auto macroResonance = juce::jlimit(0.1f, 1.4f, readParameter(filterResonance, 0.45f) + (tone * 0.22f));
    const auto filterModeIndex = static_cast<int>(readParameter(filterMode, 0.0f));
    const auto mode = static_cast<Filter::Mode>(juce::jlimit(0, 2, filterModeIndex));
    const auto cutoff = readParameter(filterCutoff, 1800.0f) * cutoffScale * toneCutoffScale;
    leftFilter.setMode(mode);
    rightFilter.setMode(mode);
    leftFilter.setCutoffAndResonance(cutoff, macroResonance);
    rightFilter.setCutoffAndResonance(cutoff, macroResonance);
}

Voice::StereoSample Voice::renderUnisonStack(float osc1Gain, float osc2Gain)
{
    StereoSample sample;
    const auto activeUnisonVoices = getUnisonVoiceCount();
    const auto blend = activeUnisonVoices > 1 ? readParameter(unisonBlend, 0.65f) : 0.0f;
    const auto spread = readParameter(monoMode, 0.0f) >= 0.5f ? 0.0f : readParameter(unisonSpread, 0.0f);
    const auto voiceWeight = activeUnisonVoices > 1 ? juce::jmap(blend, 0.0f, 1.0f, 0.2f, 1.0f) : 1.0f;
    auto weightTotal = 0.0f;

    for (auto voiceIndex = 0; voiceIndex < activeUnisonVoices; ++voiceIndex)
    {
        const auto position = getUnisonPosition(voiceIndex, activeUnisonVoices);
        const auto pan = position * spread;
        const auto leftPan = pan <= 0.0f ? 1.0f : 1.0f - pan;
        const auto rightPan = pan >= 0.0f ? 1.0f : 1.0f + pan;
        const auto weight = activeUnisonVoices > 1 ? voiceWeight * (1.0f + (std::abs(position) * blend * 0.15f)) : 1.0f;
        const auto voiceSample = (oscillators[static_cast<size_t>(voiceIndex)].process() * osc1Gain)
            + (oscillators2[static_cast<size_t>(voiceIndex)].process() * osc2Gain);

        sample.left += voiceSample * weight * leftPan;
        sample.right += voiceSample * weight * rightPan;
        weightTotal += weight;
    }

    if (weightTotal > 1.0f)
    {
        const auto compensation = 1.0f / weightTotal;
        sample.left *= compensation;
        sample.right *= compensation;
    }

    return sample;
}

int Voice::getUnisonVoiceCount() const
{
    return juce::jlimit(1, maxUnisonVoices, static_cast<int>(std::round(readParameter(unisonVoices, 1.0f))));
}

float Voice::getUnisonPosition(int voiceIndex, int voiceCount) const
{
    if (voiceCount <= 1)
        return 0.0f;

    return juce::jmap(static_cast<float>(voiceIndex), 0.0f, static_cast<float>(voiceCount - 1), -1.0f, 1.0f);
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
