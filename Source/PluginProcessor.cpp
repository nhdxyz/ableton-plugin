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
const juce::Identifier performanceSnapshotType { "PerformanceSnapshot" };
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
    meterSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    synthEngine.prepare(sampleRate, samplesPerBlock);
    samplePlayer.prepare(sampleRate);
    patternSequencer.prepare(sampleRate);
    effectsRack.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    outputMeterPeakLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterPeakRight.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsRight.store(0.0f, std::memory_order_relaxed);
    lowEndSubRms.store(0.0f, std::memory_order_relaxed);
    lowEndStereoRisk.store(0.0f, std::memory_order_relaxed);
    lowEndOutputPeak.store(0.0f, std::memory_order_relaxed);
    lowEndStateLeft = 0.0f;
    lowEndStateRight = 0.0f;
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
    synthEngine.render(buffer, midiMessages, hostBpm);
    samplePlayer.render(buffer, midiMessages, hostBpm);
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
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePitchRamp, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterEnabled, 0.0f);
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
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePitchRamp, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterEnabled, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleTranspose, transposeDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleGain, gainDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleMix, mixDistribution(sampleRandomEngine));
    return true;
}

bool NateVSTAudioProcessor::randomizeUkgVocalChop()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSample))
        return false;

    if (! samplePlayer.hasSample())
        return false;

    constexpr float pitchChoices[] { -12.0f, -7.0f, 0.0f, 7.0f, 12.0f };
    constexpr float rampChoices[] { -12.0f, -7.0f, -5.0f, 5.0f, 7.0f, 12.0f };

    std::uniform_real_distribution<float> startDistribution(0.0f, 0.9f);
    std::uniform_real_distribution<float> lengthDistribution(0.018f, 0.16f);
    std::uniform_int_distribution<int> pitchDistribution(0, 4);
    std::uniform_int_distribution<int> rampDistribution(0, 5);
    std::uniform_int_distribution<int> stutterRateDistribution(1, 2);
    std::uniform_int_distribution<int> stutterRepeatsDistribution(2, 5);
    std::uniform_real_distribution<float> gainDistribution(-10.0f, -3.0f);
    std::uniform_real_distribution<float> mixDistribution(0.62f, 1.0f);
    std::uniform_real_distribution<float> delayTimeDistribution(0.11f, 0.26f);
    std::uniform_real_distribution<float> delayFeedbackDistribution(0.12f, 0.32f);
    std::uniform_real_distribution<float> delayMixDistribution(0.08f, 0.18f);
    std::bernoulli_distribution reverseDistribution(0.22);

    const auto start = startDistribution(sampleRandomEngine);
    const auto end = juce::jlimit(start + 0.012f, 1.0f, start + lengthDistribution(sampleRandomEngine));

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, start);
    setParameterPlainValue(Parameters::ID::sampleEnd, end);
    setParameterPlainValue(Parameters::ID::sampleReverse, reverseDistribution(sampleRandomEngine) ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterRate, static_cast<float>(stutterRateDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleStutterRepeats, static_cast<float>(stutterRepeatsDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleTranspose, pitchChoices[pitchDistribution(sampleRandomEngine)]);
    setParameterPlainValue(Parameters::ID::samplePitchRamp, rampChoices[rampDistribution(sampleRandomEngine)]);
    setParameterPlainValue(Parameters::ID::sampleGain, gainDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleMix, mixDistribution(sampleRandomEngine));

    if (! isRandomLockEnabled(Parameters::ID::randomLockFx))
    {
        setParameterPlainValue(Parameters::ID::fxDelayEnabled, 1.0f);
        setParameterPlainValue(Parameters::ID::fxDelayTime, delayTimeDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::fxDelayFeedback, delayFeedbackDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::fxDelayMix, delayMixDistribution(sampleRandomEngine));
    }

    if (! isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        applySequencerPatternPreset(5);

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
    const auto scale = parameters.getRawParameterValue(Parameters::ID::sequencerScale);
    if (scale == nullptr || scale->load() <= 0.5f)
        setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);

    patternSequencer.randomize(amount != nullptr ? amount->load() : 0.55f);
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);

    std::uniform_real_distribution<float> swingDistribution(0.0f, 0.32f);
    std::uniform_real_distribution<float> accentDistribution(0.2f, 0.75f);
    std::uniform_real_distribution<float> probabilityDistribution(0.72f, 1.0f);
    std::uniform_int_distribution<int> octaveDistribution(-1, 1);
    std::bernoulli_distribution selectiveGrooveDistribution(0.55);
    setParameterPlainValue(Parameters::ID::sequencerSwing, swingDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerGrooveMode, selectiveGrooveDistribution(sampleRandomEngine) ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerAccent, accentDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerOctave, static_cast<float>(octaveDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sequencerProbability, probabilityDistribution(sampleRandomEngine));
    return true;
}

void NateVSTAudioProcessor::applySequencerPatternPreset(int presetIndex)
{
    patternSequencer.clear();
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
    const auto scaleMode = presetIndex == 1 ? 2.0f
        : presetIndex == 4 || presetIndex == 6 ? 2.0f
        : 4.0f;
    setParameterPlainValue(Parameters::ID::sequencerScale, scaleMode);

    auto setStep = [this] (int index, int noteOffset, float velocity, float probability, float timing = 0.0f)
    {
        Sequencer::Step step;
        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = velocity;
        step.probability = probability;
        step.timing = timing;
        patternSequencer.setStep(index, step);
    };

    switch (presetIndex)
    {
        case 6:
            setStep(3, 0, 0.76f, 1.0f, 0.82f);
            setStep(7, 7, 0.86f, 0.92f, 1.0f);
            setStep(10, 3, 0.66f, 0.72f, 0.34f);
            setStep(15, 10, 0.82f, 0.86f, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.38f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.34f);
            break;

        case 5:
            setStep(1, 0, 0.72f, 0.9f, 0.68f);
            setStep(4, 7, 0.86f, 1.0f, 0.0f);
            setStep(6, -5, 0.58f, 0.68f, 0.28f);
            setStep(9, 3, 0.78f, 0.86f, 0.74f);
            setStep(12, 0, 0.7f, 0.78f, 0.0f);
            setStep(14, 12, 0.82f, 0.82f, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.84f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.62f);
            break;

        case 4:
            setStep(2, 0, 0.76f, 1.0f, 0.22f);
            setStep(6, 3, 0.88f, 0.95f, 0.34f);
            setStep(10, 7, 0.72f, 0.88f, 0.26f);
            setStep(14, 10, 0.84f, 0.86f, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.56f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.92f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.38f);
            break;

        case 3:
            setStep(0, 0, 0.9f, 1.0f);
            setStep(3, 7, 0.62f, 0.76f, 0.82f);
            setStep(5, -5, 0.74f, 0.9f, 0.7f);
            setStep(8, 0, 0.84f, 1.0f);
            setStep(10, 3, 0.56f, 0.68f, 0.24f);
            setStep(13, -2, 0.78f, 0.86f, 0.76f);
            setStep(15, 7, 0.52f, 0.62f, 0.88f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.3f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.5f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.6f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.52f);
            break;

        case 2:
            setStep(0, 0, 0.88f, 1.0f);
            setStep(2, 7, 0.62f, 0.85f, 0.18f);
            setStep(5, 3, 0.78f, 0.95f, 0.72f);
            setStep(7, 10, 0.54f, 0.72f, 0.86f);
            setStep(9, 0, 0.82f, 1.0f);
            setStep(11, -5, 0.66f, 0.82f, 0.78f);
            setStep(14, 7, 0.76f, 0.92f, 0.26f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.58f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.93f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.48f);
            break;

        case 1:
            setStep(2, 0, 0.72f, 1.0f, 0.12f);
            setStep(6, 7, 0.86f, 0.95f, 0.24f);
            setStep(10, 3, 0.76f, 0.9f, 0.18f);
            setStep(14, 10, 0.9f, 0.85f, 0.32f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.94f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.3f);
            break;

        case 0:
        default:
            setStep(0, 0, 0.92f, 1.0f);
            setStep(3, 0, 0.68f, 0.85f);
            setStep(6, -5, 0.78f, 0.9f);
            setStep(8, 0, 0.88f, 1.0f);
            setStep(11, 3, 0.7f, 0.8f);
            setStep(14, -2, 0.74f, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.62f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.96f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.42f);
            break;
    }

    patternSequencer.reset();
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

void NateVSTAudioProcessor::capturePerformanceSnapshot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(performanceSnapshots.size()))
        return;

    performanceSnapshots[static_cast<size_t>(slotIndex)] = createPluginState(false);
}

bool NateVSTAudioProcessor::recallPerformanceSnapshot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(performanceSnapshots.size()))
        return false;

    const auto& snapshot = performanceSnapshots[static_cast<size_t>(slotIndex)];
    if (! snapshot.isValid())
        return false;

    restorePluginState(snapshot.createCopy(), false);
    return true;
}

bool NateVSTAudioProcessor::hasPerformanceSnapshot(int slotIndex) const
{
    return slotIndex >= 0
        && slotIndex < static_cast<int>(performanceSnapshots.size())
        && performanceSnapshots[static_cast<size_t>(slotIndex)].isValid();
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

void NateVSTAudioProcessor::getLowEndMeterLevels(float& subRms, float& lowStereoRisk, float& outputPeak) const noexcept
{
    subRms = lowEndSubRms.load(std::memory_order_relaxed);
    lowStereoRisk = lowEndStereoRisk.load(std::memory_order_relaxed);
    outputPeak = lowEndOutputPeak.load(std::memory_order_relaxed);
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
            Parameters::ID::fxTremoloEnabled,
            Parameters::ID::fxTremoloRate,
            Parameters::ID::fxTremoloDepth,
            Parameters::ID::fxTremoloPan,
            Parameters::ID::fxTremoloShape,
            Parameters::ID::fxTremoloPhase,
            Parameters::ID::fxRingEnabled,
            Parameters::ID::fxRingFrequency,
            Parameters::ID::fxRingDepth,
            Parameters::ID::fxRingMix,
            Parameters::ID::fxRingBias,
            Parameters::ID::fxCombEnabled,
            Parameters::ID::fxCombFrequency,
            Parameters::ID::fxCombFeedback,
            Parameters::ID::fxCombDamping,
            Parameters::ID::fxCombMix,
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
            Parameters::ID::fxEqEnabled,
            Parameters::ID::fxEqLowGain,
            Parameters::ID::fxEqMidGain,
            Parameters::ID::fxEqHighGain,
            Parameters::ID::fxEqTrim,
            Parameters::ID::fxPhaserEnabled,
            Parameters::ID::fxPhaserRate,
            Parameters::ID::fxPhaserDepth,
            Parameters::ID::fxPhaserMix,
            Parameters::ID::fxGuardEnabled,
            Parameters::ID::fxGuardPush,
            Parameters::ID::fxGuardCeiling,
            Parameters::ID::fxFlangerEnabled,
            Parameters::ID::fxFlangerRate,
            Parameters::ID::fxFlangerDepth,
            Parameters::ID::fxFlangerFeedback,
            Parameters::ID::fxFlangerMix
        });

        for (const auto* parameterID : Parameters::ID::fxOrder)
            restoreParameterFromState(state, parameterID);
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
        Parameters::ID::samplePlaybackMode,
        Parameters::ID::sampleTranspose,
        Parameters::ID::samplePitchRamp,
        Parameters::ID::sampleGain,
        Parameters::ID::sampleMix,
        Parameters::ID::sampleStutterEnabled,
        Parameters::ID::sampleStutterRate,
        Parameters::ID::sampleStutterRepeats
    });
}

void NateVSTAudioProcessor::restoreSequencerFromState(const juce::ValueTree& state)
{
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerScale).isValid())
        setParameterPlainValue(Parameters::ID::sequencerScale, 0.0f);

    restoreParameterGroupFromState(state, {
        Parameters::ID::sequencerEnabled,
        Parameters::ID::sequencerRate,
        Parameters::ID::sequencerRoot,
        Parameters::ID::sequencerGate,
        Parameters::ID::sequencerSwing,
        Parameters::ID::sequencerGrooveMode,
        Parameters::ID::sequencerScale,
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
        step.timing = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_timing", 0.0f));
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
        lowEndSubRms.store(0.0f, std::memory_order_relaxed);
        lowEndStereoRisk.store(0.0f, std::memory_order_relaxed);
        lowEndOutputPeak.store(0.0f, std::memory_order_relaxed);
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

    const auto lowAlpha = 1.0f - std::exp((-juce::MathConstants<float>::twoPi * 160.0f) / static_cast<float>(juce::jmax(1.0, meterSampleRate)));
    const auto* leftSamples = buffer.getReadPointer(0);
    const auto* rightSamples = buffer.getReadPointer(numChannels > 1 ? 1 : 0);
    auto monoLowSquares = 0.0;
    auto sideLowSquares = 0.0;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        lowEndStateLeft += lowAlpha * (leftSamples[sampleIndex] - lowEndStateLeft);
        lowEndStateRight += lowAlpha * (rightSamples[sampleIndex] - lowEndStateRight);

        const auto monoLow = (lowEndStateLeft + lowEndStateRight) * 0.5f;
        const auto sideLow = (lowEndStateLeft - lowEndStateRight) * 0.5f;
        monoLowSquares += static_cast<double>(monoLow) * static_cast<double>(monoLow);
        sideLowSquares += static_cast<double>(sideLow) * static_cast<double>(sideLow);
    }

    const auto subRms = static_cast<float>(std::sqrt(monoLowSquares / static_cast<double>(numSamples)));
    const auto sideRms = static_cast<float>(std::sqrt(sideLowSquares / static_cast<double>(numSamples)));
    const auto stereoRisk = sideRms / juce::jmax(0.00001f, subRms + sideRms);
    lowEndSubRms.store(juce::jlimit(0.0f, 2.0f, subRms), std::memory_order_relaxed);
    lowEndStereoRisk.store(juce::jlimit(0.0f, 1.0f, stereoRisk), std::memory_order_relaxed);
    lowEndOutputPeak.store(juce::jlimit(0.0f, 2.0f, juce::jmax(peakLeft, peakRight)), std::memory_order_relaxed);
}

juce::ValueTree NateVSTAudioProcessor::createPluginState()
{
    return createPluginState(true);
}

juce::ValueTree NateVSTAudioProcessor::createPluginState(bool includePerformanceSnapshots)
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
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_timing", step.timing, nullptr);
    }

    if (includePerformanceSnapshots)
        appendPerformanceSnapshotsToState(state);

    return state;
}

void NateVSTAudioProcessor::restorePluginState(const juce::ValueTree& state)
{
    restorePluginState(state, true);
}

void NateVSTAudioProcessor::restorePluginState(const juce::ValueTree& state, bool shouldRestorePerformanceSnapshots)
{
    auto stateForParameters = state.createCopy();
    removePerformanceSnapshotChildren(stateForParameters);

    if (shouldRestorePerformanceSnapshots)
        restorePerformanceSnapshotsFromState(state);

    loadedSamplePath = stateForParameters.getProperty("sample_file").toString();
    parameters.replaceState(stateForParameters);

    if (loadedSamplePath.isNotEmpty())
        samplePlayer.loadFile(juce::File(loadedSamplePath));
    else
        samplePlayer.clear();

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        Sequencer::Step step;
        step.enabled = static_cast<bool>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_enabled", false));
        step.noteOffset = static_cast<int>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_note", 0));
        step.velocity = static_cast<float>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_velocity", 0.8f));
        step.probability = static_cast<float>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_probability", 1.0f));
        step.timing = static_cast<float>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_timing", 0.0f));
        patternSequencer.setStep(stepIndex, step);
    }
}

void NateVSTAudioProcessor::restorePerformanceSnapshotsFromState(const juce::ValueTree& state)
{
    for (auto& snapshot : performanceSnapshots)
        snapshot = {};

    for (auto childIndex = 0; childIndex < state.getNumChildren(); ++childIndex)
    {
        const auto child = state.getChild(childIndex);
        if (! child.hasType(performanceSnapshotType))
            continue;

        const auto slotIndex = static_cast<int>(child.getProperty("slot", -1));
        if (slotIndex < 0 || slotIndex >= static_cast<int>(performanceSnapshots.size()) || child.getNumChildren() <= 0)
            continue;

        auto snapshotState = child.getChild(0).createCopy();
        removePerformanceSnapshotChildren(snapshotState);

        if (snapshotState.isValid() && snapshotState.hasType(parameters.state.getType()))
            performanceSnapshots[static_cast<size_t>(slotIndex)] = snapshotState;
    }
}

void NateVSTAudioProcessor::appendPerformanceSnapshotsToState(juce::ValueTree& state) const
{
    for (size_t slotIndex = 0; slotIndex < performanceSnapshots.size(); ++slotIndex)
    {
        const auto& snapshot = performanceSnapshots[slotIndex];
        if (! snapshot.isValid())
            continue;

        juce::ValueTree child(performanceSnapshotType);
        child.setProperty("slot", static_cast<int>(slotIndex), nullptr);
        child.addChild(snapshot.createCopy(), -1, nullptr);
        state.addChild(child, -1, nullptr);
    }
}

void NateVSTAudioProcessor::removePerformanceSnapshotChildren(juce::ValueTree& state) const
{
    for (auto childIndex = state.getNumChildren(); --childIndex >= 0;)
        if (state.getChild(childIndex).hasType(performanceSnapshotType))
            state.removeChild(childIndex, nullptr);
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
