#pragma once

#include "../Parameters.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>

namespace Sequencer
{
struct Step
{
    bool enabled = false;
    int noteOffset = 0;
    float velocity = 0.8f;
    float probability = 1.0f;
    float timing = 0.0f;
};

class PatternSequencer
{
public:
    static constexpr int numSteps = 16;
    static constexpr int minNoteOffset = -12;
    static constexpr int maxNoteOffset = 12;

    explicit PatternSequencer(Parameters::APVTS& parameters);

    void prepare(double sampleRate);
    void reset();
    bool isEnabled() const;
    Step getStep(int index) const;
    void setStep(int index, Step step);
    int getQuantizedNoteOffset(int noteOffset) const;
    void clear();
    void randomize(float amount);

    void process(juce::MidiBuffer& midi, int numSamples, double bpm);

private:
    Parameters::APVTS& parameters;
    std::array<std::atomic<int>, numSteps> stepEnabled {};
    std::array<std::atomic<int>, numSteps> stepNoteOffset {};
    std::array<std::atomic<float>, numSteps> stepVelocity {};
    std::array<std::atomic<float>, numSteps> stepProbability {};
    std::array<std::atomic<float>, numSteps> stepTiming {};
    double currentSampleRate = 44100.0;
    double samplesUntilNextStep = 0.0;
    double pendingNoteOffSamples = -1.0;
    int currentStep = 0;
    int activeNote = -1;
    juce::uint32 randomState = 0x1234abcd;

    std::atomic<float>* sequencerEnabled = nullptr;
    std::atomic<float>* sequencerRate = nullptr;
    std::atomic<float>* sequencerRoot = nullptr;
    std::atomic<float>* sequencerGate = nullptr;
    std::atomic<float>* sequencerSwing = nullptr;
    std::atomic<float>* sequencerGrooveMode = nullptr;
    std::atomic<float>* sequencerScale = nullptr;
    std::atomic<float>* sequencerAccent = nullptr;
    std::atomic<float>* sequencerOctave = nullptr;
    std::atomic<float>* sequencerProbability = nullptr;

    int getStepLengthSamples(double bpm) const;
    int getStepDurationSamples(int baseStepLengthSamples, int stepIndex) const;
    int getStepDelaySamples(int baseStepLengthSamples, int stepIndex) const;
    int quantizeNoteOffset(int noteOffset) const;
    bool isOffsetInScale(int noteOffset, int scaleMode) const;
    float nextRandomFloat();
    bool shouldTriggerStep(const Step& step);
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
