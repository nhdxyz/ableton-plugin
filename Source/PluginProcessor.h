#pragma once

#include "Effects/EffectsRack.h"
#include "Parameters.h"
#include "Randomization/Randomizer.h"
#include "Sampler/SamplePlayer.h"
#include "Sequencer/PatternSequencer.h"
#include "Synth/SynthEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <random>

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
    bool loadSampleFile(const juce::File& file);
    void clearSample();
    void randomizeSampleCut();
    juce::String getLoadedSampleName() const;
    Sequencer::Step getSequencerStep(int index) const;
    void setSequencerStep(int index, Sequencer::Step step);
    void randomizeSequencerPattern();
    void clearSequencerPattern();
    bool savePreset(const juce::String& presetName);
    bool loadPreset(const juce::String& presetName);
    juce::StringArray getPresetNames() const;
    juce::File getPresetDirectory() const;

private:
    Parameters::APVTS parameters;
    Synth::SynthEngine synthEngine;
    Randomization::Randomizer randomizer;
    Sampler::SamplePlayer samplePlayer;
    Sequencer::PatternSequencer patternSequencer;
    Effects::EffectsRack effectsRack;

    std::atomic<float>* outputGain = nullptr;
    juce::String loadedSamplePath;
    std::mt19937 sampleRandomEngine;

    void setParameterPlainValue(const juce::String& parameterID, float plainValue);
    double getHostBpm() const;
    juce::ValueTree createPluginState();
    void restorePluginState(const juce::ValueTree& state);
    juce::File presetFileForName(const juce::String& presetName) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessor)
};
