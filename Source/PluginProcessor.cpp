#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
const juce::Identifier libraryStateType { "NateVSTLibrary" };
const juce::Identifier favoritesType { "Favorites" };
const juce::Identifier recentType { "Recent" };
const juce::Identifier presetRefType { "Preset" };
}

NateVSTAudioProcessor::NateVSTAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", Parameters::createLayout()),
      synthEngine(parameters),
      randomizer(parameters),
      samplePlayer(parameters),
      patternSequencer(parameters),
      effectsRack(parameters),
      sampleRandomEngine(std::random_device{}())
{
    outputGain = parameters.getRawParameterValue(Parameters::ID::outputGain);
}

void NateVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthEngine.prepare(sampleRate, samplesPerBlock);
    samplePlayer.prepare(sampleRate);
    patternSequencer.prepare(sampleRate);
    effectsRack.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    outputMeterPeakLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterPeakRight.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsRight.store(0.0f, std::memory_order_relaxed);
}

void NateVSTAudioProcessor::releaseResources()
{
    effectsRack.reset();
    patternSequencer.reset();
}

bool NateVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
}

void NateVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
    const auto hostBpm = getHostBpm();
    patternSequencer.process(midiMessages, buffer.getNumSamples(), hostBpm);
    synthEngine.render(buffer, midiMessages);
    samplePlayer.render(buffer, midiMessages);
    effectsRack.process(buffer,
                        outputGain != nullptr ? outputGain->load() : -8.0f,
                        hostBpm,
                        getHostPpqPosition());
    updateOutputMeters(buffer);
}

juce::AudioProcessorEditor* NateVSTAudioProcessor::createEditor()
{
    return new NateVSTAudioProcessorEditor(*this);
}

bool NateVSTAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String NateVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NateVSTAudioProcessor::acceptsMidi() const
{
    return true;
}

bool NateVSTAudioProcessor::producesMidi() const
{
    return false;
}

bool NateVSTAudioProcessor::isMidiEffect() const
{
    return false;
}

double NateVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NateVSTAudioProcessor::getNumPrograms()
{
    return 1;
}

int NateVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NateVSTAudioProcessor::setCurrentProgram(int)
{
}

const juce::String NateVSTAudioProcessor::getProgramName(int)
{
    return {};
}

void NateVSTAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void NateVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = createPluginState().createXml())
        copyXmlToBinary(*xml, destData);
}

void NateVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        if (xmlState->hasTagName(parameters.state.getType()))
            restorePluginState(juce::ValueTree::fromXml(*xmlState));
}

Parameters::APVTS& NateVSTAudioProcessor::getValueTreeState()
{
    return parameters;
}

void NateVSTAudioProcessor::generateRandomPatch()
{
    runRandomAction(RandomAction::generate);
}

void NateVSTAudioProcessor::mutateRandomPatch()
{
    runRandomAction(RandomAction::mutate);
}

void NateVSTAudioProcessor::createRandomVariation()
{
    runRandomAction(RandomAction::variation);
}

bool NateVSTAudioProcessor::undoRandomization()
{
    if (! hasRandomUndoState || ! randomUndoState.isValid())
        return false;

    restorePluginState(randomUndoState.createCopy());
    hasRandomUndoState = false;
    return true;
}

bool NateVSTAudioProcessor::loadSampleFile(const juce::File& file)
{
    if (! samplePlayer.loadFile(file))
        return false;

    loadedSamplePath = file.getFullPathName();
    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleEnd, 1.0f);
    return true;
}

void NateVSTAudioProcessor::clearSample()
{
    loadedSamplePath.clear();
    samplePlayer.clear();
    setParameterPlainValue(Parameters::ID::sampleEnabled, 0.0f);
}

bool NateVSTAudioProcessor::randomizeSampleCut()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSample))
        return false;

    if (! samplePlayer.hasSample())
        return false;

    std::uniform_real_distribution<float> startDistribution(0.0f, 0.82f);
    std::uniform_real_distribution<float> lengthDistribution(0.04f, 0.45f);
    std::uniform_real_distribution<float> transposeDistribution(-12.0f, 12.0f);
    std::uniform_real_distribution<float> gainDistribution(-12.0f, -2.0f);
    std::uniform_real_distribution<float> mixDistribution(0.45f, 1.0f);
    std::bernoulli_distribution reverseDistribution(0.25);

    const auto start = startDistribution(sampleRandomEngine);
    const auto end = juce::jlimit(0.02f, 1.0f, start + lengthDistribution(sampleRandomEngine));

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, start);
    setParameterPlainValue(Parameters::ID::sampleEnd, end);
    setParameterPlainValue(Parameters::ID::sampleReverse, reverseDistribution(sampleRandomEngine) ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::sampleTranspose, transposeDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleGain, gainDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleMix, mixDistribution(sampleRandomEngine));
    return true;
}

juce::String NateVSTAudioProcessor::getLoadedSampleName() const
{
    return samplePlayer.getLoadedFileName();
}

Sequencer::Step NateVSTAudioProcessor::getSequencerStep(int index) const
{
    return patternSequencer.getStep(index);
}

void NateVSTAudioProcessor::setSequencerStep(int index, Sequencer::Step step)
{
    patternSequencer.setStep(index, step);
}

bool NateVSTAudioProcessor::randomizeSequencerPattern()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        return false;

    const auto amount = parameters.getRawParameterValue(Parameters::ID::sequencerRandomAmount);
    patternSequencer.randomize(amount != nullptr ? amount->load() : 0.55f);
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);

    std::uniform_real_distribution<float> swingDistribution(0.0f, 0.32f);
    std::uniform_real_distribution<float> accentDistribution(0.2f, 0.75f);
    std::uniform_real_distribution<float> probabilityDistribution(0.72f, 1.0f);
    std::uniform_int_distribution<int> octaveDistribution(-1, 1);
    setParameterPlainValue(Parameters::ID::sequencerSwing, swingDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerAccent, accentDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerOctave, static_cast<float>(octaveDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sequencerProbability, probabilityDistribution(sampleRandomEngine));
    return true;
}

void NateVSTAudioProcessor::applySequencerPatternPreset(int presetIndex)
{
    patternSequencer.clear();
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);

    auto setStep = [this] (int index, int noteOffset, float velocity, float probability)
    {
        Sequencer::Step step;
        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = velocity;
        step.probability = probability;
        patternSequencer.setStep(index, step);
    };

    switch (presetIndex)
    {
        case 2:
            setStep(0, 0, 0.88f, 1.0f);
            setStep(2, 7, 0.62f, 0.85f);
            setStep(5, 3, 0.78f, 0.95f);
            setStep(7, 10, 0.54f, 0.72f);
            setStep(9, 0, 0.82f, 1.0f);
            setStep(11, -5, 0.66f, 0.82f);
            setStep(14, 7, 0.76f, 0.92f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.58f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.93f);
            break;

        case 1:
            setStep(2, 0, 0.72f, 1.0f);
            setStep(6, 7, 0.86f, 0.95f);
            setStep(10, 3, 0.76f, 0.9f);
            setStep(14, 10, 0.9f, 0.85f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.94f);
            break;

        case 0:
        default:
            setStep(0, 0, 0.92f, 1.0f);
            setStep(3, 0, 0.68f, 0.85f);
            setStep(6, -5, 0.78f, 0.9f);
            setStep(8, 0, 0.88f, 1.0f);
            setStep(11, 3, 0.7f, 0.8f);
            setStep(14, -2, 0.74f, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.62f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.96f);
            break;
    }
}

void NateVSTAudioProcessor::copySequencerFirstHalfToSecondHalf()
{
    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps / 2; ++stepIndex)
        patternSequencer.setStep(stepIndex + (Sequencer::PatternSequencer::numSteps / 2), patternSequencer.getStep(stepIndex));
}

void NateVSTAudioProcessor::clearSequencerPattern()
{
    patternSequencer.clear();
}

bool NateVSTAudioProcessor::savePreset(const juce::String& presetName)
{
    return savePreset(presetName, "User");
}

bool NateVSTAudioProcessor::savePreset(const juce::String& presetName, const juce::String& category)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    const auto directory = getPresetDirectory();
    if (! directory.createDirectory())
        return false;

    auto state = createPluginState();
    state.setProperty("preset_name", trimmedName, nullptr);
    state.setProperty("preset_category", category.trim().isNotEmpty() ? category.trim() : "User", nullptr);
    state.setProperty("preset_author", "User", nullptr);
    state.setProperty("preset_source", "User", nullptr);

    if (auto xml = state.createXml())
        return xml->writeTo(presetFileForName(trimmedName));

    return false;
}

bool NateVSTAudioProcessor::loadPreset(const juce::String& presetName)
{
    if (presetName.trim().isEmpty())
        return false;

    auto file = presetFileForName(presetName);
    if (! file.existsAsFile())
        file = factoryPresetFileForName(presetName);

    if (! file.existsAsFile())
        return false;

    if (auto xml = juce::XmlDocument::parse(file))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(parameters.state.getType()))
        {
            restorePluginState(state);
            notePresetLoaded(file.getFileNameWithoutExtension());
            return true;
        }
    }

    return false;
}

juce::StringArray NateVSTAudioProcessor::getPresetNames() const
{
    juce::StringArray names;

    for (const auto& preset : getPresetLibrary())
        names.add(preset.name);

    names.sort(true);
    return names;
}

std::vector<NateVSTAudioProcessor::PresetInfo> NateVSTAudioProcessor::getPresetLibrary() const
{
    std::vector<PresetInfo> presets;
    juce::StringArray seenNames;
    const auto favorites = getLibraryStateNames(favoritesType);

    auto collectFromDirectory = [&presets, &seenNames, &favorites] (const juce::File& directory, bool isFactory)
    {
        const auto presetFiles = directory.findChildFiles(juce::File::findFiles, false, "*.natevstpreset");

        for (const auto& file : presetFiles)
        {
            const auto name = file.getFileNameWithoutExtension();
            if (seenNames.contains(name))
                continue;

            auto category = isFactory ? juce::String("Factory") : juce::String("User");
            auto source = isFactory ? juce::String("Factory") : juce::String("User");

            if (auto xml = juce::XmlDocument::parse(file))
            {
                const auto state = juce::ValueTree::fromXml(*xml);
                if (state.isValid())
                {
                    const auto storedCategory = state.getProperty("preset_category").toString().trim();
                    if (storedCategory.isNotEmpty())
                        category = storedCategory;

                    const auto storedSource = state.getProperty("preset_source").toString().trim();
                    if (storedSource.isNotEmpty())
                        source = storedSource;
                }
            }

            presets.push_back({ name, category, source, isFactory, favorites.contains(name) });
            seenNames.add(name);
        }
    };

    collectFromDirectory(getPresetDirectory(), false);
    collectFromDirectory(getFactoryPresetDirectory(), true);

    std::sort(presets.begin(), presets.end(), [] (const PresetInfo& left, const PresetInfo& right)
    {
        return left.name.compareIgnoreCase(right.name) < 0;
    });

    return presets;
}

juce::File NateVSTAudioProcessor::getPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Nate VST")
        .getChildFile("Presets");
}

juce::File NateVSTAudioProcessor::getFactoryPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Nate VST")
        .getChildFile("Factory Presets");
}

bool NateVSTAudioProcessor::isPresetFavorite(const juce::String& presetName) const
{
    return getLibraryStateNames(favoritesType).contains(presetName.trim());
}

bool NateVSTAudioProcessor::setPresetFavorite(const juce::String& presetName, bool shouldBeFavorite)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    auto state = loadLibraryState();
    auto favorites = state.getChildWithName(favoritesType);
    if (! favorites.isValid())
    {
        favorites = juce::ValueTree(favoritesType);
        state.addChild(favorites, -1, nullptr);
    }

    for (auto index = favorites.getNumChildren(); --index >= 0;)
        if (favorites.getChild(index).getProperty("name").toString() == trimmedName)
            favorites.removeChild(index, nullptr);

    if (shouldBeFavorite)
    {
        juce::ValueTree favorite(presetRefType);
        favorite.setProperty("name", trimmedName, nullptr);
        favorites.addChild(favorite, -1, nullptr);
    }

    return saveLibraryState(state);
}

juce::StringArray NateVSTAudioProcessor::getRecentPresetNames() const
{
    return getLibraryStateNames(recentType);
}

void NateVSTAudioProcessor::notePresetLoaded(const juce::String& presetName)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return;

    auto state = loadLibraryState();
    auto recent = state.getChildWithName(recentType);
    if (! recent.isValid())
    {
        recent = juce::ValueTree(recentType);
        state.addChild(recent, -1, nullptr);
    }

    for (auto index = recent.getNumChildren(); --index >= 0;)
        if (recent.getChild(index).getProperty("name").toString() == trimmedName)
            recent.removeChild(index, nullptr);

    juce::ValueTree item(presetRefType);
    item.setProperty("name", trimmedName, nullptr);
    recent.addChild(item, 0, nullptr);

    while (recent.getNumChildren() > 12)
        recent.removeChild(recent.getNumChildren() - 1, nullptr);

    saveLibraryState(state);
}

juce::MidiKeyboardState& NateVSTAudioProcessor::getMidiKeyboardState() noexcept
{
    return midiKeyboardState;
}

void NateVSTAudioProcessor::getOutputMeterLevels(float& peakLeft,
                                                 float& peakRight,
                                                 float& rmsLeft,
                                                 float& rmsRight) const noexcept
{
    peakLeft = outputMeterPeakLeft.load(std::memory_order_relaxed);
    peakRight = outputMeterPeakRight.load(std::memory_order_relaxed);
    rmsLeft = outputMeterRmsLeft.load(std::memory_order_relaxed);
    rmsRight = outputMeterRmsRight.load(std::memory_order_relaxed);
}

juce::String NateVSTAudioProcessor::getActiveRandomizationLockSummary() const
{
    juce::StringArray locks;

    auto addIfLocked = [this, &locks] (const juce::String& parameterID, const juce::String& label)
    {
        if (isRandomLockEnabled(parameterID))
            locks.add(label);
    };

    addIfLocked(Parameters::ID::randomLockPitch, "Pitch");
    addIfLocked(Parameters::ID::randomLockEnvelope, "Env");
    addIfLocked(Parameters::ID::randomLockFilter, "Filter");
    addIfLocked(Parameters::ID::randomLockSource, "Source");
    addIfLocked(Parameters::ID::randomLockSample, "Sample");
    addIfLocked(Parameters::ID::randomLockFx, "FX");
    addIfLocked(Parameters::ID::randomLockOutput, "Output");
    addIfLocked(Parameters::ID::randomLockSequencer, "Seq");

    return locks.joinIntoString(", ");
}

void NateVSTAudioProcessor::runRandomAction(RandomAction action)
{
    const auto snapshot = createPluginState();
    randomUndoState = snapshot.createCopy();
    hasRandomUndoState = true;

    switch (action)
    {
        case RandomAction::generate:
            randomizer.generate();
            break;
        case RandomAction::mutate:
            randomizer.mutate();
            break;
        case RandomAction::variation:
            randomizer.variation();
            break;
    }

    restoreLockedSectionsFromState(snapshot);
}

bool NateVSTAudioProcessor::isRandomLockEnabled(const juce::String& parameterID) const
{
    if (auto* value = parameters.getRawParameterValue(parameterID))
        return value->load() >= 0.5f;

    return false;
}

float NateVSTAudioProcessor::getParameterPlainValue(const juce::String& parameterID, float fallback) const
{
    if (auto* value = parameters.getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

float NateVSTAudioProcessor::getParameterPlainValueFromState(const juce::ValueTree& state,
                                                              const juce::String& parameterID,
                                                              float fallback) const
{
    const auto parameterState = state.getChildWithProperty("id", parameterID);
    if (parameterState.isValid())
        return static_cast<float>(parameterState.getProperty("value", fallback));

    return fallback;
}

void NateVSTAudioProcessor::restoreParameterFromState(const juce::ValueTree& state, const juce::String& parameterID)
{
    const auto parameterState = state.getChildWithProperty("id", parameterID);
    if (! parameterState.isValid())
        return;

    setParameterPlainValue(parameterID, static_cast<float>(parameterState.getProperty("value", getParameterPlainValue(parameterID, 0.0f))));
}

void NateVSTAudioProcessor::restoreParameterGroupFromState(const juce::ValueTree& state,
                                                            std::initializer_list<const char*> parameterIDs)
{
    for (const auto* parameterID : parameterIDs)
        restoreParameterFromState(state, parameterID);
}

void NateVSTAudioProcessor::restoreLockedSectionsFromState(const juce::ValueTree& state)
{
    if (isRandomLockEnabled(Parameters::ID::randomLockPitch))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::oscOctave,
            Parameters::ID::oscTune,
            Parameters::ID::osc2Octave,
            Parameters::ID::osc2Tune,
            Parameters::ID::monoMode,
            Parameters::ID::glideTime
        });
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockEnvelope))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::ampAttack,
            Parameters::ID::ampDecay,
            Parameters::ID::ampSustain,
            Parameters::ID::ampRelease
        });
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockFilter))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::filterCutoff,
            Parameters::ID::filterResonance,
            Parameters::ID::filterEnvAmount,
            Parameters::ID::filterMode
        });
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockSource))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::oscWave,
            Parameters::ID::osc1Level,
            Parameters::ID::osc2Wave,
            Parameters::ID::osc2Level,
            Parameters::ID::subLevel,
            Parameters::ID::noiseLevel,
            Parameters::ID::unisonVoices,
            Parameters::ID::unisonDetune,
            Parameters::ID::unisonBlend,
            Parameters::ID::unisonSpread
        });
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockSample))
        restoreSampleFromState(state);

    if (isRandomLockEnabled(Parameters::ID::randomLockFx))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::fxDistortionEnabled,
            Parameters::ID::fxDistortionAmount,
            Parameters::ID::fxBitcrushEnabled,
            Parameters::ID::fxBitcrushBits,
            Parameters::ID::fxBitcrushDownsample,
            Parameters::ID::fxBitcrushMix,
            Parameters::ID::fxPumpEnabled,
            Parameters::ID::fxPumpRate,
            Parameters::ID::fxPumpDepth,
            Parameters::ID::fxPumpShape,
            Parameters::ID::fxPumpPhase,
            Parameters::ID::fxChorusEnabled,
            Parameters::ID::fxChorusRate,
            Parameters::ID::fxChorusDepth,
            Parameters::ID::fxChorusMix,
            Parameters::ID::fxDelayEnabled,
            Parameters::ID::fxDelayTime,
            Parameters::ID::fxDelayFeedback,
            Parameters::ID::fxDelayMix,
            Parameters::ID::fxReverbEnabled,
            Parameters::ID::fxReverbSize,
            Parameters::ID::fxReverbDamping,
            Parameters::ID::fxReverbMix,
            Parameters::ID::fxWidthEnabled,
            Parameters::ID::fxWidthAmount,
            Parameters::ID::fxWidthMonoCutoff,
            Parameters::ID::fxToneEnabled,
            Parameters::ID::fxToneTilt,
            Parameters::ID::fxToneLowCut,
            Parameters::ID::fxPhaserEnabled,
            Parameters::ID::fxPhaserRate,
            Parameters::ID::fxPhaserDepth,
            Parameters::ID::fxPhaserMix,
            Parameters::ID::fxGuardEnabled,
            Parameters::ID::fxGuardPush,
            Parameters::ID::fxGuardCeiling
        });
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockOutput))
    {
        const auto previousOutput = getParameterPlainValueFromState(state,
                                                                    Parameters::ID::outputGain,
                                                                    getParameterPlainValue(Parameters::ID::outputGain, -8.0f));
        const auto randomizedSafeOutput = getParameterPlainValue(Parameters::ID::outputGain, previousOutput);
        setParameterPlainValue(Parameters::ID::outputGain, juce::jmin(previousOutput, randomizedSafeOutput));
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        restoreSequencerFromState(state);
}

void NateVSTAudioProcessor::restoreSampleFromState(const juce::ValueTree& state)
{
    loadedSamplePath = state.getProperty("sample_file").toString();

    if (loadedSamplePath.isNotEmpty())
        samplePlayer.loadFile(juce::File(loadedSamplePath));
    else
        samplePlayer.clear();

    restoreParameterGroupFromState(state, {
        Parameters::ID::sampleEnabled,
        Parameters::ID::sampleStart,
        Parameters::ID::sampleEnd,
        Parameters::ID::sampleReverse,
        Parameters::ID::sampleTranspose,
        Parameters::ID::sampleGain,
        Parameters::ID::sampleMix
    });
}

void NateVSTAudioProcessor::restoreSequencerFromState(const juce::ValueTree& state)
{
    restoreParameterGroupFromState(state, {
        Parameters::ID::sequencerEnabled,
        Parameters::ID::sequencerRate,
        Parameters::ID::sequencerRoot,
        Parameters::ID::sequencerGate,
        Parameters::ID::sequencerSwing,
        Parameters::ID::sequencerAccent,
        Parameters::ID::sequencerOctave,
        Parameters::ID::sequencerProbability,
        Parameters::ID::sequencerRandomAmount
    });

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        Sequencer::Step step;
        step.enabled = static_cast<bool>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_enabled", false));
        step.noteOffset = static_cast<int>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_note", 0));
        step.velocity = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_velocity", 0.8f));
        step.probability = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_probability", 1.0f));
        patternSequencer.setStep(stepIndex, step);
    }
}

void NateVSTAudioProcessor::setParameterPlainValue(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = parameters.getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}

double NateVSTAudioProcessor::getHostBpm() const
{
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto bpm = position->getBpm())
                return *bpm;

    return 124.0;
}

std::optional<double> NateVSTAudioProcessor::getHostPpqPosition() const
{
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto ppqPosition = position->getPpqPosition())
                return *ppqPosition;

    return std::nullopt;
}

void NateVSTAudioProcessor::updateOutputMeters(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
    {
        outputMeterPeakLeft.store(0.0f, std::memory_order_relaxed);
        outputMeterPeakRight.store(0.0f, std::memory_order_relaxed);
        outputMeterRmsLeft.store(0.0f, std::memory_order_relaxed);
        outputMeterRmsRight.store(0.0f, std::memory_order_relaxed);
        return;
    }

    auto measureChannel = [numSamples] (const float* samples, float& peak, float& rms) noexcept
    {
        auto localPeak = 0.0f;
        auto sumSquares = 0.0;

        for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            const auto sample = samples[sampleIndex];
            localPeak = juce::jmax(localPeak, std::abs(sample));
            sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
        }

        peak = localPeak;
        rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(numSamples)));
    };

    auto peakLeft = 0.0f;
    auto peakRight = 0.0f;
    auto rmsLeft = 0.0f;
    auto rmsRight = 0.0f;

    measureChannel(buffer.getReadPointer(0), peakLeft, rmsLeft);
    measureChannel(buffer.getReadPointer(numChannels > 1 ? 1 : 0), peakRight, rmsRight);

    outputMeterPeakLeft.store(peakLeft, std::memory_order_relaxed);
    outputMeterPeakRight.store(peakRight, std::memory_order_relaxed);
    outputMeterRmsLeft.store(rmsLeft, std::memory_order_relaxed);
    outputMeterRmsRight.store(rmsRight, std::memory_order_relaxed);
}

juce::ValueTree NateVSTAudioProcessor::createPluginState()
{
    auto state = parameters.copyState();
    state.setProperty("nate_vst_state_version", 1, nullptr);
    state.setProperty("sample_file", loadedSamplePath, nullptr);

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        const auto step = patternSequencer.getStep(stepIndex);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_enabled", step.enabled, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_note", step.noteOffset, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_velocity", step.velocity, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_probability", step.probability, nullptr);
    }

    return state;
}

void NateVSTAudioProcessor::restorePluginState(const juce::ValueTree& state)
{
    loadedSamplePath = state.getProperty("sample_file").toString();
    parameters.replaceState(state);

    if (loadedSamplePath.isNotEmpty())
        samplePlayer.loadFile(juce::File(loadedSamplePath));
    else
        samplePlayer.clear();

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        Sequencer::Step step;
        step.enabled = static_cast<bool>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_enabled", false));
        step.noteOffset = static_cast<int>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_note", 0));
        step.velocity = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_velocity", 0.8f));
        step.probability = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_probability", 1.0f));
        patternSequencer.setStep(stepIndex, step);
    }
}

juce::ValueTree NateVSTAudioProcessor::loadLibraryState() const
{
    const auto file = libraryStateFile();
    if (file.existsAsFile())
        if (auto xml = juce::XmlDocument::parse(file))
            if (xml->hasTagName(libraryStateType.toString()))
                return juce::ValueTree::fromXml(*xml);

    auto state = juce::ValueTree(libraryStateType);
    state.addChild(juce::ValueTree(favoritesType), -1, nullptr);
    state.addChild(juce::ValueTree(recentType), -1, nullptr);
    return state;
}

bool NateVSTAudioProcessor::saveLibraryState(const juce::ValueTree& state) const
{
    const auto file = libraryStateFile();
    if (! file.getParentDirectory().createDirectory())
        return false;

    if (auto xml = state.createXml())
        return xml->writeTo(file);

    return false;
}

juce::StringArray NateVSTAudioProcessor::getLibraryStateNames(const juce::Identifier& childType) const
{
    juce::StringArray names;
    const auto state = loadLibraryState();
    const auto child = state.getChildWithName(childType);

    if (child.isValid())
    {
        for (auto index = 0; index < child.getNumChildren(); ++index)
        {
            const auto name = child.getChild(index).getProperty("name").toString().trim();
            if (name.isNotEmpty())
                names.add(name);
        }
    }

    names.removeDuplicates(false);
    return names;
}

juce::File NateVSTAudioProcessor::presetFileForName(const juce::String& presetName) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    return getPresetDirectory().getChildFile(legalName).withFileExtension(".natevstpreset");
}

juce::File NateVSTAudioProcessor::factoryPresetFileForName(const juce::String& presetName) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    return getFactoryPresetDirectory().getChildFile(legalName).withFileExtension(".natevstpreset");
}

juce::File NateVSTAudioProcessor::libraryStateFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Nate VST")
        .getChildFile("Library.xml");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NateVSTAudioProcessor();
}
