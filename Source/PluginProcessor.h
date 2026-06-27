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
        bool isFactory = false;
        bool isFavorite = false;
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
    void mutateRandomPatch();
    void createRandomVariation();
    bool undoRandomization();
    bool loadSampleFile(const juce::File& file);
    void clearSample();
    bool randomizeSampleCut();
    bool randomizeUkgVocalChop();
    bool triggerSampleAudition();
    juce::String getLoadedSampleName() const;
    Sequencer::Step getSequencerStep(int index) const;
    void setSequencerStep(int index, Sequencer::Step step);
    bool randomizeSequencerPattern();
    void applySequencerPatternPreset(int presetIndex);
    void copySequencerFirstHalfToSecondHalf();
    void rotateSequencerPattern(int stepOffset);
    bool exportSequencerMidiFile(const juce::File& destination) const;
    void clearSequencerPattern();
    juce::String getActiveRandomizationLockSummary() const;
    bool savePreset(const juce::String& presetName);
    bool savePreset(const juce::String& presetName, const juce::String& category);
    bool loadPreset(const juce::String& presetName);
    juce::StringArray getPresetNames() const;
    std::vector<PresetInfo> getPresetLibrary() const;
    juce::File getPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const;
    bool isPresetFavorite(const juce::String& presetName) const;
    bool setPresetFavorite(const juce::String& presetName, bool shouldBeFavorite);
    juce::StringArray getRecentPresetNames() const;
    void notePresetLoaded(const juce::String& presetName);
    void capturePerformanceSnapshot(int slotIndex);
    bool recallPerformanceSnapshot(int slotIndex);
    bool hasPerformanceSnapshot(int slotIndex) const;
    juce::MidiKeyboardState& getMidiKeyboardState() noexcept;
    void getOutputMeterLevels(float& peakLeft, float& peakRight, float& rmsLeft, float& rmsRight) const noexcept;
    void getLowEndMeterLevels(float& subRms, float& lowStereoRisk, float& outputPeak) const noexcept;

private:
    enum class RandomAction
    {
        generate,
        mutate,
        variation
    };

    Parameters::APVTS parameters;
    Synth::SynthEngine synthEngine;
    Randomization::Randomizer randomizer;
    Sampler::SamplePlayer samplePlayer;
    Sequencer::PatternSequencer patternSequencer;
    Effects::EffectsRack effectsRack;
    juce::MidiKeyboardState midiKeyboardState;

    std::atomic<float>* outputGain = nullptr;
    std::atomic<float> outputMeterPeakLeft { 0.0f };
    std::atomic<float> outputMeterPeakRight { 0.0f };
    std::atomic<float> outputMeterRmsLeft { 0.0f };
    std::atomic<float> outputMeterRmsRight { 0.0f };
    std::atomic<float> lowEndSubRms { 0.0f };
    std::atomic<float> lowEndStereoRisk { 0.0f };
    std::atomic<float> lowEndOutputPeak { 0.0f };
    float lowEndStateLeft = 0.0f;
    float lowEndStateRight = 0.0f;
    double meterSampleRate = 44100.0;
    juce::String loadedSamplePath;
    std::mt19937 sampleRandomEngine;
    juce::ValueTree randomUndoState;
    std::array<juce::ValueTree, 2> performanceSnapshots;
    bool hasRandomUndoState = false;

    void runRandomAction(RandomAction action);
    bool isRandomLockEnabled(const juce::String& parameterID) const;
    float getParameterPlainValue(const juce::String& parameterID, float fallback) const;
    float getParameterPlainValueFromState(const juce::ValueTree& state, const juce::String& parameterID, float fallback) const;
    void restoreParameterFromState(const juce::ValueTree& state, const juce::String& parameterID);
    void restoreParameterGroupFromState(const juce::ValueTree& state, std::initializer_list<const char*> parameterIDs);
    void restoreLockedSectionsFromState(const juce::ValueTree& state);
    void restoreSampleFromState(const juce::ValueTree& state);
    void restoreSequencerFromState(const juce::ValueTree& state);
    void setParameterPlainValue(const juce::String& parameterID, float plainValue);
    double getHostBpm() const;
    std::optional<double> getHostPpqPosition() const;
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
    juce::File factoryPresetFileForName(const juce::String& presetName) const;
    juce::File libraryStateFile() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessor)
};
