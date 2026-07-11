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
    int ratchet = 1;
    int condition = 0;
    bool slide = false;
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
    static constexpr int minRatchet = 1;
    static constexpr int maxRatchet = 4;
    static constexpr int minCondition = 0;
    static constexpr int maxCondition = 3;
    static constexpr int maxSceneChainLength = 4;
    using ChordNoteArray = std::array<int, maxChordNotes>;
    using Pattern = std::array<Step, numSteps>;
    using SceneChain = std::array<Pattern, maxSceneChainLength>;
    struct SceneControls
    {
        float root = 36.0f;
        float gate = 0.55f;
        float swing = 0.0f;
        float grooveMode = 0.0f;
        float scale = 0.0f;
        float chordMode = 0.0f;
        float chordVoicing = 0.0f;
        float chordStrum = 0.0f;
        float accent = 0.35f;
        float octave = 0.0f;
        float probability = 1.0f;
    };
    using SceneControlChain = std::array<SceneControls, maxSceneChainLength>;

    explicit PatternSequencer(Parameters::APVTS& parameters);

    void prepare(double sampleRate);
    void reset();
    void suspend(juce::MidiBuffer& midi, int samplePosition = 0);
    bool isEnabled() const;
    Step getStep(int index) const;
    void setStep(int index, Step step);
    int getQuantizedNoteOffset(int noteOffset) const;
    ChordNoteArray getChordNotes(int rootNote, int octaveOffset, int noteOffset, int& noteCount) const;
    float getChordNoteVelocity(float velocity, int noteIndex) const;
    int getChordStrumOffset(int stepLength, int noteIndex, int noteCount) const;
    void clear();
    void randomize(float amount);
    void setSceneChain(const SceneChain& scenes, int sceneCount);
    void setSceneChain(const SceneChain& scenes, const SceneControlChain& controls, int sceneCount);
    void clearSceneChain();
    bool isSceneChainEnabled() const noexcept;
    int getActiveSceneChainIndex() const noexcept;
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
    std::array<std::atomic<int>, numSteps> stepRatchets {};
    std::array<std::atomic<int>, numSteps> stepConditions {};
    std::array<std::atomic<int>, numSteps> stepSlides {};
    struct StepStorage
    {
        std::array<std::atomic<int>, numSteps> enabled {};
        std::array<std::atomic<int>, numSteps> noteOffset {};
        std::array<std::atomic<float>, numSteps> velocity {};
        std::array<std::atomic<float>, numSteps> probability {};
        std::array<std::atomic<float>, numSteps> timing {};
        std::array<std::atomic<float>, numSteps> length {};
        std::array<std::atomic<float>, numSteps> lock {};
        std::array<std::atomic<int>, numSteps> ratchet {};
        std::array<std::atomic<int>, numSteps> condition {};
        std::array<std::atomic<int>, numSteps> slide {};
    };
    struct SceneControlStorage
    {
        std::atomic<float> root { 36.0f };
        std::atomic<float> gate { 0.55f };
        std::atomic<float> swing { 0.0f };
        std::atomic<float> grooveMode { 0.0f };
        std::atomic<float> scale { 0.0f };
        std::atomic<float> chordMode { 0.0f };
        std::atomic<float> chordVoicing { 0.0f };
        std::atomic<float> chordStrum { 0.0f };
        std::atomic<float> accent { 0.35f };
        std::atomic<float> octave { 0.0f };
        std::atomic<float> probability { 1.0f };
    };
    std::array<StepStorage, maxSceneChainLength> sceneChainSteps {};
    std::array<SceneControlStorage, maxSceneChainLength> sceneChainControls {};
    std::atomic<int> sceneChainLength { 0 };
    std::atomic<float> activeStepLock { 0.0f };
    std::atomic<int> activeStepIndex { -1 };
    mutable std::atomic<int> activeSceneChainIndex { -1 };
    double currentSampleRate = 44100.0;
    double samplesUntilNextStep = 0.0;
    double pendingNoteOffSamples = -1.0;
    int currentStep = 0;
    int currentCycle = 0;
    ChordNoteArray activeNotes {};
    int activeNoteCount = 0;
    bool activeNotesSlideToNext = false;
    struct PendingChordEvent
    {
        double samplesUntil = -1.0;
        ChordNoteArray notes {};
        int noteCount = 0;
        float velocity = 0.0f;
        int strumStepLength = 1;
        bool noteOn = false;
    };
    std::array<PendingChordEvent, maxRatchet * 2> pendingChordEvents {};
    int pendingChordEventCount = 0;
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

    Step normaliseStep(Step step) const;
    Step readStoredStep(const StepStorage& storage, int index) const;
    void writeStoredStep(StepStorage& storage, int index, Step step);
    SceneControls readCurrentSceneControls() const;
    SceneControls readStoredSceneControls(const SceneControlStorage& storage) const;
    void writeStoredSceneControls(SceneControlStorage& storage, SceneControls controls);
    int getPlaybackSceneIndex() const;
    Step readPlaybackStep(int index, int sceneIndex) const;
    Step getPlaybackStep(int index) const;
    int getStepLengthSamples(double bpm) const;
    double getStepLengthPpq() const;
    int getStepDurationSamples(int baseStepLengthSamples, int stepIndex, const SceneControls& controls) const;
    int getStepDelaySamples(int baseStepLengthSamples, int stepIndex, const SceneControls& controls) const;
    int quantizeNoteOffset(int noteOffset) const;
    int quantizeNoteOffset(int noteOffset, const SceneControls& controls) const;
    ChordNoteArray getChordNotes(int rootNote, int octaveOffset, int noteOffset, int& noteCount, const SceneControls& controls) const;
    int getChordIntervalCount(std::array<int, maxChordNotes>& intervals, const SceneControls& controls) const;
    void applyChordVoicing(std::array<int, maxChordNotes>& intervals, int& intervalCount, const SceneControls& controls) const;
    bool isOffsetInScale(int noteOffset, int scaleMode) const;
    int getChordStrumOffset(int stepLength, int noteIndex, int noteCount, const SceneControls& controls) const;
    void addNoteOffsForNotes(juce::MidiBuffer& midi,
                             const ChordNoteArray& notes,
                             int noteCount,
                             int samplePosition) const;
    void addNoteOffsForActiveNotes(juce::MidiBuffer& midi, int samplePosition);
    void clearPendingChordEvents();
    void emitChordEvent(juce::MidiBuffer& midi,
                        const PendingChordEvent& event,
                        int samplePosition);
    void queueChordEvent(double samplesUntil,
                         bool noteOn,
                         ChordNoteArray notes,
                         int noteCount,
                         float velocity,
                         int strumStepLength);
    void emitOrQueueChordEvent(juce::MidiBuffer& midi,
                               int numSamples,
                               double samplePosition,
                               bool noteOn,
                               ChordNoteArray notes,
                               int noteCount,
                               float velocity,
                               int strumStepLength);
    void processPendingChordEvents(juce::MidiBuffer& midi, int numSamples);
    float nextRandomFloat();
    bool shouldTriggerStep(const Step& step, const SceneControls& controls);
    bool stepConditionAllows(int condition) const;
    void alignToHostPosition(juce::MidiBuffer& midi, HostPosition hostPosition, int baseStepLengthSamples, int numSamples, double bpm);
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
