#pragma once

#include "../Parameters.h"
#include "Distortion.h"
#include "Envelope.h"
#include "Filter.h"
#include "Oscillator.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace Synth
{
class Sound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Voice final : public juce::SynthesiserVoice
{
public:
    explicit Voice(Parameters::APVTS& parameters);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepare(double sampleRate, int maximumBlockSize);

private:
    Parameters::APVTS& parameters;
    Oscillator oscillator;
    Envelope ampEnvelope;
    Filter filter;
    Distortion distortion;

    float noteVelocity = 0.0f;
    float currentFrequencyHz = 440.0f;
    float pitchBendSemitones = 0.0f;

    std::atomic<float>* oscWave = nullptr;
    std::atomic<float>* oscOctave = nullptr;
    std::atomic<float>* oscTune = nullptr;
    std::atomic<float>* ampAttack = nullptr;
    std::atomic<float>* ampDecay = nullptr;
    std::atomic<float>* ampSustain = nullptr;
    std::atomic<float>* ampRelease = nullptr;
    std::atomic<float>* filterCutoff = nullptr;
    std::atomic<float>* filterResonance = nullptr;
    std::atomic<float>* filterEnvAmount = nullptr;
    std::atomic<float>* driveAmount = nullptr;

    void updateVoiceParameters(float envelopeValue);
    float frequencyForNote(int midiNoteNumber) const;
};
}

