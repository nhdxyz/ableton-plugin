#pragma once

#include "../Parameters.h"
#include "Distortion.h"
#include "Envelope.h"
#include "Filter.h"
#include "Oscillator.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

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
    void setHostBpm(double bpm) noexcept;

private:
    static constexpr int maxUnisonVoices = 7;
    static constexpr int maxModSlots = 8;

    struct StereoSample
    {
        float left = 0.0f;
        float right = 0.0f;
    };

    Parameters::APVTS& parameters;
    std::array<Oscillator, maxUnisonVoices> oscillators;
    std::array<Oscillator, maxUnisonVoices> oscillators2;
    Oscillator subOscillator;
    Envelope ampEnvelope;
    Envelope modEnvelope;
    Filter leftFilter;
    Filter rightFilter;
    Distortion distortion;
    juce::Random noiseRandom;
    juce::Random modulationRandom;

    float noteVelocity = 0.0f;
    float currentFrequencyHz = 440.0f;
    float targetFrequencyHz = 440.0f;
    float glideStartFrequencyHz = 440.0f;
    float pitchBendSemitones = 0.0f;
    float lfoPhase = 0.0f;
    float lfoStepValue = 0.0f;
    float currentOsc2LevelOffset = 0.0f;
    float currentDriveOffset = 0.0f;
    double currentSampleRate = 44100.0;
    double hostBpm = 124.0;
    int glideSamplesRemaining = 0;
    int glideTotalSamples = 0;
    bool hasPreviousNoteFrequency = false;

    std::atomic<float>* oscWave = nullptr;
    std::atomic<float>* oscOctave = nullptr;
    std::atomic<float>* oscTune = nullptr;
    std::atomic<float>* osc1Level = nullptr;
    std::atomic<float>* osc2Wave = nullptr;
    std::atomic<float>* osc2Octave = nullptr;
    std::atomic<float>* osc2Tune = nullptr;
    std::atomic<float>* osc2Level = nullptr;
    std::atomic<float>* subLevel = nullptr;
    std::atomic<float>* noiseLevel = nullptr;
    std::atomic<float>* ampAttack = nullptr;
    std::atomic<float>* ampDecay = nullptr;
    std::atomic<float>* ampSustain = nullptr;
    std::atomic<float>* ampRelease = nullptr;
    std::atomic<float>* filterCutoff = nullptr;
    std::atomic<float>* filterResonance = nullptr;
    std::atomic<float>* filterEnvAmount = nullptr;
    std::atomic<float>* filterMode = nullptr;
    std::atomic<float>* filterCharacter = nullptr;
    std::atomic<float>* filterSlope = nullptr;
    std::atomic<float>* driveAmount = nullptr;
    std::atomic<float>* monoMode = nullptr;
    std::atomic<float>* glideTime = nullptr;
    std::atomic<float>* unisonVoices = nullptr;
    std::atomic<float>* unisonDetune = nullptr;
    std::atomic<float>* unisonBlend = nullptr;
    std::atomic<float>* unisonSpread = nullptr;
    std::atomic<float>* macroTone = nullptr;
    std::atomic<float>* macroDirt = nullptr;
    std::atomic<float>* macroMotion = nullptr;
    std::atomic<float>* macroSpace = nullptr;
    std::atomic<float>* macroWeight = nullptr;
    std::atomic<float>* macroBounce = nullptr;
    std::atomic<float>* macroWarp = nullptr;
    std::atomic<float>* macroThrow = nullptr;
    std::atomic<float>* lfo1Rate = nullptr;
    std::atomic<float>* lfo1Sync = nullptr;
    std::atomic<float>* lfo1SyncRate = nullptr;
    std::atomic<float>* lfo1Shape = nullptr;
    std::atomic<float>* lfo1Depth = nullptr;
    std::atomic<float>* lfo1Phase = nullptr;
    std::atomic<float>* lfo1Retrigger = nullptr;
    std::array<std::atomic<float>*, 8> lfo1CurvePoints {};
    std::atomic<float>* modEnv1Attack = nullptr;
    std::atomic<float>* modEnv1Decay = nullptr;
    std::atomic<float>* modEnv1Sustain = nullptr;
    std::atomic<float>* modEnv1Release = nullptr;
    std::atomic<float>* modEnv1Depth = nullptr;
    std::array<std::atomic<float>*, maxModSlots> modMatrixSources {};
    std::array<std::atomic<float>*, maxModSlots> modMatrixDestinations {};
    std::array<std::atomic<float>*, maxModSlots> modMatrixAmounts {};

    void updateVoiceParameters(float envelopeValue);
    void updateGlide();
    float processLfo();
    float evaluateLfoCurve(float phase) const;
    float evaluateModulationSource(int sourceIndex, float lfoValue, float modEnvelopeValue) const;
    StereoSample renderUnisonStack(float osc1Gain, float osc2Gain);
    int getUnisonVoiceCount() const;
    float getUnisonPosition(int voiceIndex, int voiceCount) const;
    float frequencyForNote(int midiNoteNumber) const;
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
