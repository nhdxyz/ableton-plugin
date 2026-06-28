#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
const juce::Identifier libraryStateType { "NateVSTLibrary" };
const juce::Identifier favoritesType { "Favorites" };
const juce::Identifier ratingsType { "Ratings" };
const juce::Identifier recentType { "Recent" };
const juce::Identifier presetRefType { "Preset" };
const juce::Identifier performanceSnapshotType { "PerformanceSnapshot" };

juce::StringArray presetCategoryPathSegments(const juce::String& category)
{
    juce::StringArray segments;
    const auto normalisedCategory = category.trim().replaceCharacter('\\', '/');
    segments.addTokens(normalisedCategory, "/", "\"");
    segments.removeEmptyStrings();

    if (segments.isEmpty())
        segments.add("User");

    for (auto index = 0; index < segments.size(); ++index)
    {
        auto legalSegment = juce::File::createLegalFileName(segments[index].trim());
        if (legalSegment.isEmpty())
            legalSegment = "User";

        segments.set(index, legalSegment);
    }

    return segments;
}

juce::String normalisedPresetCategory(const juce::String& category)
{
    return presetCategoryPathSegments(category).joinIntoString("/");
}

juce::String presetTextOrFallback(const juce::String& text, const juce::String& fallback)
{
    const auto trimmed = text.trim();
    return trimmed.isNotEmpty() ? trimmed : fallback;
}

int normalisePresetBpm(int bpm)
{
    return bpm >= 20 && bpm <= 300 ? bpm : 0;
}

float readPresetParameterValue(const juce::ValueTree& state, const char* parameterID, float fallback)
{
    if (const auto child = state.getChildWithProperty("id", parameterID); child.isValid())
        return static_cast<float>(child.getProperty("value", fallback));

    return fallback;
}

juce::String presetMacroSummary(const juce::ValueTree& state, float& intensity)
{
    static constexpr std::array<const char*, 8> labels {
        "Tone",
        "Dirt",
        "Motion",
        "Space",
        "Weight",
        "Bounce",
        "Warp",
        "Throw"
    };

    static constexpr std::array<const char*, 8> ids {
        Parameters::ID::macroTone,
        Parameters::ID::macroDirt,
        Parameters::ID::macroMotion,
        Parameters::ID::macroSpace,
        Parameters::ID::macroWeight,
        Parameters::ID::macroBounce,
        Parameters::ID::macroWarp,
        Parameters::ID::macroThrow
    };

    struct MacroPreview
    {
        juce::String label;
        float value = 0.0f;
    };

    std::array<MacroPreview, 8> previews {};
    intensity = 0.0f;

    for (size_t index = 0; index < ids.size(); ++index)
    {
        const auto value = juce::jlimit(0.0f, 1.0f, readPresetParameterValue(state, ids[index], 0.0f));
        previews[index] = { labels[index], value };
        intensity = juce::jmax(intensity, value);
    }

    std::stable_sort(previews.begin(),
                     previews.end(),
                     [] (const auto& left, const auto& right)
                     {
                         return left.value > right.value;
                     });

    juce::StringArray summary;
    for (const auto& preview : previews)
    {
        if (preview.value < 0.05f)
            continue;

        summary.add(preview.label + " " + juce::String(juce::roundToInt(preview.value * 100.0f)));
        if (summary.size() >= 4)
            break;
    }

    return summary.isEmpty() ? juce::String("Macros flat") : summary.joinIntoString(" ");
}
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
    sequencerChordMemory = parameters.getRawParameterValue(Parameters::ID::sequencerChordMemory);
    sequencerLockDestination = parameters.getRawParameterValue(Parameters::ID::sequencerLockDestination);
    sequencerLockDepth = parameters.getRawParameterValue(Parameters::ID::sequencerLockDepth);
    clearChordMemoryActiveNotes();
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
    hostSyncBpm.store(124.0f, std::memory_order_relaxed);
    hostSyncPpqPosition.store(0.0f, std::memory_order_relaxed);
    hostSyncPositionAvailable.store(false, std::memory_order_relaxed);
    hostSyncPlaying.store(false, std::memory_order_relaxed);
    hostSyncPpqAvailable.store(false, std::memory_order_relaxed);
    lowEndStateLeft = 0.0f;
    lowEndStateRight = 0.0f;
}

void NateVSTAudioProcessor::releaseResources()
{
    effectsRack.reset();
    patternSequencer.reset();
    clearChordMemoryActiveNotes();
    hostSyncPlaying.store(false, std::memory_order_relaxed);
    hostSyncPpqAvailable.store(false, std::memory_order_relaxed);
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
    if (panicRequested.exchange(false, std::memory_order_acq_rel))
    {
        clearChordMemoryActiveNotes();
        samplePlayer.stopAllVoices();

        for (auto channel = 1; channel <= 16; ++channel)
        {
            midiMessages.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
            midiMessages.addEvent(juce::MidiMessage::allSoundOff(channel), 0);
        }
    }

    applyChordMemoryToMidi(midiMessages);
    const auto hostBpm = getHostBpm();
    const auto hostPosition = getHostPosition();
    hostSyncBpm.store(static_cast<float>(hostBpm), std::memory_order_relaxed);
    hostSyncPositionAvailable.store(hostPosition.isAvailable, std::memory_order_relaxed);
    hostSyncPlaying.store(hostPosition.isPlaying, std::memory_order_relaxed);
    hostSyncPpqAvailable.store(hostPosition.ppqPosition.has_value(), std::memory_order_relaxed);
    hostSyncPpqPosition.store(hostPosition.ppqPosition.has_value() ? static_cast<float>(*hostPosition.ppqPosition) : 0.0f,
                              std::memory_order_relaxed);
    patternSequencer.process(midiMessages, buffer.getNumSamples(), hostBpm, hostPosition);
    const auto lockDestination = sequencerLockDestination != nullptr
        ? juce::jlimit(0, 8, static_cast<int>(std::round(sequencerLockDestination->load(std::memory_order_relaxed))))
        : 0;
    const auto lockDepth = sequencerLockDepth != nullptr
        ? juce::jlimit(0.0f, 1.0f, sequencerLockDepth->load(std::memory_order_relaxed))
        : 0.0f;
    const auto lockAmount = lockDestination > 0
        ? juce::jlimit(0.0f, 1.0f, patternSequencer.getActiveStepLock() * lockDepth)
        : 0.0f;
    synthEngine.setSequencerLock(lockDestination, lockAmount);
    effectsRack.setSequencerLock(lockDestination, lockAmount);
    synthEngine.render(buffer, midiMessages, hostBpm);
    samplePlayer.render(buffer,
                        midiMessages,
                        hostBpm,
                        hostPosition.isPlaying ? hostPosition.ppqPosition : std::nullopt);
    effectsRack.process(buffer,
                        outputGain != nullptr ? outputGain->load() : -8.0f,
                        hostBpm,
                        hostPosition.isPlaying ? hostPosition.ppqPosition : std::nullopt);
    updateOutputMeters(buffer);
}

void NateVSTAudioProcessor::applyChordMemoryToMidi(juce::MidiBuffer& midiMessages)
{
    const auto shouldExpandNoteOns = sequencerChordMemory != nullptr
        && sequencerChordMemory->load(std::memory_order_relaxed) > 0.5f
        && getParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f) > 0.5f;

    auto hasActiveChordNotes = false;
    for (const auto& channelCounts : chordMemoryActiveNoteCounts)
    {
        for (const auto noteCount : channelCounts)
        {
            if (noteCount > 0)
            {
                hasActiveChordNotes = true;
                break;
            }
        }

        if (hasActiveChordNotes)
            break;
    }

    if (! shouldExpandNoteOns && ! hasActiveChordNotes)
        return;

    juce::MidiBuffer expandedMidi;

    auto releaseStoredChord = [this, &expandedMidi] (int channelIndex, int inputNote, int midiChannel, int samplePosition)
    {
        auto& activeCount = chordMemoryActiveNoteCounts[static_cast<size_t>(channelIndex)][static_cast<size_t>(inputNote)];
        if (activeCount <= 0)
            return false;

        auto& activeNotes = chordMemoryActiveNotes[static_cast<size_t>(channelIndex)][static_cast<size_t>(inputNote)];
        for (auto noteIndex = 0; noteIndex < activeCount; ++noteIndex)
        {
            const auto noteNumber = activeNotes[static_cast<size_t>(noteIndex)];
            if (noteNumber >= 0)
                expandedMidi.addEvent(juce::MidiMessage::noteOff(midiChannel, noteNumber), samplePosition);
        }

        activeNotes.fill(-1);
        activeCount = 0;
        return true;
    };

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;

        if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            const auto channelIndex = juce::jlimit(0, 15, message.getChannel() - 1);
            for (auto note = 0; note < 128; ++note)
            {
                chordMemoryActiveNotes[static_cast<size_t>(channelIndex)][static_cast<size_t>(note)].fill(-1);
                chordMemoryActiveNoteCounts[static_cast<size_t>(channelIndex)][static_cast<size_t>(note)] = 0;
            }

            expandedMidi.addEvent(message, samplePosition);
            continue;
        }

        if (message.isNoteOn())
        {
            const auto midiChannel = message.getChannel();
            const auto channelIndex = juce::jlimit(0, 15, midiChannel - 1);
            const auto inputNote = juce::jlimit(0, 127, message.getNoteNumber());

            if (! shouldExpandNoteOns)
            {
                expandedMidi.addEvent(message, samplePosition);
                continue;
            }

            releaseStoredChord(channelIndex, inputNote, midiChannel, samplePosition);

            auto noteCount = 0;
            const auto chordNotes = patternSequencer.getChordNotes(inputNote, 0, 0, noteCount);
            auto& activeNotes = chordMemoryActiveNotes[static_cast<size_t>(channelIndex)][static_cast<size_t>(inputNote)];
            auto& activeCount = chordMemoryActiveNoteCounts[static_cast<size_t>(channelIndex)][static_cast<size_t>(inputNote)];
            activeNotes.fill(-1);
            activeCount = juce::jlimit(0, Sequencer::PatternSequencer::maxChordNotes, noteCount);

            for (auto noteIndex = 0; noteIndex < activeCount; ++noteIndex)
            {
                const auto noteNumber = chordNotes[static_cast<size_t>(noteIndex)];
                activeNotes[static_cast<size_t>(noteIndex)] = noteNumber;
                expandedMidi.addEvent(juce::MidiMessage::noteOn(midiChannel, noteNumber, message.getFloatVelocity()), samplePosition);
            }

            continue;
        }

        if (message.isNoteOff())
        {
            const auto midiChannel = message.getChannel();
            const auto channelIndex = juce::jlimit(0, 15, midiChannel - 1);
            const auto inputNote = juce::jlimit(0, 127, message.getNoteNumber());

            if (! releaseStoredChord(channelIndex, inputNote, midiChannel, samplePosition))
                expandedMidi.addEvent(message, samplePosition);

            continue;
        }

        expandedMidi.addEvent(message, samplePosition);
    }

    midiMessages.swapWith(expandedMidi);
}

void NateVSTAudioProcessor::clearChordMemoryActiveNotes()
{
    for (auto channel = 0; channel < 16; ++channel)
    {
        for (auto note = 0; note < 128; ++note)
        {
            chordMemoryActiveNotes[static_cast<size_t>(channel)][static_cast<size_t>(note)].fill(-1);
            chordMemoryActiveNoteCounts[static_cast<size_t>(channel)][static_cast<size_t>(note)] = 0;
        }
    }
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
    generateRandomPatch(0);
}

void NateVSTAudioProcessor::generateRandomPatch(int mutationScopeIndex)
{
    runRandomAction(RandomAction::generate, mutationScopeIndex);
}

void NateVSTAudioProcessor::mutateRandomPatch()
{
    mutateRandomPatch(0);
}

void NateVSTAudioProcessor::mutateRandomPatch(int mutationScopeIndex)
{
    runRandomAction(RandomAction::mutate, mutationScopeIndex);
}

void NateVSTAudioProcessor::wildMutateRandomPatch()
{
    wildMutateRandomPatch(0);
}

void NateVSTAudioProcessor::wildMutateRandomPatch(int mutationScopeIndex)
{
    runRandomAction(RandomAction::wild, mutationScopeIndex);
}

void NateVSTAudioProcessor::createRandomVariation()
{
    createRandomVariation(0);
}

void NateVSTAudioProcessor::createRandomVariation(int mutationScopeIndex)
{
    runRandomAction(RandomAction::variation, mutationScopeIndex);
}

bool NateVSTAudioProcessor::undoRandomization()
{
    if (! hasRandomUndoState || ! randomUndoState.isValid())
        return false;

    randomRedoState = createPluginState();
    randomRedoLabel = randomUndoLabel;
    hasRandomRedoState = true;
    restorePluginState(randomUndoState.createCopy());
    hasRandomUndoState = false;
    randomUndoState = {};
    randomUndoLabel.clear();
    return true;
}

bool NateVSTAudioProcessor::redoRandomization()
{
    if (! hasRandomRedoState || ! randomRedoState.isValid())
        return false;

    randomUndoState = createPluginState();
    randomUndoLabel = randomRedoLabel;
    hasRandomUndoState = true;
    restorePluginState(randomRedoState.createCopy());
    hasRandomRedoState = false;
    randomRedoState = {};
    randomRedoLabel.clear();
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
    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        const auto equalEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        setParameterPlainValue(Parameters::ID::sampleSliceCustom[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceStart[index], equalStart);
        setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], equalEnd);
        setParameterPlainValue(Parameters::ID::sampleSliceReverse[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceTranspose[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceGain[index], -6.0f);
        setParameterPlainValue(Parameters::ID::sampleSlicePan[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceStutter[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceChoke[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceStutterRepeats[index], 3.0f);
    }
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

    auto addModRouteIfAvailable = [this] (int sourceIndex, int destinationIndex, float amount)
    {
        for (size_t slotIndex = 0; slotIndex < Parameters::ID::modMatrixSource.size(); ++slotIndex)
        {
            const auto currentSource = getParameterPlainValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f);
            const auto currentDestination = getParameterPlainValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f);
            const auto currentAmount = getParameterPlainValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);

            if (currentSource > 0.5f && currentDestination > 0.5f && std::abs(currentAmount) > 0.001f)
                continue;

            setParameterPlainValue(Parameters::ID::modMatrixSource[slotIndex], static_cast<float>(sourceIndex));
            setParameterPlainValue(Parameters::ID::modMatrixDestination[slotIndex], static_cast<float>(destinationIndex));
            setParameterPlainValue(Parameters::ID::modMatrixAmount[slotIndex], amount);
            setParameterPlainValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);
            return;
        }
    };

    addModRouteIfAvailable(1, 12, 0.18f);
    addModRouteIfAvailable(10, 14, 0.12f);
    addModRouteIfAvailable(10, 15, 0.10f);

    if (! isRandomLockEnabled(Parameters::ID::randomLockFx))
    {
        setParameterPlainValue(Parameters::ID::fxDelayEnabled, 1.0f);
        setParameterPlainValue(Parameters::ID::fxDelaySync, 1.0f);
        setParameterPlainValue(Parameters::ID::fxDelayRate, 3.0f);
        setParameterPlainValue(Parameters::ID::fxDelayTime, delayTimeDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::fxDelayFeedback, delayFeedbackDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::fxDelayMix, delayMixDistribution(sampleRandomEngine));
    }

    if (! isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        applySequencerPatternPreset(5);

    return true;
}

bool NateVSTAudioProcessor::triggerSampleAudition()
{
    if (! samplePlayer.hasSample())
        return false;

    return samplePlayer.triggerAudition(60, 1.0f, getHostBpm());
}

bool NateVSTAudioProcessor::triggerSampleSliceAudition(int sliceIndex)
{
    if (! samplePlayer.hasSample())
        return false;

    const auto safeSliceIndex = juce::jlimit(0, 7, sliceIndex);
    return samplePlayer.triggerSliceAudition(safeSliceIndex, 60 + safeSliceIndex, 1.0f, getHostBpm());
}

juce::String NateVSTAudioProcessor::getLoadedSampleName() const
{
    return samplePlayer.getLoadedFileName();
}

Sampler::SamplePeakOverview NateVSTAudioProcessor::createSamplePeakOverview(int pointCount) const
{
    return samplePlayer.createPeakOverview(pointCount);
}

Sequencer::Step NateVSTAudioProcessor::getSequencerStep(int index) const
{
    return patternSequencer.getStep(index);
}

void NateVSTAudioProcessor::setSequencerStep(int index, Sequencer::Step step)
{
    captureSequencerUndoState();
    patternSequencer.setStep(index, step);
}

bool NateVSTAudioProcessor::randomizeSequencerPattern()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        return false;

    captureSequencerUndoState();

    const auto amount = parameters.getRawParameterValue(Parameters::ID::sequencerRandomAmount);
    const auto scale = parameters.getRawParameterValue(Parameters::ID::sequencerScale);
    if (scale == nullptr || scale->load() <= 0.5f)
        setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);

    setParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f);

    patternSequencer.randomize(amount != nullptr ? amount->load() : 0.55f);
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);

    std::uniform_real_distribution<float> swingDistribution(0.0f, 0.32f);
    std::uniform_real_distribution<float> accentDistribution(0.2f, 0.75f);
    std::uniform_real_distribution<float> probabilityDistribution(0.72f, 1.0f);
    std::uniform_int_distribution<int> octaveDistribution(-1, 1);
    std::uniform_int_distribution<int> lockDestinationDistribution(1, 6);
    std::bernoulli_distribution selectiveGrooveDistribution(0.55);
    setParameterPlainValue(Parameters::ID::sequencerSwing, swingDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerGrooveMode, selectiveGrooveDistribution(sampleRandomEngine) ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerAccent, accentDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerOctave, static_cast<float>(octaveDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sequencerProbability, probabilityDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sequencerLockDestination, static_cast<float>(lockDestinationDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sequencerLockDepth, juce::jlimit(0.18f, 0.5f, 0.18f + ((amount != nullptr ? amount->load() : 0.55f) * 0.32f)));
    return true;
}

bool NateVSTAudioProcessor::mutateSequencerPattern()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        return false;

    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;
    const auto amountValue = parameters.getRawParameterValue(Parameters::ID::sequencerRandomAmount);
    const auto amount = juce::jlimit(0.0f, 1.0f, amountValue != nullptr ? amountValue->load() : 0.45f);
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_real_distribution<float> smallMove(-1.0f, 1.0f);
    const std::array<int, 9> noteMoves { -7, -5, -3, -2, 2, 3, 5, 7, 12 };

    auto hasEnabledStep = false;
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
        hasEnabledStep = hasEnabledStep || patternSequencer.getStep(stepIndex).enabled;

    if (! hasEnabledStep)
        return randomizeSequencerPattern();

    captureSequencerUndoState();

    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        auto step = patternSequencer.getStep(stepIndex);
        const auto isAnchorStep = stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12;

        if (step.enabled)
        {
            if (chance(sampleRandomEngine) < (0.12f + amount * 0.26f))
                step.noteOffset += noteMoves[static_cast<size_t>(std::uniform_int_distribution<int>(0, static_cast<int>(noteMoves.size()) - 1)(sampleRandomEngine))];

            step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity + smallMove(sampleRandomEngine) * amount * 0.16f);
            step.probability = juce::jlimit(0.35f, 1.0f, step.probability + smallMove(sampleRandomEngine) * amount * 0.18f);
            step.length = juce::jlimit(0.18f, 1.0f, step.length + smallMove(sampleRandomEngine) * amount * 0.16f);
            step.lock = juce::jlimit(0.0f, 1.0f, step.lock + smallMove(sampleRandomEngine) * amount * 0.2f);

            if (! isAnchorStep)
                step.timing = juce::jlimit(0.0f, 1.0f, step.timing + smallMove(sampleRandomEngine) * amount * 0.18f);

            if (! isAnchorStep && chance(sampleRandomEngine) < amount * 0.08f)
                step.enabled = false;
        }
        else if (! isAnchorStep && chance(sampleRandomEngine) < (0.03f + amount * 0.09f))
        {
            step.enabled = true;
            step.noteOffset = noteMoves[static_cast<size_t>(std::uniform_int_distribution<int>(0, static_cast<int>(noteMoves.size()) - 1)(sampleRandomEngine))];
            step.velocity = juce::jlimit(0.25f, 0.82f, 0.42f + chance(sampleRandomEngine) * 0.24f);
            step.probability = juce::jlimit(0.35f, 0.92f, 0.48f + chance(sampleRandomEngine) * 0.3f);
            step.timing = (stepIndex % 2) != 0 ? juce::jlimit(0.0f, 1.0f, 0.28f + chance(sampleRandomEngine) * 0.48f) : 0.0f;
            step.length = juce::jlimit(0.18f, 1.0f, 0.32f + chance(sampleRandomEngine) * 0.55f);
            step.lock = juce::jlimit(0.0f, 1.0f, chance(sampleRandomEngine) * amount);
        }

        patternSequencer.setStep(stepIndex, step);
    }

    std::uniform_real_distribution<float> globalMove(-1.0f, 1.0f);
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sequencerSwing,
                           juce::jlimit(0.0f, 0.65f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f) + globalMove(sampleRandomEngine) * amount * 0.045f));
    setParameterPlainValue(Parameters::ID::sequencerAccent,
                           juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f) + globalMove(sampleRandomEngine) * amount * 0.06f));
    setParameterPlainValue(Parameters::ID::sequencerProbability,
                           juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f) + globalMove(sampleRandomEngine) * amount * 0.04f));
    setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                           juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f) + globalMove(sampleRandomEngine) * amount * 0.05f));

    if (getParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f) > 0.5f)
        setParameterPlainValue(Parameters::ID::sequencerChordStrum,
                               juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f) + globalMove(sampleRandomEngine) * amount * 0.04f));

    patternSequencer.reset();
    return true;
}

bool NateVSTAudioProcessor::undoSequencerEdit()
{
    if (! hasSequencerUndoState || ! sequencerUndoState.isValid())
        return false;

    restoreSequencerFromState(sequencerUndoState);
    hasSequencerUndoState = false;
    patternSequencer.reset();
    return true;
}

void NateVSTAudioProcessor::applySequencerPatternPreset(int presetIndex)
{
    captureSequencerUndoState();

    patternSequencer.clear();
    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
    const auto scaleMode = presetIndex == 1 ? 2.0f
        : presetIndex == 4 || presetIndex == 6 ? 2.0f
        : 4.0f;
    setParameterPlainValue(Parameters::ID::sequencerScale, scaleMode);
    setParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerLockDestination, 0.0f);
    setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f);

    auto setStep = [this] (int index, int noteOffset, float velocity, float probability, float timing = 0.0f, float length = 1.0f, float lock = 0.0f)
    {
        Sequencer::Step step;
        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = velocity;
        step.probability = probability;
        step.timing = timing;
        step.length = length;
        step.lock = lock;
        patternSequencer.setStep(index, step);
    };

    switch (presetIndex)
    {
        case 10:
            setStep(0, 0, 0.94f, 1.0f);
            setStep(2, 0, 0.52f, 0.72f, 0.08f, 1.0f, 0.24f);
            setStep(4, 0, 0.86f, 1.0f);
            setStep(6, 7, 0.5f, 0.68f, 0.08f, 1.0f, 0.38f);
            setStep(8, 0, 0.9f, 1.0f);
            setStep(10, -5, 0.54f, 0.74f, 0.08f, 1.0f, 0.28f);
            setStep(12, 0, 0.88f, 1.0f);
            setStep(14, 3, 0.5f, 0.68f, 0.08f, 1.0f, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.38f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.08f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.72f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.96f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.28f);
            break;

        case 9:
            setStep(0, 0, 0.82f, 0.95f);
            setStep(5, 3, 0.58f, 0.72f, 0.5f, 1.0f, 0.3f);
            setStep(10, 7, 0.74f, 0.82f, 0.24f, 1.0f, 0.22f);
            setStep(15, 10, 0.5f, 0.58f, 0.72f, 1.0f, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.16f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.3f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.82f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.56f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 7.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.32f);
            break;

        case 8:
            setStep(0, 0, 0.92f, 1.0f);
            setStep(3, 0, 0.64f, 0.78f, 0.56f, 1.0f, 0.22f);
            setStep(5, -5, 0.78f, 0.88f, 0.18f, 1.0f, 0.36f);
            setStep(7, 0, 0.58f, 0.66f, 0.32f, 1.0f, 0.18f);
            setStep(10, 3, 0.74f, 0.84f, 0.18f, 1.0f, 0.34f);
            setStep(12, 0, 0.88f, 0.96f);
            setStep(14, -2, 0.68f, 0.78f, 0.24f, 1.0f, 0.28f);
            setStep(15, 0, 0.56f, 0.62f, 0.48f, 1.0f, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.64f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.94f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.44f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.26f);
            break;

        case 7:
            setStep(2, 0, 0.78f, 1.0f, 0.12f, 1.0f, 0.26f);
            setStep(6, 3, 0.86f, 0.94f, 0.2f, 1.0f, 0.42f);
            setStep(10, 7, 0.74f, 0.9f, 0.14f, 1.0f, 0.3f);
            setStep(14, 10, 0.84f, 0.86f, 0.24f, 1.0f, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.32f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.5f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 6.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.24f);
            break;

        case 6:
            setStep(3, 0, 0.76f, 1.0f, 0.82f, 1.0f, 0.42f);
            setStep(7, 7, 0.86f, 0.92f, 1.0f, 1.0f, 0.55f);
            setStep(10, 3, 0.66f, 0.72f, 0.34f, 1.0f, 0.24f);
            setStep(15, 10, 0.82f, 0.86f, 0.9f, 1.0f, 0.5f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.38f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.32f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.34f);
            break;

        case 5:
            setStep(1, 0, 0.72f, 0.9f, 0.68f, 1.0f, 0.32f);
            setStep(4, 7, 0.86f, 1.0f, 0.0f);
            setStep(6, -5, 0.58f, 0.68f, 0.28f, 1.0f, 0.18f);
            setStep(9, 3, 0.78f, 0.86f, 0.74f, 1.0f, 0.42f);
            setStep(12, 0, 0.7f, 0.78f, 0.0f);
            setStep(14, 12, 0.82f, 0.82f, 0.36f, 1.0f, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.84f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.62f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.32f);
            break;

        case 4:
            setStep(2, 0, 0.76f, 1.0f, 0.22f, 1.0f, 0.18f);
            setStep(6, 3, 0.88f, 0.95f, 0.34f, 1.0f, 0.36f);
            setStep(10, 7, 0.72f, 0.88f, 0.26f, 1.0f, 0.22f);
            setStep(14, 10, 0.84f, 0.86f, 0.42f, 1.0f, 0.4f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.22f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.56f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.92f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.38f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 6.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.22f);
            break;

        case 3:
            setStep(0, 0, 0.9f, 1.0f);
            setStep(3, 7, 0.62f, 0.76f, 0.82f, 1.0f, 0.5f);
            setStep(5, -5, 0.74f, 0.9f, 0.7f, 1.0f, 0.36f);
            setStep(8, 0, 0.84f, 1.0f);
            setStep(10, 3, 0.56f, 0.68f, 0.24f, 1.0f, 0.22f);
            setStep(13, -2, 0.78f, 0.86f, 0.76f, 1.0f, 0.42f);
            setStep(15, 7, 0.52f, 0.62f, 0.88f, 1.0f, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.3f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.5f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.6f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.36f);
            break;

        case 2:
            setStep(0, 0, 0.88f, 1.0f);
            setStep(2, 7, 0.62f, 0.85f, 0.18f, 1.0f, 0.16f);
            setStep(5, 3, 0.78f, 0.95f, 0.72f, 1.0f, 0.44f);
            setStep(7, 10, 0.54f, 0.72f, 0.86f, 1.0f, 0.32f);
            setStep(9, 0, 0.82f, 1.0f);
            setStep(11, -5, 0.66f, 0.82f, 0.78f, 1.0f, 0.38f);
            setStep(14, 7, 0.76f, 0.92f, 0.26f, 1.0f, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.58f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.93f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.38f);
            break;

        case 1:
            setStep(2, 0, 0.72f, 1.0f, 0.12f, 1.0f, 0.22f);
            setStep(6, 7, 0.86f, 0.95f, 0.24f, 1.0f, 0.34f);
            setStep(10, 3, 0.76f, 0.9f, 0.18f, 1.0f, 0.28f);
            setStep(14, 10, 0.9f, 0.85f, 0.32f, 1.0f, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.12f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.94f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.3f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.26f);
            break;

        case 0:
        default:
            setStep(0, 0, 0.92f, 1.0f);
            setStep(3, 0, 0.68f, 0.85f, 0.0f, 1.0f, 0.2f);
            setStep(6, -5, 0.78f, 0.9f, 0.0f, 1.0f, 0.32f);
            setStep(8, 0, 0.88f, 1.0f);
            setStep(11, 3, 0.7f, 0.8f, 0.0f, 1.0f, 0.24f);
            setStep(14, -2, 0.74f, 0.9f, 0.0f, 1.0f, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.52f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.62f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.96f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.42f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.28f);
            break;
    }

    patternSequencer.reset();
}

bool NateVSTAudioProcessor::applySequencerGrooveTransform(int transformIndex)
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSequencer))
        return false;

    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;
    auto hasEnabledStep = false;
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
        hasEnabledStep = hasEnabledStep || patternSequencer.getStep(stepIndex).enabled;

    if (! hasEnabledStep)
        return false;

    captureSequencerUndoState();

    const auto isAnchorStep = [] (int stepIndex)
    {
        return (stepIndex % 4) == 0;
    };
    const auto isOffbeatStep = [] (int stepIndex)
    {
        return (stepIndex % 2) != 0;
    };
    const auto isLateStabStep = [] (int stepIndex)
    {
        return stepIndex == 2 || stepIndex == 6 || stepIndex == 10 || stepIndex == 14;
    };
    const auto isVocalPushStep = [] (int stepIndex)
    {
        return stepIndex == 3 || stepIndex == 7 || stepIndex == 11 || stepIndex == 15;
    };

    std::uniform_real_distribution<float> humanize(-1.0f, 1.0f);

    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        auto step = patternSequencer.getStep(stepIndex);
        if (! step.enabled)
            continue;

        switch (transformIndex)
        {
            case 1: // Straight Anchors
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = 1.0f;
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.08f);
                    step.length = juce::jmax(step.length, 0.72f);
                    step.lock = 0.0f;
                }
                break;

            case 2: // Swung Ghosts
                if (! isAnchorStep(stepIndex))
                {
                    if (isOffbeatStep(stepIndex))
                        step.timing = juce::jmax(step.timing, 0.58f);
                    else
                        step.timing = juce::jmax(step.timing, 0.24f);

                    if (step.velocity < 0.72f || step.probability < 0.9f)
                    {
                        step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity * 0.88f);
                        step.probability = juce::jlimit(0.35f, 1.0f, step.probability * 0.86f);
                        step.length = juce::jlimit(0.18f, 1.0f, step.length * 0.82f);
                        step.lock = juce::jmax(step.lock, isOffbeatStep(stepIndex) ? 0.28f : 0.16f);
                    }
                }
                break;

            case 3: // Late Stabs
                if (isLateStabStep(stepIndex) || (! isAnchorStep(stepIndex) && step.velocity >= 0.72f))
                {
                    step.timing = juce::jmax(step.timing, isLateStabStep(stepIndex) ? 0.64f : 0.42f);
                    step.probability = juce::jlimit(0.5f, 1.0f, step.probability + 0.06f);
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.04f);
                    step.length = juce::jlimit(0.28f, 1.0f, juce::jmin(step.length, 0.72f));
                    step.lock = juce::jmax(step.lock, isLateStabStep(stepIndex) ? 0.46f : 0.28f);
                }
                break;

            case 4: // Vocal Push
                if (isVocalPushStep(stepIndex))
                {
                    step.timing = juce::jmax(step.timing, 0.78f);
                    step.probability = juce::jlimit(0.35f, 0.92f, step.probability * 0.9f);
                    step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.58f));
                    step.lock = juce::jmax(step.lock, 0.52f);
                }
                else if (! isAnchorStep(stepIndex))
                {
                    step.timing = juce::jmax(step.timing, isOffbeatStep(stepIndex) ? 0.42f : 0.16f);
                    step.length = juce::jlimit(0.18f, 1.0f, step.length * 0.9f);
                    step.lock = juce::jmax(step.lock, 0.18f);
                }
                break;

            case 5: // Humanize Light
                if (isAnchorStep(stepIndex))
                {
                    step.timing = juce::jlimit(0.0f, 0.12f, step.timing + humanize(sampleRandomEngine) * 0.035f);
                }
                else
                {
                    step.timing = juce::jlimit(0.0f, 1.0f, step.timing + humanize(sampleRandomEngine) * 0.09f);
                    step.probability = juce::jlimit(0.35f, 1.0f, step.probability + humanize(sampleRandomEngine) * 0.045f);
                    step.length = juce::jlimit(0.18f, 1.0f, step.length + humanize(sampleRandomEngine) * 0.065f);
                }

                step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity + humanize(sampleRandomEngine) * 0.055f);
                break;

            case 0: // Tighten
            default:
                step.timing = isAnchorStep(stepIndex) ? 0.0f : step.timing * 0.32f;
                step.probability = juce::jlimit(0.55f, 1.0f, step.probability + 0.08f);
                step.length = juce::jlimit(0.2f, 1.0f, juce::jmin(step.length, isAnchorStep(stepIndex) ? 0.86f : 0.64f));
                step.lock = juce::jlimit(0.0f, 1.0f, step.lock * 0.72f);
                break;
        }

        patternSequencer.setStep(stepIndex, step);
    }

    switch (transformIndex)
    {
        case 1:
            setParameterPlainValue(Parameters::ID::sequencerSwing,
                                   juce::jlimit(0.0f, 0.65f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f) * 0.82f));
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f) * 0.72f));
            break;

        case 2:
            setParameterPlainValue(Parameters::ID::sequencerSwing,
                                   juce::jlimit(0.0f, 0.65f, juce::jmax(0.24f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f))));
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmin(0.94f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.3f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 3:
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.38f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            if (getParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f) > 0.5f)
                setParameterPlainValue(Parameters::ID::sequencerChordStrum,
                                       juce::jlimit(0.0f, 1.0f, juce::jmax(0.14f, getParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.28f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 4:
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.26f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.34f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 5:
            setParameterPlainValue(Parameters::ID::sequencerSwing,
                                   juce::jlimit(0.0f, 0.65f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f) + humanize(sampleRandomEngine) * 0.025f));
            break;

        case 0:
        default:
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.42f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f) * 0.9f)));
            setParameterPlainValue(Parameters::ID::sequencerSwing,
                                   juce::jlimit(0.0f, 0.65f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f) * 0.65f));
            break;
    }

    setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
    patternSequencer.reset();
    return true;
}

void NateVSTAudioProcessor::copySequencerFirstHalfToSecondHalf()
{
    captureSequencerUndoState();

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps / 2; ++stepIndex)
        patternSequencer.setStep(stepIndex + (Sequencer::PatternSequencer::numSteps / 2), patternSequencer.getStep(stepIndex));
}

void NateVSTAudioProcessor::rotateSequencerPattern(int stepOffset)
{
    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;
    auto offset = stepOffset % stepCount;
    if (offset < 0)
        offset += stepCount;

    if (offset == 0)
        return;

    captureSequencerUndoState();

    std::array<Sequencer::Step, stepCount> steps {};
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
        steps[static_cast<size_t>(stepIndex)] = patternSequencer.getStep(stepIndex);

    for (auto destination = 0; destination < stepCount; ++destination)
    {
        const auto source = (destination + stepCount - offset) % stepCount;
        patternSequencer.setStep(destination, steps[static_cast<size_t>(source)]);
    }
}

bool NateVSTAudioProcessor::exportSequencerMidiFile(const juce::File& destination) const
{
    if (destination == juce::File{})
        return false;

    auto outputFile = destination.hasFileExtension(".mid;.midi") ? destination : destination.withFileExtension(".mid");
    if (! outputFile.getParentDirectory().createDirectory())
        return false;

    constexpr auto ticksPerQuarterNote = 960;
    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;
    const auto rateIndex = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerRate, 1.0f));
    const auto stepTicks = rateIndex == 0 ? ticksPerQuarterNote / 2
        : rateIndex == 2 ? ticksPerQuarterNote / 8
        : ticksPerQuarterNote / 4;
    const auto root = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerRoot, 36.0f));
    const auto octaveOffset = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f)) * 12;
    const auto gate = juce::jlimit(0.05f, 0.95f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f));
    const auto swing = juce::jlimit(0.0f, 0.65f, getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f));
    const auto grooveMode = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerGrooveMode, 0.0f));
    const auto accent = juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f));

    auto stepDelayTicks = [this, stepTicks, swing, grooveMode] (int stepIndex)
    {
        const auto safeIndex = juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1, stepIndex);
        const auto step = patternSequencer.getStep(safeIndex);
        const auto maxDelay = static_cast<float>(stepTicks) * swing * 0.5f;
        const auto isAnchorStep = safeIndex == 0 || safeIndex == 4 || safeIndex == 8 || safeIndex == 12;
        const auto isOffbeatStep = (safeIndex % 2) != 0;

        auto weight = 0.0f;
        switch (grooveMode)
        {
            case 1:
                weight = step.timing;
                break;

            case 2:
                if (! isAnchorStep)
                    weight = step.timing > 0.0f ? step.timing : (isOffbeatStep ? 0.62f : 0.18f);
                break;

            case 3:
                weight = isAnchorStep ? 0.0f : step.timing * 0.65f;
                break;

            case 0:
            default:
                weight = isOffbeatStep ? 1.0f : 0.0f;
                break;
        }

        return juce::roundToInt(maxDelay * juce::jlimit(0.0f, 1.0f, weight));
    };

    juce::MidiMessageSequence sequence;
    auto exportedNotes = 0;

    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        const auto step = patternSequencer.getStep(stepIndex);
        if (! step.enabled)
            continue;

        const auto delayTicks = stepDelayTicks(stepIndex);
        const auto nextDelayTicks = stepDelayTicks((stepIndex + 1) % stepCount);
        const auto durationTicks = juce::jmax(1, stepTicks + nextDelayTicks - delayTicks);
        const auto stepGate = juce::jlimit(0.05f, 1.0f, gate * juce::jlimit(0.1f, 1.0f, step.length));
        const auto gateTicks = juce::jlimit(1, juce::jmax(1, durationTicks - 1),
                                           juce::roundToInt(static_cast<float>(durationTicks) * stepGate));
        const auto isAnchorStep = stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12;
        const auto velocity = isAnchorStep
            ? juce::jlimit(0.0f, 1.0f, step.velocity + ((1.0f - step.velocity) * accent))
            : juce::jlimit(0.0f, 1.0f, step.velocity * (1.0f - (accent * 0.12f)));
        const auto startTicks = (stepIndex * stepTicks) + delayTicks;
        auto noteCount = 0;
        const auto notes = patternSequencer.getChordNotes(root, octaveOffset, step.noteOffset, noteCount);

        for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
        {
            const auto noteNumber = notes[static_cast<size_t>(noteIndex)];
            const auto noteVelocity = patternSequencer.getChordNoteVelocity(velocity, noteIndex);
            const auto noteStartTicks = startTicks + patternSequencer.getChordStrumOffset(stepTicks, noteIndex, noteCount);
            const auto noteEndTicks = juce::jmax(noteStartTicks + 1, startTicks + gateTicks);
            sequence.addEvent(juce::MidiMessage::noteOn(1, noteNumber, noteVelocity), static_cast<double>(noteStartTicks));
            sequence.addEvent(juce::MidiMessage::noteOff(1, noteNumber), static_cast<double>(noteEndTicks));
            ++exportedNotes;
        }
    }

    if (exportedNotes == 0)
        return false;

    sequence.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);
    midiFile.addTrack(sequence);

    auto output = outputFile.createOutputStream();
    if (output == nullptr || output->failedToOpen())
        return false;

    output->setPosition(0);
    output->truncate();
    return midiFile.writeTo(*output);
}

void NateVSTAudioProcessor::clearSequencerPattern()
{
    captureSequencerUndoState();
    patternSequencer.clear();
}

bool NateVSTAudioProcessor::savePreset(const juce::String& presetName)
{
    return savePreset(presetName, "User");
}

bool NateVSTAudioProcessor::savePreset(const juce::String& presetName, const juce::String& category)
{
    PresetSaveOptions options;
    options.category = category;
    return savePreset(presetName, options);
}

bool NateVSTAudioProcessor::savePreset(const juce::String& presetName, const PresetSaveOptions& options)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    const auto storedCategory = normalisedPresetCategory(presetTextOrFallback(options.category, "User"));
    const auto storedAuthor = presetTextOrFallback(options.author, "User");
    const auto storedPack = presetTextOrFallback(options.pack, "User Pack");
    const auto storedKey = presetTextOrFallback(options.key, "Any Key");
    const auto storedBpm = normalisePresetBpm(options.bpm);
    const auto directory = presetDirectoryForCategory(storedCategory);
    if (! directory.createDirectory())
        return false;

    auto state = createPluginState();
    state.setProperty("preset_name", trimmedName, nullptr);
    state.setProperty("preset_category", storedCategory, nullptr);
    state.setProperty("preset_author", storedAuthor, nullptr);
    state.setProperty("preset_source", "User", nullptr);
    state.setProperty("preset_pack", storedPack, nullptr);
    state.setProperty("preset_key", storedKey, nullptr);
    state.setProperty("preset_bpm", storedBpm, nullptr);
    state.setProperty("preset_tags", storedPack.equalsIgnoreCase(storedCategory) ? storedCategory : storedCategory + ", " + storedPack, nullptr);
    state.setProperty("preset_folder", presetCategoryPathSegments(storedCategory).joinIntoString("/"), nullptr);

    if (auto xml = state.createXml())
    {
        const auto destination = presetFileForName(trimmedName, storedCategory);
        auto legalName = juce::File::createLegalFileName(trimmedName);
        if (legalName.isEmpty())
            legalName = "Untitled";

        juce::TemporaryFile temporaryFile(destination);
        if (! xml->writeTo(temporaryFile.getFile()))
            return false;

        if (! temporaryFile.getFile().existsAsFile() || temporaryFile.getFile().getSize() == 0)
            return false;

        if (! temporaryFile.overwriteTargetFileWithTemporary())
            return false;

        const auto existingFiles = getPresetDirectory().findChildFiles(juce::File::findFiles, true, "*.natevstpreset");
        for (const auto& file : existingFiles)
            if (file.getFileNameWithoutExtension() == legalName
                && file.getFullPathName() != destination.getFullPathName())
            {
                file.deleteFile();
            }

        return true;
    }

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
    const auto libraryState = loadLibraryState();
    const auto favorites = getLibraryStateNames(favoritesType);
    const auto ratings = libraryState.getChildWithName(ratingsType);

    auto ratingForPreset = [&ratings] (const juce::String& presetName)
    {
        if (! ratings.isValid())
            return 0;

        for (auto index = 0; index < ratings.getNumChildren(); ++index)
        {
            const auto rating = ratings.getChild(index);
            if (rating.getProperty("name").toString() == presetName)
                return juce::jlimit(0, 5, static_cast<int>(rating.getProperty("rating", 0)));
        }

        return 0;
    };

    auto collectFromDirectory = [&presets, &seenNames, &favorites, &ratingForPreset] (const juce::File& directory, bool isFactory)
    {
        const auto presetFiles = directory.findChildFiles(juce::File::findFiles, true, "*.natevstpreset");

        for (const auto& file : presetFiles)
        {
            const auto name = file.getFileNameWithoutExtension();
            if (seenNames.contains(name))
                continue;

            auto category = isFactory ? juce::String("Factory") : juce::String("User");
            auto source = isFactory ? juce::String("Factory") : juce::String("User");
            auto author = isFactory ? juce::String("Nate") : juce::String("User");
            auto pack = isFactory ? juce::String("Factory Pack") : juce::String("User Pack");
            auto key = juce::String("Any Key");
            auto macroSummary = juce::String("Macros flat");
            auto bpm = 0;
            auto macroIntensity = 0.0f;
            juce::String tags;
            juce::String folder;
            const auto parentPath = file.getParentDirectory().getFullPathName();
            const auto rootPath = directory.getFullPathName();
            if (parentPath != rootPath && parentPath.startsWith(rootPath))
            {
                folder = parentPath.substring(rootPath.length()).replaceCharacter('\\', '/');
                while (folder.startsWithChar('/'))
                    folder = folder.substring(1);
            }

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

                    const auto storedAuthor = state.getProperty("preset_author").toString().trim();
                    if (storedAuthor.isNotEmpty())
                        author = storedAuthor;

                    const auto storedPack = state.getProperty("preset_pack").toString().trim();
                    if (storedPack.isNotEmpty())
                        pack = storedPack;

                    const auto storedKey = state.getProperty("preset_key").toString().trim();
                    if (storedKey.isNotEmpty())
                        key = storedKey;

                    bpm = normalisePresetBpm(static_cast<int>(state.getProperty("preset_bpm", 0)));

                    const auto storedTags = state.getProperty("preset_tags").toString().trim();
                    if (storedTags.isNotEmpty())
                        tags = storedTags;

                    macroSummary = presetMacroSummary(state, macroIntensity);

                    const auto storedFolder = state.getProperty("preset_folder").toString().trim();
                    if (storedFolder.isNotEmpty())
                        folder = storedFolder.replaceCharacter('\\', '/');
                }
            }

            if (folder.isNotEmpty() && category == "User")
                category = folder.fromLastOccurrenceOf("/", false, true);

            presets.push_back({ name,
                                category,
                                source,
                                tags,
                                folder,
                                author,
                                pack,
                                key,
                                macroSummary,
                                bpm,
                                ratingForPreset(name),
                                macroIntensity,
                                file.getLastModificationTime().toMilliseconds(),
                                isFactory,
                                favorites.contains(name) });
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

int NateVSTAudioProcessor::getPresetRating(const juce::String& presetName) const
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return 0;

    const auto state = loadLibraryState();
    const auto ratings = state.getChildWithName(ratingsType);
    if (! ratings.isValid())
        return 0;

    for (auto index = 0; index < ratings.getNumChildren(); ++index)
    {
        const auto rating = ratings.getChild(index);
        if (rating.getProperty("name").toString() == trimmedName)
            return juce::jlimit(0, 5, static_cast<int>(rating.getProperty("rating", 0)));
    }

    return 0;
}

bool NateVSTAudioProcessor::setPresetRating(const juce::String& presetName, int rating)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    auto state = loadLibraryState();
    auto ratings = state.getChildWithName(ratingsType);
    if (! ratings.isValid())
    {
        ratings = juce::ValueTree(ratingsType);
        state.addChild(ratings, -1, nullptr);
    }

    for (auto index = ratings.getNumChildren(); --index >= 0;)
        if (ratings.getChild(index).getProperty("name").toString() == trimmedName)
            ratings.removeChild(index, nullptr);

    rating = juce::jlimit(0, 5, rating);
    if (rating > 0)
    {
        juce::ValueTree ratingItem(presetRefType);
        ratingItem.setProperty("name", trimmedName, nullptr);
        ratingItem.setProperty("rating", rating, nullptr);
        ratings.addChild(ratingItem, -1, nullptr);
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

void NateVSTAudioProcessor::panicAllNotesOff()
{
    midiKeyboardState.allNotesOff(0);
    panicRequested.store(true, std::memory_order_release);
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

NateVSTAudioProcessor::HostSyncStatus NateVSTAudioProcessor::getHostSyncStatus() const noexcept
{
    HostSyncStatus status;
    status.bpm = hostSyncBpm.load(std::memory_order_relaxed);
    status.positionAvailable = hostSyncPositionAvailable.load(std::memory_order_relaxed);
    status.playing = hostSyncPlaying.load(std::memory_order_relaxed);
    status.ppqAvailable = hostSyncPpqAvailable.load(std::memory_order_relaxed);
    status.ppqPosition = hostSyncPpqPosition.load(std::memory_order_relaxed);
    return status;
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

juce::String NateVSTAudioProcessor::getRandomHistorySummary() const
{
    juce::StringArray history;

    if (hasRandomUndoState && randomUndoState.isValid())
        history.add("Undo: " + (randomUndoLabel.isNotEmpty() ? randomUndoLabel : "Random"));

    if (hasRandomRedoState && randomRedoState.isValid())
        history.add("Redo: " + (randomRedoLabel.isNotEmpty() ? randomRedoLabel : "Random"));

    return history.joinIntoString(" / ");
}

void NateVSTAudioProcessor::runRandomAction(RandomAction action, int mutationScopeIndex)
{
    const auto snapshot = createPluginState();
    const auto mutationScope = randomMutationScopeFromIndex(mutationScopeIndex);
    randomUndoState = snapshot.createCopy();
    randomUndoLabel = randomActionLabel(action);
    hasRandomUndoState = true;
    randomRedoState = {};
    randomRedoLabel.clear();
    hasRandomRedoState = false;

    switch (action)
    {
        case RandomAction::generate:
            randomizer.generate();
            break;
        case RandomAction::mutate:
            randomizer.mutate();
            break;
        case RandomAction::wild:
            randomizer.wildMutate();
            break;
        case RandomAction::variation:
            randomizer.variation();
            break;
    }

    if (mutationScope == RandomMutationScope::sample)
    {
        if (action == RandomAction::wild)
            randomizeUkgVocalChop();
        else
            randomizeSampleCut();
    }

    restoreSectionsOutsideMutationScope(snapshot, mutationScope);
    restoreLockedSectionsFromState(snapshot);
}

juce::String NateVSTAudioProcessor::randomActionLabel(RandomAction action)
{
    switch (action)
    {
        case RandomAction::generate: return "Generate";
        case RandomAction::mutate: return "Mutate";
        case RandomAction::wild: return "Wild";
        case RandomAction::variation: return "Variation";
    }

    return "Random";
}

NateVSTAudioProcessor::RandomMutationScope NateVSTAudioProcessor::randomMutationScopeFromIndex(int mutationScopeIndex)
{
    switch (mutationScopeIndex)
    {
        case 1: return RandomMutationScope::source;
        case 2: return RandomMutationScope::envelope;
        case 3: return RandomMutationScope::filter;
        case 4: return RandomMutationScope::sample;
        case 5: return RandomMutationScope::effects;
        case 6: return RandomMutationScope::sequencer;
        case 7: return RandomMutationScope::macros;
        default: return RandomMutationScope::all;
    }
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

void NateVSTAudioProcessor::restoreSectionsOutsideMutationScope(const juce::ValueTree& state, RandomMutationScope mutationScope)
{
    if (mutationScope == RandomMutationScope::all)
        return;

    for (auto scopeIndex = 1; scopeIndex <= 7; ++scopeIndex)
    {
        const auto candidate = randomMutationScopeFromIndex(scopeIndex);
        if (candidate != mutationScope)
            restoreMutationScopeFromState(state, candidate);
    }

    restoreModulationFromState(state);
    restoreOutputFromState(state);
}

void NateVSTAudioProcessor::restoreMutationScopeFromState(const juce::ValueTree& state, RandomMutationScope mutationScope)
{
    switch (mutationScope)
    {
        case RandomMutationScope::source:
            restoreParameterGroupFromState(state, {
                Parameters::ID::oscOctave,
                Parameters::ID::oscTune,
                Parameters::ID::oscWave,
                Parameters::ID::osc1Level,
                Parameters::ID::osc2Octave,
                Parameters::ID::osc2Tune,
                Parameters::ID::osc2Wave,
                Parameters::ID::osc2Level,
                Parameters::ID::subLevel,
                Parameters::ID::noiseLevel,
                Parameters::ID::oscWarp,
                Parameters::ID::oscWavetablePosition,
                Parameters::ID::osc2WavetablePosition,
                Parameters::ID::monoMode,
                Parameters::ID::glideTime,
                Parameters::ID::unisonVoices,
                Parameters::ID::unisonDetune,
                Parameters::ID::unisonBlend,
                Parameters::ID::unisonSpread
            });
            break;

        case RandomMutationScope::envelope:
            restoreParameterGroupFromState(state, {
                Parameters::ID::ampAttack,
                Parameters::ID::ampDecay,
                Parameters::ID::ampSustain,
                Parameters::ID::ampRelease
            });
            break;

        case RandomMutationScope::filter:
            restoreParameterGroupFromState(state, {
                Parameters::ID::filterCutoff,
                Parameters::ID::filterResonance,
                Parameters::ID::filterEnvAmount,
                Parameters::ID::filterMode,
                Parameters::ID::filterCharacter,
                Parameters::ID::filterSlope,
                Parameters::ID::driveAmount
            });
            break;

        case RandomMutationScope::sample:
            restoreSampleFromState(state);
            break;

        case RandomMutationScope::effects:
            restoreParameterGroupFromState(state, {
                Parameters::ID::fxDistortionEnabled,
                Parameters::ID::fxDistortionAmount,
                Parameters::ID::fxBitcrushEnabled,
                Parameters::ID::fxBitcrushBits,
                Parameters::ID::fxBitcrushDownsample,
                Parameters::ID::fxBitcrushMix,
                Parameters::ID::fxPumpEnabled,
                Parameters::ID::fxPumpRate,
                Parameters::ID::fxPumpCurve,
                Parameters::ID::fxPumpCustomCurve[0],
                Parameters::ID::fxPumpCustomCurve[1],
                Parameters::ID::fxPumpCustomCurve[2],
                Parameters::ID::fxPumpCustomCurve[3],
                Parameters::ID::fxPumpCustomCurve[4],
                Parameters::ID::fxPumpCustomCurve[5],
                Parameters::ID::fxPumpCustomCurve[6],
                Parameters::ID::fxPumpCustomCurve[7],
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
                Parameters::ID::fxDelaySync,
                Parameters::ID::fxDelayRate,
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

            break;

        case RandomMutationScope::sequencer:
            restoreSequencerFromState(state);
            break;

        case RandomMutationScope::macros:
            restoreParameterGroupFromState(state, {
                Parameters::ID::macroTone,
                Parameters::ID::macroDirt,
                Parameters::ID::macroMotion,
                Parameters::ID::macroSpace,
                Parameters::ID::macroWeight,
                Parameters::ID::macroBounce,
                Parameters::ID::macroWarp,
                Parameters::ID::macroThrow
            });
            break;

        case RandomMutationScope::all:
            break;
    }
}

void NateVSTAudioProcessor::restoreOutputFromState(const juce::ValueTree& state)
{
    restoreParameterFromState(state, Parameters::ID::outputGain);
}

void NateVSTAudioProcessor::restoreModulationFromState(const juce::ValueTree& state)
{
    restoreParameterGroupFromState(state, {
        Parameters::ID::lfo1Rate,
        Parameters::ID::lfo1Sync,
        Parameters::ID::lfo1SyncRate,
        Parameters::ID::lfo1Shape,
        Parameters::ID::lfo1Depth,
        Parameters::ID::lfo1Phase,
        Parameters::ID::lfo1Retrigger,
        Parameters::ID::lfo2Rate,
        Parameters::ID::lfo2Sync,
        Parameters::ID::lfo2SyncRate,
        Parameters::ID::lfo2Shape,
        Parameters::ID::lfo2Depth,
        Parameters::ID::lfo2Phase,
        Parameters::ID::lfo2Retrigger,
        Parameters::ID::modEnv1Attack,
        Parameters::ID::modEnv1Decay,
        Parameters::ID::modEnv1Sustain,
        Parameters::ID::modEnv1Release,
        Parameters::ID::modEnv1Depth
    });

    for (const auto* parameterID : Parameters::ID::lfo1Curve)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixSource)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixDestination)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixAmount)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixEnabled)
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
            Parameters::ID::filterMode,
            Parameters::ID::filterCharacter,
            Parameters::ID::filterSlope,
            Parameters::ID::driveAmount
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
            Parameters::ID::oscWarp,
            Parameters::ID::oscWavetablePosition,
            Parameters::ID::osc2WavetablePosition,
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
            Parameters::ID::fxPumpCurve,
            Parameters::ID::fxPumpCustomCurve[0],
            Parameters::ID::fxPumpCustomCurve[1],
            Parameters::ID::fxPumpCustomCurve[2],
            Parameters::ID::fxPumpCustomCurve[3],
            Parameters::ID::fxPumpCustomCurve[4],
            Parameters::ID::fxPumpCustomCurve[5],
            Parameters::ID::fxPumpCustomCurve[6],
            Parameters::ID::fxPumpCustomCurve[7],
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
            Parameters::ID::fxDelaySync,
            Parameters::ID::fxDelayRate,
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

    if (! state.getChildWithProperty("id", Parameters::ID::sampleSliceStyle).isValid())
        setParameterPlainValue(Parameters::ID::sampleSliceStyle, 0.0f);
    const auto hasSampleSliceRegions = state.getChildWithProperty("id", Parameters::ID::sampleSliceStart[0]).isValid()
        && state.getChildWithProperty("id", Parameters::ID::sampleSliceEnd[0]).isValid();
    const auto hasSampleSlicePanProbability = state.getChildWithProperty("id", Parameters::ID::sampleSlicePan[0]).isValid()
        && state.getChildWithProperty("id", Parameters::ID::sampleSliceProbability[0]).isValid();

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
        Parameters::ID::sampleStutterRepeats,
        Parameters::ID::sampleSliceStyle
    });

    for (const auto* parameterID : Parameters::ID::sampleSliceCustom)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceStart)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceEnd)
        restoreParameterFromState(state, parameterID);
    if (! hasSampleSliceRegions)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSliceStart.size(); ++index)
        {
            const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceStart.size());
            const auto equalEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceStart.size());
            setParameterPlainValue(Parameters::ID::sampleSliceStart[index], equalStart);
            setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], equalEnd);
        }
    }
    for (const auto* parameterID : Parameters::ID::sampleSliceReverse)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceTranspose)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceGain)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSlicePan)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceProbability)
        restoreParameterFromState(state, parameterID);
    if (! hasSampleSlicePanProbability)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSlicePan.size(); ++index)
        {
            setParameterPlainValue(Parameters::ID::sampleSlicePan[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
        }
    }
    for (const auto* parameterID : Parameters::ID::sampleSliceStutter)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceChoke)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceStutterRepeats)
        restoreParameterFromState(state, parameterID);
}

void NateVSTAudioProcessor::restoreSequencerFromState(const juce::ValueTree& state)
{
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerScale).isValid())
        setParameterPlainValue(Parameters::ID::sequencerScale, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerChordMode).isValid())
        setParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerChordVoicing).isValid())
        setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerChordStrum).isValid())
        setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerChordMemory).isValid())
        setParameterPlainValue(Parameters::ID::sequencerChordMemory, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerLockDestination).isValid())
        setParameterPlainValue(Parameters::ID::sequencerLockDestination, 0.0f);
    if (! state.getChildWithProperty("id", Parameters::ID::sequencerLockDepth).isValid())
        setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f);

    restoreParameterGroupFromState(state, {
        Parameters::ID::sequencerEnabled,
        Parameters::ID::sequencerRate,
        Parameters::ID::sequencerRoot,
        Parameters::ID::sequencerGate,
        Parameters::ID::sequencerSwing,
        Parameters::ID::sequencerGrooveMode,
        Parameters::ID::sequencerScale,
        Parameters::ID::sequencerChordMode,
        Parameters::ID::sequencerChordVoicing,
        Parameters::ID::sequencerChordStrum,
        Parameters::ID::sequencerChordMemory,
        Parameters::ID::sequencerAccent,
        Parameters::ID::sequencerOctave,
        Parameters::ID::sequencerProbability,
        Parameters::ID::sequencerRandomAmount,
        Parameters::ID::sequencerLockDestination,
        Parameters::ID::sequencerLockDepth
    });

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        Sequencer::Step step;
        step.enabled = static_cast<bool>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_enabled", false));
        step.noteOffset = static_cast<int>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_note", 0));
        step.velocity = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_velocity", 0.8f));
        step.probability = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_probability", 1.0f));
        step.timing = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_timing", 0.0f));
        step.length = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_length", 1.0f));
        step.lock = static_cast<float>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_lock", 0.0f));
        patternSequencer.setStep(stepIndex, step);
    }
}

void NateVSTAudioProcessor::captureSequencerUndoState()
{
    sequencerUndoState = createPluginState(false);
    hasSequencerUndoState = true;
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

Sequencer::HostPosition NateVSTAudioProcessor::getHostPosition() const
{
    Sequencer::HostPosition hostPosition;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            hostPosition.isAvailable = true;
            hostPosition.isPlaying = position->getIsPlaying();
            if (auto ppqPosition = position->getPpqPosition())
                hostPosition.ppqPosition = *ppqPosition;
        }
    }

    return hostPosition;
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
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_length", step.length, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_lock", step.lock, nullptr);
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
    const auto hasSequencerChordMode = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordMode).isValid();
    const auto hasSequencerChordVoicing = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordVoicing).isValid();
    const auto hasSequencerChordStrum = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordStrum).isValid();
    const auto hasSequencerChordMemory = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordMemory).isValid();
    const auto hasSampleSliceStyle = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceStyle).isValid();
    const auto hasSampleSliceEdits = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceCustom[0]).isValid();
    const auto hasSampleSliceRegions = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceStart[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceEnd[0]).isValid();
    const auto hasSampleSlicePanProbability = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSlicePan[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceProbability[0]).isValid();
    const auto hasSampleSliceChoke = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceChoke[0]).isValid();
    const auto hasFilterCharacter = stateForParameters.getChildWithProperty("id", Parameters::ID::filterCharacter).isValid();
    const auto hasFilterSlope = stateForParameters.getChildWithProperty("id", Parameters::ID::filterSlope).isValid();
    const auto hasOscWarp = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWarp).isValid();
    const auto hasWavetablePositions = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWavetablePosition).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::osc2WavetablePosition).isValid();
    const auto hasSequencerLockControls = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerLockDestination).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerLockDepth).isValid();
    const auto hasModMatrixEnabled = stateForParameters.getChildWithProperty("id", Parameters::ID::modMatrixEnabled[0]).isValid();

    if (shouldRestorePerformanceSnapshots)
        restorePerformanceSnapshotsFromState(state);

    loadedSamplePath = stateForParameters.getProperty("sample_file").toString();
    parameters.replaceState(stateForParameters);
    if (! hasSequencerChordMode)
        setParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
    if (! hasSequencerChordVoicing)
        setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 0.0f);
    if (! hasSequencerChordStrum)
        setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.0f);
    if (! hasSequencerChordMemory)
        setParameterPlainValue(Parameters::ID::sequencerChordMemory, 0.0f);
    if (! hasSampleSliceStyle)
        setParameterPlainValue(Parameters::ID::sampleSliceStyle, 0.0f);
    if (! hasSampleSliceEdits)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
        {
            const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
            const auto equalEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
            setParameterPlainValue(Parameters::ID::sampleSliceCustom[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceStart[index], equalStart);
            setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], equalEnd);
            setParameterPlainValue(Parameters::ID::sampleSliceReverse[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceTranspose[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceGain[index], -6.0f);
            setParameterPlainValue(Parameters::ID::sampleSlicePan[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceStutter[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceChoke[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceStutterRepeats[index], 3.0f);
        }
    }
    if (! hasSampleSliceRegions)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSliceStart.size(); ++index)
        {
            const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceStart.size());
            const auto equalEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceStart.size());
            setParameterPlainValue(Parameters::ID::sampleSliceStart[index], equalStart);
            setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], equalEnd);
        }
    }
    if (! hasSampleSliceChoke)
        for (const auto* parameterID : Parameters::ID::sampleSliceChoke)
            setParameterPlainValue(parameterID, 0.0f);
    if (! hasSampleSlicePanProbability)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSlicePan.size(); ++index)
        {
            setParameterPlainValue(Parameters::ID::sampleSlicePan[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
        }
    }

    if (! hasFilterCharacter)
        setParameterPlainValue(Parameters::ID::filterCharacter, 0.0f);
    if (! hasFilterSlope)
        setParameterPlainValue(Parameters::ID::filterSlope, 0.0f);
    if (! hasOscWarp)
        setParameterPlainValue(Parameters::ID::oscWarp, 0.0f);
    if (! hasWavetablePositions)
    {
        setParameterPlainValue(Parameters::ID::oscWavetablePosition, 0.0f);
        setParameterPlainValue(Parameters::ID::osc2WavetablePosition, 0.35f);
    }
    if (! hasSequencerLockControls)
    {
        setParameterPlainValue(Parameters::ID::sequencerLockDestination, 0.0f);
        setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f);
    }
    if (! hasModMatrixEnabled)
        for (const auto* parameterID : Parameters::ID::modMatrixEnabled)
            setParameterPlainValue(parameterID, 1.0f);

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
        step.length = static_cast<float>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_length", 1.0f));
        step.lock = static_cast<float>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_lock", 0.0f));
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
    state.addChild(juce::ValueTree(ratingsType), -1, nullptr);
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
    return findPresetFileInDirectory(getPresetDirectory(), presetName);
}

juce::File NateVSTAudioProcessor::presetFileForName(const juce::String& presetName, const juce::String& category) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    return presetDirectoryForCategory(category).getChildFile(legalName).withFileExtension(".natevstpreset");
}

juce::File NateVSTAudioProcessor::factoryPresetFileForName(const juce::String& presetName) const
{
    return findPresetFileInDirectory(getFactoryPresetDirectory(), presetName);
}

juce::File NateVSTAudioProcessor::findPresetFileInDirectory(const juce::File& directory, const juce::String& presetName) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    const auto directFile = directory.getChildFile(legalName).withFileExtension(".natevstpreset");
    if (directFile.existsAsFile())
        return directFile;

    const auto presetFiles = directory.findChildFiles(juce::File::findFiles, true, "*.natevstpreset");
    for (const auto& file : presetFiles)
        if (file.getFileNameWithoutExtension() == legalName)
            return file;

    return directFile;
}

juce::File NateVSTAudioProcessor::presetDirectoryForCategory(const juce::String& category) const
{
    auto directory = getPresetDirectory();
    const auto segments = presetCategoryPathSegments(category);
    for (const auto& segment : segments)
        directory = directory.getChildFile(segment);

    return directory;
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
