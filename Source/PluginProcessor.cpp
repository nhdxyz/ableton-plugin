#include "PluginProcessor.h"
#include "PluginEditor.h"

FloorformAudioProcessor::FloorformAudioProcessor()
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

void FloorformAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthEngine.prepare(sampleRate, samplesPerBlock);
    samplePlayer.prepare(sampleRate);
    patternSequencer.prepare(sampleRate);
    effectsRack.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void FloorformAudioProcessor::releaseResources()
{
    effectsRack.reset();
    patternSequencer.reset();
}

bool FloorformAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
}

void FloorformAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    patternSequencer.process(midiMessages, buffer.getNumSamples(), getHostBpm());
    synthEngine.render(buffer, midiMessages);
    samplePlayer.render(buffer, midiMessages);
    effectsRack.process(buffer, outputGain != nullptr ? outputGain->load() : -8.0f);
}

juce::AudioProcessorEditor* FloorformAudioProcessor::createEditor()
{
    return new FloorformAudioProcessorEditor(*this);
}

bool FloorformAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String FloorformAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FloorformAudioProcessor::acceptsMidi() const
{
    return true;
}

bool FloorformAudioProcessor::producesMidi() const
{
    return false;
}

bool FloorformAudioProcessor::isMidiEffect() const
{
    return false;
}

double FloorformAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FloorformAudioProcessor::getNumPrograms()
{
    return 1;
}

int FloorformAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FloorformAudioProcessor::setCurrentProgram(int)
{
}

const juce::String FloorformAudioProcessor::getProgramName(int)
{
    return {};
}

void FloorformAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void FloorformAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = createPluginState().createXml())
        copyXmlToBinary(*xml, destData);
}

void FloorformAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        if (xmlState->hasTagName(parameters.state.getType()))
            restorePluginState(juce::ValueTree::fromXml(*xmlState));
}

Parameters::APVTS& FloorformAudioProcessor::getValueTreeState()
{
    return parameters;
}

void FloorformAudioProcessor::generateRandomPatch()
{
    randomizer.generate();
}

void FloorformAudioProcessor::mutateRandomPatch()
{
    randomizer.mutate();
}

void FloorformAudioProcessor::createRandomVariation()
{
    randomizer.variation();
}

bool FloorformAudioProcessor::loadSampleFile(const juce::File& file)
{
    if (! samplePlayer.loadFile(file))
        return false;

    loadedSamplePath = file.getFullPathName();
    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleEnd, 1.0f);
    return true;
}

void FloorformAudioProcessor::clearSample()
{
    loadedSamplePath.clear();
    samplePlayer.clear();
    setParameterPlainValue(Parameters::ID::sampleEnabled, 0.0f);
}

void FloorformAudioProcessor::randomizeSampleCut()
{
    if (! samplePlayer.hasSample())
        return;

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
}

juce::String FloorformAudioProcessor::getLoadedSampleName() const
{
    return samplePlayer.getLoadedFileName();
}

Sequencer::Step FloorformAudioProcessor::getSequencerStep(int index) const
{
    return patternSequencer.getStep(index);
}

void FloorformAudioProcessor::setSequencerStep(int index, Sequencer::Step step)
{
    patternSequencer.setStep(index, step);
}

void FloorformAudioProcessor::randomizeSequencerPattern()
{
    const auto amount = parameters.getRawParameterValue(Parameters::ID::sequencerRandomAmount);
    patternSequencer.randomize(amount != nullptr ? amount->load() : 0.55f);
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
}

void FloorformAudioProcessor::clearSequencerPattern()
{
    patternSequencer.clear();
}

bool FloorformAudioProcessor::savePreset(const juce::String& presetName)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    const auto directory = getPresetDirectory();
    if (! directory.createDirectory())
        return false;

    auto state = createPluginState();
    state.setProperty("preset_name", trimmedName, nullptr);

    if (auto xml = state.createXml())
        return xml->writeTo(presetFileForName(trimmedName));

    return false;
}

bool FloorformAudioProcessor::loadPreset(const juce::String& presetName)
{
    if (presetName.trim().isEmpty())
        return false;

    const auto file = presetFileForName(presetName);
    if (! file.existsAsFile())
        return false;

    if (auto xml = juce::XmlDocument::parse(file))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(parameters.state.getType()))
        {
            restorePluginState(state);
            return true;
        }
    }

    return false;
}

juce::StringArray FloorformAudioProcessor::getPresetNames() const
{
    juce::StringArray names;
    const auto directory = getPresetDirectory();
    const auto presetFiles = directory.findChildFiles(juce::File::findFiles, false, "*.floorformpreset");

    for (const auto& file : presetFiles)
        names.add(file.getFileNameWithoutExtension());

    names.sort(true);
    return names;
}

juce::File FloorformAudioProcessor::getPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Floorform")
        .getChildFile("Presets");
}

void FloorformAudioProcessor::setParameterPlainValue(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = parameters.getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}

double FloorformAudioProcessor::getHostBpm() const
{
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto bpm = position->getBpm())
                return *bpm;

    return 124.0;
}

juce::ValueTree FloorformAudioProcessor::createPluginState()
{
    auto state = parameters.copyState();
    state.setProperty("floorform_state_version", 1, nullptr);
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

void FloorformAudioProcessor::restorePluginState(const juce::ValueTree& state)
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

juce::File FloorformAudioProcessor::presetFileForName(const juce::String& presetName) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    return getPresetDirectory().getChildFile(legalName).withFileExtension(".floorformpreset");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FloorformAudioProcessor();
}
