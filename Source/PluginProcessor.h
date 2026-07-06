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
    static constexpr size_t outputSpectrumSnapshotSize = 1024;

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
        juce::String notes;
        std::array<float, 8> macroValues {};
        int bpm = 0;
        int rating = 0;
        float macroIntensity = 0.0f;
        juce::int64 lastModifiedMs = 0;
        bool isFactory = false;
        bool isFavorite = false;
        bool previewAvailable = false;
        bool previewStale = false;
        float previewPeak = 0.0f;
        float previewRms = 0.0f;
        double previewDurationSeconds = 0.0;
        juce::int64 previewLastModifiedMs = 0;
    };

    struct PresetSaveOptions
    {
        juce::String category;
        juce::String author;
        juce::String pack;
        juce::String key;
        int bpm = 0;
        bool generated = false;
        juce::String generatedRecipe;
        juce::String notes;
    };

    struct PresetPreviewInfo
    {
        juce::File file;
        bool available = false;
        bool stale = false;
        float peak = 0.0f;
        float rms = 0.0f;
        double durationSeconds = 0.0;
        juce::int64 lastModifiedMs = 0;
        juce::String status;
    };

    struct PresetPreviewBatchResult
    {
        int requested = 0;
        int rendered = 0;
        int alreadyReady = 0;
        int stale = 0;
        int failed = 0;
        juce::StringArray renderedPresetNames;
        juce::StringArray readyPresetNames;
        juce::StringArray stalePresetNames;
        juce::StringArray failedPresetNames;
        juce::String status;
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
    bool hasRandomCandidate(int slotIndex) const;
    juce::String getRandomCandidateSummary(int slotIndex) const;
    juce::String getRandomCandidateCompareSummary(int slotIndex);
    juce::String getRandomCandidateChangedSectionsSummary(int slotIndex);
    int getRandomCandidateChangedSectionCount(int slotIndex);
    juce::String getRandomCandidateDiffSummary(int slotIndex);
    juce::String getRandomCandidateValidationSummary(int slotIndex) const;
    juce::String getLastRandomValidationSummary() const;
    int getActiveRandomCandidateIndex() const noexcept;
    bool recallRandomCandidate(int slotIndex);
    bool beginRandomCandidateAudition(int slotIndex);
    bool endRandomCandidateAudition();
    bool promoteRandomCandidateToPerformanceSnapshot(int candidateSlotIndex, int snapshotSlotIndex);
    bool loadSampleFile(const juce::File& file);
    void clearSample();
    bool hasLoadedSample() const;
    bool hasMissingSampleReference() const;
    void beginSampleCapture();
    void stopSampleCapture();
    bool isSampleCaptureEnabled() const noexcept;
    bool isSampleCaptureWaitingForThreshold() const noexcept;
    float getSampleCaptureDurationSeconds() const noexcept;
    float getSampleCaptureCapacitySeconds() const noexcept;
    int getSampleCaptureSourceIndex() const noexcept;
    int getSampleCaptureStartModeIndex() const noexcept;
    int getSampleCaptureLengthModeIndex() const noexcept;
    int getSampleCapturePreRollModeIndex() const noexcept;
    float getSampleCaptureThresholdDb() const noexcept;
    float getSampleCaptureSourcePeak() const noexcept;
    float getSampleCaptureTargetDurationSeconds() const noexcept;
    float getSampleCapturePreRollDurationSeconds() const noexcept;
    juce::String getSampleCaptureSourceName() const;
    juce::String getSampleCaptureStartModeName() const;
    juce::String getSampleCaptureLengthModeName() const;
    juce::String getSampleCapturePreRollModeName() const;
    bool commitSampleCaptureToSampler();
    int getSampleCaptureTakeCount() const;
    juce::StringArray getSampleCaptureTakeNames(int limit) const;
    int getSelectedSampleCaptureTakeIndex() const;
    bool selectSampleCaptureTake(int takeIndex);
    juce::String getSelectedSampleCaptureTakePath() const;
    juce::String getLatestSampleCaptureTakePath() const;
    bool autoTrimSampleToContent();
    bool spliceSampleToSlices();
    bool randomizeRecordedSample();
    bool randomizeSampleCut();
    int detectSampleTransientSlices();
    bool randomizeUkgVocalChop();
    bool triggerSampleAudition();
    bool triggerSampleSliceAudition(int sliceIndex);
    juce::String getLoadedSampleName() const;
    juce::String getLoadedSamplePath() const;
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
    void captureSequencerPatternScene(int slotIndex);
    bool recallSequencerPatternScene(int slotIndex);
    bool hasSequencerPatternScene(int slotIndex) const;
    juce::String getSequencerPatternSceneSummary(int slotIndex) const;
    void setSequencerSceneChainPlaybackEnabled(bool shouldEnable);
    bool isSequencerSceneChainPlaybackEnabled() const noexcept;
    void setSequencerSceneChainClipBars(int barCount);
    int getSequencerSceneChainClipBars() const noexcept;
    int getSequencerSceneChainPlaybackLength() const;
    bool exportSequencerMidiFile(const juce::File& destination) const;
    bool exportSequencerSceneChainMidiFile(const juce::File& destination) const;
    void clearSequencerPattern();
    juce::String getActiveRandomizationLockSummary() const;
    juce::String getRandomHistorySummary() const;
    bool savePreset(const juce::String& presetName);
    bool savePreset(const juce::String& presetName, const juce::String& category);
    bool savePreset(const juce::String& presetName, const PresetSaveOptions& options);
    bool saveRandomCandidatePreset(int slotIndex, const juce::String& presetName, const PresetSaveOptions& options);
    bool userPresetExists(const juce::String& presetName, const juce::String& category) const;
    bool loadPreset(const juce::String& presetName);
    juce::StringArray getPresetNames() const;
    std::vector<PresetInfo> getPresetLibrary() const;
    void invalidatePresetLibraryCache() const;
    juce::File getPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const;
    juce::File getPresetPreviewDirectory() const;
    PresetPreviewInfo getPresetPreviewInfo(const juce::String& presetName) const;
    PresetPreviewInfo ensurePresetPreview(const juce::String& presetName);
    PresetPreviewBatchResult ensurePresetPreviews(const juce::StringArray& presetNames, bool regenerateExisting = false);
    PresetPreviewBatchResult ensurePresetPreviews(int maxRenderCount, bool regenerateExisting = false);
    bool startPresetPreviewPlayback(const juce::String& presetName);
    void stopPresetPreviewPlayback();
    bool isPresetPreviewPlaying() const noexcept;
    bool isPresetFavorite(const juce::String& presetName) const;
    bool setPresetFavorite(const juce::String& presetName, bool shouldBeFavorite);
    int getPresetRating(const juce::String& presetName) const;
    bool setPresetRating(const juce::String& presetName, int rating);
    juce::StringArray getRecentPresetNames() const;
    void notePresetLoaded(const juce::String& presetName);
    void capturePerformanceSnapshot(int slotIndex);
    bool recallPerformanceSnapshot(int slotIndex);
    bool morphPerformanceSnapshots(int leftSlotIndex, int rightSlotIndex, float amount);
    bool hasPerformanceSnapshot(int slotIndex) const;
    void captureGlobalEditState(const juce::String& label);
    bool undoGlobalEdit();
    bool redoGlobalEdit();
    bool canUndoGlobalEdit() const;
    bool canRedoGlobalEdit() const;
    juce::String getGlobalEditHistorySummary() const;
    juce::MidiKeyboardState& getMidiKeyboardState() noexcept;
    void panicAllNotesOff();
    struct PerformanceModulationStatus
    {
        float modWheel = 0.0f;
        float aftertouch = 0.0f;
        float pitchBend = 0.0f;
        float note = 0.0f;
    };
    PerformanceModulationStatus getPerformanceModulationStatus() const noexcept;
    void getOutputMeterLevels(float& peakLeft, float& peakRight, float& rmsLeft, float& rmsRight) const noexcept;
    void getOutputSpectrumSnapshot(std::array<float, outputSpectrumSnapshotSize>& destination) const noexcept;
    void getStereoFieldLevels(float& correlation, float& width, float& balance, float& lowStereoRisk) const noexcept;
    void getLowEndMeterLevels(float& subRms, float& lowStereoRisk, float& outputPeak) const noexcept;
    void getPumpMeterLevels(float& phase, float& gain, float& reduction, bool& active) const noexcept;
    void getGuardMeterLevels(float& drive, float& reduction, bool& active) const noexcept;
    void requestFxSendTailKill() noexcept;
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

    struct RandomCandidateSnapshot
    {
        juce::ValueTree state;
        juce::String label;
        juce::String validationSummary;
        bool valid = false;
    };

    struct RandomRenderMetrics
    {
        float peak = 0.0f;
        float rms = 0.0f;
        float tailRms = 0.0f;
        bool finite = true;
    };

    struct RandomValidationResult
    {
        juce::String summary;
        bool shouldRetry = false;
    };

    struct PresetPreviewRenderResult
    {
        juce::AudioBuffer<float> buffer;
        PresetPreviewInfo info;
        bool rendered = false;
    };

    struct GlobalEditSnapshot
    {
        juce::ValueTree state;
        juce::String label;
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
    juce::MidiBuffer chordMemoryScratchMidi;

    std::atomic<float>* outputGain = nullptr;
    std::atomic<float>* sampleRecordSource = nullptr;
    std::atomic<float>* sampleRecordStart = nullptr;
    std::atomic<float>* sampleRecordLength = nullptr;
    std::atomic<float>* sampleRecordPreRoll = nullptr;
    std::atomic<float>* sequencerChordMemory = nullptr;
    std::atomic<float>* sequencerLockDestination = nullptr;
    std::atomic<float>* sequencerLockDepth = nullptr;
    std::atomic<float> outputMeterPeakLeft { 0.0f };
    std::atomic<float> outputMeterPeakRight { 0.0f };
    std::atomic<float> outputMeterRmsLeft { 0.0f };
    std::atomic<float> outputMeterRmsRight { 0.0f };
    std::atomic<float> stereoFieldCorrelation { 0.0f };
    std::atomic<float> stereoFieldWidth { 0.0f };
    std::atomic<float> stereoFieldBalance { 0.0f };
    std::atomic<float> lowEndSubRms { 0.0f };
    std::atomic<float> lowEndStereoRisk { 0.0f };
    std::atomic<float> lowEndOutputPeak { 0.0f };
    std::array<std::atomic<float>, outputSpectrumSnapshotSize> outputSpectrumSamples {};
    std::atomic<uint32_t> outputSpectrumWriteIndex { 0 };
    std::atomic<float> hostSyncBpm { 124.0f };
    std::atomic<float> hostSyncPpqPosition { 0.0f };
    std::atomic<bool> hostSyncPositionAvailable { false };
    std::atomic<bool> hostSyncPlaying { false };
    std::atomic<bool> hostSyncPpqAvailable { false };
    std::atomic<float> performanceModWheel { 0.0f };
    std::atomic<float> performanceModAftertouch { 0.0f };
    std::atomic<float> performanceModPitchBend { 0.0f };
    std::atomic<float> performanceModNote { 0.0f };
    std::atomic<int> performanceModActiveNotes { 0 };
    std::atomic<bool> panicRequested { false };
    float lowEndStateLeft = 0.0f;
    float lowEndStateRight = 0.0f;
    uint32_t outputSpectrumWriteCursor = 0;
    double meterSampleRate = 44100.0;
    int preparedSamplesPerBlock = 512;
    juce::String loadedSamplePath;
    mutable juce::CriticalSection sampleCaptureTakeLock;
    std::vector<juce::File> sampleCaptureTakeFiles;
    juce::String selectedSampleCaptureTakePath;
    static constexpr size_t sampleCaptureBufferBankSize = 2;
    std::array<juce::AudioBuffer<float>, sampleCaptureBufferBankSize> sampleCaptureBuffers;
    juce::AudioBuffer<float> sampleCapturePreRollBuffer;
    std::atomic<bool> sampleCaptureEnabled { false };
    std::atomic<bool> sampleCaptureWaitingForThreshold { false };
    std::atomic<int> activeSampleCaptureBufferIndex { 0 };
    std::atomic<uint32_t> sampleCaptureSessionSerial { 1 };
    std::atomic<int> sampleCaptureActiveWriters { 0 };
    std::atomic<int> sampleCaptureWritePosition { 0 };
    std::atomic<int> sampleCaptureSamplesRecorded { 0 };
    std::atomic<int> sampleCaptureTargetSamples { 0 };
    std::atomic<int> sampleCapturePreRollWritePosition { 0 };
    std::atomic<int> sampleCapturePreRollSamplesReady { 0 };
    std::atomic<float> sampleCaptureSourcePeak { 0.0f };
    double sampleCaptureSampleRate = 44100.0;
    juce::SpinLock presetPreviewLock;
    juce::AudioBuffer<float> presetPreviewPlaybackBuffer;
    std::atomic<bool> presetPreviewPlaying { false };
    int presetPreviewPlaybackPosition = 0;
    mutable bool presetLibraryCacheValid = false;
    mutable std::vector<PresetInfo> presetLibraryCache;
    std::mt19937 sampleRandomEngine;
    juce::ValueTree randomUndoState;
    juce::ValueTree randomRedoState;
    juce::ValueTree randomCandidateAuditionReturnState;
    juce::ValueTree sequencerUndoState;
    std::array<juce::ValueTree, 4> performanceSnapshots;
    std::array<juce::ValueTree, 4> sequencerPatternScenes;
    std::atomic<int> sequencerSceneChainPlaybackEnabled { 0 };
    std::atomic<int> sequencerSceneChainClipBars { 0 };
    std::array<RandomCandidateSnapshot, 4> randomCandidateSnapshots;
    std::vector<GlobalEditSnapshot> globalUndoStack;
    std::vector<GlobalEditSnapshot> globalRedoStack;
    juce::String randomUndoLabel;
    juce::String randomRedoLabel;
    juce::String lastRandomValidationSummary;
    int nextRandomCandidateSlot = 0;
    int activeRandomCandidateSlot = -1;
    int auditioningRandomCandidateSlot = -1;
    bool hasRandomUndoState = false;
    bool hasRandomRedoState = false;
    bool hasSequencerUndoState = false;

    void runRandomAction(RandomAction action, int mutationScopeIndex = 0);
    static juce::String randomActionLabel(RandomAction action);
    static juce::String randomMutationScopeLabel(RandomMutationScope mutationScope);
    static RandomMutationScope randomMutationScopeFromIndex(int mutationScopeIndex);
    static float readStateParameterValue(const juce::ValueTree& state, const char* parameterID, float fallback);
    juce::StringArray getRandomCandidateChangedSections(int slotIndex);
    juce::String currentRandomRecipeName() const;
    RandomValidationResult applyRandomGenerationGuardrails(RandomMutationScope mutationScope);
    RandomValidationResult applyRandomRenderValidation(bool fxLocked);
    RandomValidationResult applyRandomExhaustionFallback(const juce::ValueTree& originalState,
                                                        RandomMutationScope mutationScope);
    RandomRenderMetrics renderRandomValidationSnippet();
    void resetRandomValidationRenderState();
    void captureRandomCandidateSnapshot(RandomAction action, RandomMutationScope mutationScope);
    bool isRandomLockEnabled(const juce::String& parameterID) const;
    float getParameterPlainValue(const juce::String& parameterID, float fallback) const;
    float getParameterPlainValueFromState(const juce::ValueTree& state, const juce::String& parameterID, float fallback) const;
    void restoreParameterFromState(const juce::ValueTree& state, const juce::String& parameterID);
    void restoreParameterGroupFromState(const juce::ValueTree& state, std::initializer_list<const char*> parameterIDs);
    void blendParameterFromState(const juce::ValueTree& state, const juce::String& parameterID, float intensity);
    void blendParameterGroupFromState(const juce::ValueTree& state, std::initializer_list<const char*> parameterIDs, float intensity);
    void restoreDiscreteParameterFromStateIfNeeded(const juce::ValueTree& state, const juce::String& parameterID, float intensity);
    void restoreDiscreteParameterGroupFromStateIfNeeded(const juce::ValueTree& state, std::initializer_list<const char*> parameterIDs, float intensity);
    float randomSectionIntensity(RandomMutationScope mutationScope) const;
    void applyRandomSectionIntensities(const juce::ValueTree& state, RandomMutationScope mutationScope);
    void applyRandomSectionIntensity(const juce::ValueTree& state, RandomMutationScope mutationScope, float intensity);
    void restoreSectionsOutsideMutationScope(const juce::ValueTree& state, RandomMutationScope mutationScope);
    void restoreMutationScopeFromState(const juce::ValueTree& state, RandomMutationScope mutationScope);
    void restoreModulationFromState(const juce::ValueTree& state);
    void restoreOutputFromState(const juce::ValueTree& state);
    void restoreLockedSectionsFromState(const juce::ValueTree& state);
    bool restoreSampleFileReference(const juce::String& samplePath);
    void restoreSampleFromState(const juce::ValueTree& state);
    void restoreSequencerFromState(const juce::ValueTree& state);
    juce::ValueTree createSequencerSceneState() const;
    void buildSequencerFourBarSceneChain();
    void restoreSequencerScenesFromState(const juce::ValueTree& state);
    void appendSequencerScenesToState(juce::ValueTree& state) const;
    void removeSequencerSceneChildren(juce::ValueTree& state) const;
    std::vector<juce::ValueTree> createSequencerSceneChainSegments(int clipBars) const;
    void refreshSequencerSceneChainPlayback();
    bool writeSequencerMidiFile(const juce::File& destination, const std::vector<juce::ValueTree>& segments) const;
    void captureSequencerUndoState();
    void setParameterPlainValue(const juce::String& parameterID, float plainValue);
    void resetSampleParametersForNewSource(const juce::String& path);
    juce::AudioBuffer<float>& getActiveSampleCaptureBuffer() noexcept;
    const juce::AudioBuffer<float>& getActiveSampleCaptureBuffer() const noexcept;
    int getNextSampleCaptureBufferIndex() const noexcept;
    void invalidateSampleCaptureSession() noexcept;
    void updateSampleCaptureSourcePeak(const juce::AudioBuffer<float>& buffer, int sourceChannelLimit = -1) noexcept;
    float getSampleCaptureThresholdGain() const noexcept;
    int calculateSampleCaptureTargetSamples() const;
    int calculateSampleCapturePreRollSamples() const noexcept;
    void appendToSampleCapturePreRoll(const juce::AudioBuffer<float>& buffer, int sourceChannelLimit = -1) noexcept;
    void flushSampleCapturePreRoll() noexcept;
    int appendSamplesToSampleCaptureBuffer(const juce::AudioBuffer<float>& source,
                                           int sourceChannelLimit,
                                           int sourceOffset,
                                           int requestedSamples) noexcept;
    void appendToSampleCapture(const juce::AudioBuffer<float>& buffer, int sourceChannelLimit = -1) noexcept;
    void waitForSampleCaptureWritersToFinish();
    void rememberSampleCaptureTake(const juce::File& file);
    void updatePerformanceModulationStatus(const juce::MidiBuffer& midiMessages) noexcept;
    void resetPerformanceModulationStatus() noexcept;
    void applyChordMemoryToMidi(juce::MidiBuffer& midiMessages);
    void clearChordMemoryActiveNotes();
    double getHostBpm() const;
    Sequencer::HostPosition getHostPosition() const;
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer) noexcept;
    void updateOutputSpectrumSnapshot(const juce::AudioBuffer<float>& buffer) noexcept;
    void mixPresetPreviewPlayback(juce::AudioBuffer<float>& buffer) noexcept;
    PresetPreviewRenderResult renderPresetPreviewBuffer(const juce::String& presetName) const;
    bool savePresetState(const juce::String& presetName, const PresetSaveOptions& options, juce::ValueTree state);
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
    juce::File presetPreviewFileForName(const juce::String& presetName) const;
    juce::File findPresetFileInDirectory(const juce::File& directory, const juce::String& presetName) const;
    juce::File presetDirectoryForCategory(const juce::String& category) const;
    juce::File libraryStateFile() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessor)
};
