#pragma once

#include "../Parameters.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <optional>

namespace Sequencer
{
struct Step
{
    bool enabled = false;
    int noteOffset = 0;
    float velocity = 0.8f;
    float probability = 1.0f;
    float timing = 0.0f;
    float length = 1.0f;
    float lock = 0.0f;
};

struct HostPosition
{
    bool isAvailable = false;
    bool isPlaying = false;
    std::optional<double> ppqPosition;
};

class PatternSequencer
{
public:
    static constexpr int numSteps = 16;
    static constexpr int maxChordNotes = 5;
    static constexpr int minNoteOffset = -12;
    static constexpr int maxNoteOffset = 12;
    using ChordNoteArray = std::array<int, maxChordNotes>;

    explicit PatternSequencer(Parameters::APVTS& parameters);

    void prepare(double sampleRate);
    void reset();
    bool isEnabled() const;
    Step getStep(int index) const;
    void setStep(int index, Step step);
    int getQuantizedNoteOffset(int noteOffset) const;
    ChordNoteArray getChordNotes(int rootNote, int octaveOffset, int noteOffset, int& noteCount) const;
    float getChordNoteVelocity(float velocity, int noteIndex) const;
    int getChordStrumOffset(int stepLength, int noteIndex, int noteCount) const;
    void clear();
    void randomize(float amount);
    float getActiveStepLock() const noexcept;
    int getActiveStepIndex() const noexcept;

    void process(juce::MidiBuffer& midi, int numSamples, double bpm, HostPosition hostPosition);

private:
    Parameters::APVTS& parameters;
    std::array<std::atomic<int>, numSteps> stepEnabled {};
    std::array<std::atomic<int>, numSteps> stepNoteOffset {};
    std::array<std::atomic<float>, numSteps> stepVelocity {};
    std::array<std::atomic<float>, numSteps> stepProbability {};
    std::array<std::atomic<float>, numSteps> stepTiming {};
    std::array<std::atomic<float>, numSteps> stepLengths {};
    std::array<std::atomic<float>, numSteps> stepLocks {};
    std::atomic<float> activeStepLock { 0.0f };
    std::atomic<int> activeStepIndex { -1 };
    double currentSampleRate = 44100.0;
    double samplesUntilNextStep = 0.0;
    double pendingNoteOffSamples = -1.0;
    int currentStep = 0;
    ChordNoteArray activeNotes {};
    int activeNoteCount = 0;
    std::optional<double> lastHostPpqPosition;
    bool wasHostPlaying = false;
    juce::uint32 randomState = 0x1234abcd;

    std::atomic<float>* sequencerEnabled = nullptr;
    std::atomic<float>* sequencerRate = nullptr;
    std::atomic<float>* sequencerRoot = nullptr;
    std::atomic<float>* sequencerGate = nullptr;
    std::atomic<float>* sequencerSwing = nullptr;
    std::atomic<float>* sequencerGrooveMode = nullptr;
    std::atomic<float>* sequencerScale = nullptr;
    std::atomic<float>* sequencerChordMode = nullptr;
    std::atomic<float>* sequencerChordVoicing = nullptr;
    std::atomic<float>* sequencerChordStrum = nullptr;
    std::atomic<float>* sequencerAccent = nullptr;
    std::atomic<float>* sequencerOctave = nullptr;
    std::atomic<float>* sequencerProbability = nullptr;

    int getStepLengthSamples(double bpm) const;
    double getStepLengthPpq() const;
    int getStepDurationSamples(int baseStepLengthSamples, int stepIndex) const;
    int getStepDelaySamples(int baseStepLengthSamples, int stepIndex) const;
    int quantizeNoteOffset(int noteOffset) const;
    int getChordIntervalCount(std::array<int, maxChordNotes>& intervals) const;
    void applyChordVoicing(std::array<int, maxChordNotes>& intervals, int& intervalCount) const;
    bool isOffsetInScale(int noteOffset, int scaleMode) const;
    void addNoteOffsForActiveNotes(juce::MidiBuffer& midi, int samplePosition);
    float nextRandomFloat();
    bool shouldTriggerStep(const Step& step);
    void alignToHostPosition(juce::MidiBuffer& midi, HostPosition hostPosition, int baseStepLengthSamples, int numSamples, double bpm);
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
