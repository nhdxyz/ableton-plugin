#pragma once

#include "Effects/EffectsRack.h"
#include "Parameters.h"
#include "Randomization/Randomizer.h"
#include "Sampler/SamplePlayer.h"
#include "Sequencer/PatternSequencer.h"
#include "Synth/SynthEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <initializer_list>
#include <optional>
#include <random>
#include <vector>

class NateVSTAudioProcessor final : public juce::AudioProcessor
{
public:
    NateVSTAudioProcessor();
    ~NateVSTAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    struct PresetInfo
    {
        juce::String name;
        juce::String category;
        juce::String source;
        juce::String tags;
        juce::String folder;
        juce::String author;
        juce::String pack;
        juce::String key;
        juce::String macroSummary;
        int bpm = 0;
        int rating = 0;
        float macroIntensity = 0.0f;
        juce::int64 lastModifiedMs = 0;
        bool isFactory = false;
        bool isFavorite = false;
    };

    struct PresetSaveOptions
    {
        juce::String category;
        juce::String author;
        juce::String pack;
        juce::String key;
        int bpm = 0;
    };

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    Parameters::APVTS& getValueTreeState();
    void generateRandomPatch();
    void generateRandomPatch(int mutationScopeIndex);
    void mutateRandomPatch();
    void mutateRandomPatch(int mutationScopeIndex);
    void wildMutateRandomPatch();
    void wildMutateRandomPatch(int mutationScopeIndex);
    void createRandomVariation();
    void createRandomVariation(int mutationScopeIndex);
    bool undoRandomization();
    bool redoRandomization();
    bool loadSampleFile(const juce::File& file);
    void clearSample();
    bool randomizeSampleCut();
    bool randomizeUkgVocalChop();
    bool triggerSampleAudition();
    juce::String getLoadedSampleName() const;
    Sampler::SamplePeakOverview createSamplePeakOverview(int pointCount) const;
    Sequencer::Step getSequencerStep(int index) const;
    void setSequencerStep(int index, Sequencer::Step step);
    bool randomizeSequencerPattern();
    bool mutateSequencerPattern();
    bool undoSequencerEdit();
    void applySequencerPatternPreset(int presetIndex);
    bool applySequencerGrooveTransform(int transformIndex);
    void copySequencerFirstHalfToSecondHalf();
    void rotateSequencerPattern(int stepOffset);
    bool exportSequencerMidiFile(const juce::File& destination) const;
    void clearSequencerPattern();
    juce::String getActiveRandomizationLockSummary() const;
    juce::String getRandomHistorySummary() const;
    bool savePreset(const juce::String& presetName);
    bool savePreset(const juce::String& presetName, const juce::String& category);
    bool savePreset(const juce::String& presetName, const PresetSaveOptions& options);
    bool loadPreset(const juce::String& presetName);
    juce::StringArray getPresetNames() const;
    std::vector<PresetInfo> getPresetLibrary() const;
    juce::File getPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const;
    bool isPresetFavorite(const juce::String& presetName) const;
    bool setPresetFavorite(const juce::String& presetName, bool shouldBeFavorite);
    int getPresetRating(const juce::String& presetName) const;
    bool setPresetRating(const juce::String& presetName, int rating);
    juce::StringArray getRecentPresetNames() const;
    void notePresetLoaded(const juce::String& presetName);
    void capturePerformanceSnapshot(int slotIndex);
    bool recallPerformanceSnapshot(int slotIndex);
    bool hasPerformanceSnapshot(int slotIndex) const;
    juce::MidiKeyboardState& getMidiKeyboardState() noexcept;
    void getOutputMeterLevels(float& peakLeft, float& peakRight, float& rmsLeft, float& rmsRight) const noexcept;
    void getLowEndMeterLevels(float& subRms, float& lowStereoRisk, float& outputPeak) const noexcept;
    struct HostSyncStatus
    {
        double bpm = 124.0;
        bool positionAvailable = false;
        bool playing = false;
        bool ppqAvailable = false;
        double ppqPosition = 0.0;
    };
    HostSyncStatus getHostSyncStatus() const noexcept;

private:
    enum class RandomAction
    {
        generate,
        mutate,
        wild,
        variation
    };

    enum class RandomMutationScope
    {
        all = 0,
        source,
        envelope,
        filter,
        sample,
        effects,
        sequencer,
        macros
    };

    Parameters::APVTS parameters;
    Synth::SynthEngine synthEngine;
    Randomization::Randomizer randomizer;
    Sampler::SamplePlayer samplePlayer;
    Sequencer::PatternSequencer patternSequencer;
    Effects::EffectsRack effectsRack;
    juce::MidiKeyboardState midiKeyboardState;

    using ChordMemoryNoteArray = Sequencer::PatternSequencer::ChordNoteArray;
    std::array<std::array<ChordMemoryNoteArray, 128>, 16> chordMemoryActiveNotes {};
    std::array<std::array<int, 128>, 16> chordMemoryActiveNoteCounts {};

    std::atomic<float>* outputGain = nullptr;
    std::atomic<float>* sequencerChordMemory = nullptr;
    std::atomic<float> outputMeterPeakLeft { 0.0f };
    std::atomic<float> outputMeterPeakRight { 0.0f };
    std::atomic<float> outputMeterRmsLeft { 0.0f };
    std::atomic<float> outputMeterRmsRight { 0.0f };
    std::atomic<float> lowEndSubRms { 0.0f };
    std::atomic<float> lowEndStereoRisk { 0.0f };
    std::atomic<float> lowEndOutputPeak { 0.0f };
    std::atomic<float> hostSyncBpm { 124.0f };
    std::atomic<float> hostSyncPpqPosition { 0.0f };
    std::atomic<bool> hostSyncPositionAvailable { false };
    std::atomic<bool> hostSyncPlaying { false };
    std::atomic<bool> hostSyncPpqAvailable { false };
    float lowEndStateLeft = 0.0f;
    float lowEndStateRight = 0.0f;
    double meterSampleRate = 44100.0;
    juce::String loadedSamplePath;
    std::mt19937 sampleRandomEngine;
    juce::ValueTree randomUndoState;
    juce::ValueTree randomRedoState;
    juce::ValueTree sequencerUndoState;
    std::array<juce::ValueTree, 2> performanceSnapshots;
    juce::String randomUndoLabel;
    juce::String randomRedoLabel;
    bool hasRandomUndoState = false;
    bool hasRandomRedoState = false;
    bool hasSequencerUndoState = false;

    void runRandomAction(RandomAction action, int mutationScopeIndex = 0);
    static juce::String randomActionLabel(RandomAction action);
    static RandomMutationScope randomMutationScopeFromIndex(int mutationScopeIndex);
    bool isRandomLockEnabled(const juce::String& parameterID) const;
    float getParameterPlainValue(const juce::String& parameterID, float fallback) const;
    float getParameterPlainValueFromState(const juce::ValueTree& state, const juce::String& parameterID, float fallback) const;
    void restoreParameterFromState(const juce::ValueTree& state, const juce::String& parameterID);
    void restoreParameterGroupFromState(const juce::ValueTree& state, std::initializer_list<const char*> parameterIDs);
    void restoreSectionsOutsideMutationScope(const juce::ValueTree& state, RandomMutationScope mutationScope);
    void restoreMutationScopeFromState(const juce::ValueTree& state, RandomMutationScope mutationScope);
    void restoreModulationFromState(const juce::ValueTree& state);
    void restoreOutputFromState(const juce::ValueTree& state);
    void restoreLockedSectionsFromState(const juce::ValueTree& state);
    void restoreSampleFromState(const juce::ValueTree& state);
    void restoreSequencerFromState(const juce::ValueTree& state);
    void captureSequencerUndoState();
    void setParameterPlainValue(const juce::String& parameterID, float plainValue);
    void applyChordMemoryToMidi(juce::MidiBuffer& midiMessages);
    void clearChordMemoryActiveNotes();
    double getHostBpm() const;
    Sequencer::HostPosition getHostPosition() const;
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer) noexcept;
    juce::ValueTree createPluginState();
    juce::ValueTree createPluginState(bool includePerformanceSnapshots);
    void restorePluginState(const juce::ValueTree& state);
    void restorePluginState(const juce::ValueTree& state, bool restorePerformanceSnapshots);
    void restorePerformanceSnapshotsFromState(const juce::ValueTree& state);
    void appendPerformanceSnapshotsToState(juce::ValueTree& state) const;
    void removePerformanceSnapshotChildren(juce::ValueTree& state) const;
    juce::ValueTree loadLibraryState() const;
    bool saveLibraryState(const juce::ValueTree& state) const;
    juce::StringArray getLibraryStateNames(const juce::Identifier& childType) const;
    juce::File presetFileForName(const juce::String& presetName) const;
    juce::File presetFileForName(const juce::String& presetName, const juce::String& category) const;
    juce::File factoryPresetFileForName(const juce::String& presetName) const;
    juce::File findPresetFileInDirectory(const juce::File& directory, const juce::String& presetName) const;
    juce::File presetDirectoryForCategory(const juce::String& category) const;
    juce::File libraryStateFile() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessor)
};
