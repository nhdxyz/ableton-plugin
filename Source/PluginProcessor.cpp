#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <memory>
#include <vector>

namespace
{
const juce::Identifier libraryStateType { "NateVSTLibrary" };
const juce::Identifier favoritesType { "Favorites" };
const juce::Identifier ratingsType { "Ratings" };
const juce::Identifier recentType { "Recent" };
const juce::Identifier presetRefType { "Preset" };
const juce::Identifier performanceSnapshotType { "PerformanceSnapshot" };
const juce::Identifier sequencerPatternSceneType { "SequencerPatternScene" };
constexpr auto sequencerSceneChainPlaybackProperty = "seq_scene_chain_playback_enabled";
constexpr auto sequencerSceneChainClipBarsProperty = "seq_scene_chain_clip_bars";
constexpr auto maxGlobalEditHistoryDepth = 24;
constexpr auto presetPreviewSampleRate = 44100.0;
constexpr auto presetPreviewBlockSize = 512;
constexpr auto presetPreviewChannelCount = 2;
constexpr auto presetPreviewDurationSeconds = 2.35;
constexpr auto sampleCaptureMaxSeconds = 16.0;
constexpr auto sampleCaptureMaxPreRollSeconds = 0.5;
constexpr auto sampleCaptureTakeHistoryLimit = 12;
constexpr auto sequencerFourBarChainTransformIndex = 12;
constexpr auto sequencerChordStabPaintTransformIndex = 13;

float calculateBufferPeak(const juce::AudioBuffer<float>& buffer, int sourceChannelLimit = -1) noexcept
{
    const auto sourceChannels = sourceChannelLimit >= 0
        ? juce::jmin(buffer.getNumChannels(), sourceChannelLimit)
        : buffer.getNumChannels();
    const auto blockSamples = buffer.getNumSamples();
    auto peak = 0.0f;

    if (sourceChannels > 0 && blockSamples > 0)
    {
        for (auto channel = 0; channel < sourceChannels; ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < blockSamples; ++sample)
                peak = juce::jmax(peak, std::abs(samples[sample]));
        }
    }

    return peak;
}

enum class PresetPreviewRole
{
    bass,
    chord,
    chop,
    sequence,
    fx,
    lead,
    pluck,
    general
};

struct PresetPreviewMidiEvent
{
    int sample = 0;
    juce::MidiMessage message;
};

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

int foldPreviewNoteToRange(int note, int low, int high) noexcept
{
    while (note < low)
        note += 12;

    while (note > high)
        note -= 12;

    return juce::jlimit(0, 127, note);
}

juce::String presetPreviewDescriptor(const NateVSTAudioProcessor::PresetInfo* presetInfo, const juce::String& presetName)
{
    if (presetInfo == nullptr)
        return presetName.toLowerCase();

    return (presetInfo->name + " " + presetInfo->category + " " + presetInfo->pack + " "
            + presetInfo->tags + " " + presetInfo->notes)
        .toLowerCase();
}

PresetPreviewRole presetPreviewRoleForDescriptor(const juce::String& descriptor)
{
    if (descriptor.contains("bass")
        || descriptor.contains("reese")
        || descriptor.contains("rumble")
        || descriptor.contains("sub")
        || descriptor.contains("acid")
        || descriptor.contains("wobble")
        || descriptor.contains("log drum"))
        return PresetPreviewRole::bass;

    if (descriptor.contains("chord")
        || descriptor.contains("stab")
        || descriptor.contains("keys")
        || descriptor.contains("piano")
        || descriptor.contains("organ")
        || descriptor.contains("dub"))
        return PresetPreviewRole::chord;

    if (descriptor.contains("chop") || descriptor.contains("vocal") || descriptor.contains("garage"))
        return PresetPreviewRole::chop;

    if (descriptor.contains("sequence") || descriptor.contains("hypno") || descriptor.contains("arp"))
        return PresetPreviewRole::sequence;

    if (descriptor.contains("riser")
        || descriptor.contains("sweep")
        || descriptor.contains("noise")
        || descriptor.contains("fx")
        || descriptor.contains("throw"))
        return PresetPreviewRole::fx;

    if (descriptor.contains("lead"))
        return PresetPreviewRole::lead;

    if (descriptor.contains("pluck")
        || descriptor.contains("bell")
        || descriptor.contains("marimba")
        || descriptor.contains("kalimba")
        || descriptor.contains("perc")
        || descriptor.contains("tick")
        || descriptor.contains("hit"))
        return PresetPreviewRole::pluck;

    return PresetPreviewRole::general;
}

NateVSTAudioProcessor::PresetPreviewInfo measurePresetPreviewBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    NateVSTAudioProcessor::PresetPreviewInfo info;
    info.durationSeconds = sampleRate > 0.0
        ? static_cast<double>(buffer.getNumSamples()) / sampleRate
        : 0.0;

    double sumSquares = 0.0;
    auto sampleCount = 0;
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = samples[sample];
            if (! std::isfinite(value))
            {
                info.status = "Preview render was non-finite";
                return info;
            }

            info.peak = juce::jmax(info.peak, std::abs(value));
            sumSquares += static_cast<double>(value) * static_cast<double>(value);
            ++sampleCount;
        }
    }

    info.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    return info;
}

bool writePresetPreviewFile(const juce::File& file, const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (! file.getParentDirectory().createDirectory())
        return false;

    if (file.existsAsFile() && ! file.deleteFile())
        return false;

    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());
    if (fileStream == nullptr || ! fileStream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));

    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(16);
    auto writer = format.createWriterFor(stream, options);
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

bool readPresetPreviewFile(const juce::File& file, juce::AudioBuffer<float>& buffer)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const auto channels = juce::jlimit(1, presetPreviewChannelCount, static_cast<int>(reader->numChannels));
    const auto samples = static_cast<int>(juce::jmin<juce::int64>(reader->lengthInSamples,
                                                                  static_cast<juce::int64>(std::round(presetPreviewSampleRate * presetPreviewDurationSeconds))));
    buffer.setSize(presetPreviewChannelCount, samples);
    buffer.clear();
    reader->read(&buffer, 0, samples, 0, true, channels > 1);

    if (channels == 1 && presetPreviewChannelCount > 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, samples);

    return true;
}

juce::String previewLevelText(float peak, float rms)
{
    if (peak <= 0.0f || rms <= 0.0f)
        return "silent";

    const auto peakDb = juce::Decibels::gainToDecibels(peak, -100.0f);
    const auto rmsDb = juce::Decibels::gainToDecibels(rms, -100.0f);
    return "peak " + juce::String(peakDb, 1) + " dB | rms " + juce::String(rmsDb, 1) + " dB";
}

juce::String presetPreviewRoleName(PresetPreviewRole role)
{
    switch (role)
    {
        case PresetPreviewRole::bass: return "bass loop";
        case PresetPreviewRole::chord: return "chord stab";
        case PresetPreviewRole::chop: return "chop phrase";
        case PresetPreviewRole::sequence: return "sequence";
        case PresetPreviewRole::fx: return "FX cue";
        case PresetPreviewRole::lead: return "lead hook";
        case PresetPreviewRole::pluck: return "pluck hook";
        case PresetPreviewRole::general: return "patch cue";
    }

    return "patch cue";
}

void addPresetPreviewNote(std::vector<PresetPreviewMidiEvent>& events,
                          int note,
                          double startSeconds,
                          double durationSeconds,
                          float velocity)
{
    const auto startSample = juce::jmax(0, static_cast<int>(std::round(startSeconds * presetPreviewSampleRate)));
    const auto endSample = juce::jmax(startSample + 1,
                                      static_cast<int>(std::round((startSeconds + durationSeconds) * presetPreviewSampleRate)));
    events.push_back({ startSample, juce::MidiMessage::noteOn(1,
                                                               juce::jlimit(0, 127, note),
                                                               juce::jlimit(0.05f, 1.0f, velocity)) });
    events.push_back({ endSample, juce::MidiMessage::noteOff(1, juce::jlimit(0, 127, note)) });
}

void addPresetPreviewChord(std::vector<PresetPreviewMidiEvent>& events,
                           int root,
                           std::initializer_list<int> intervals,
                           double startSeconds,
                           double durationSeconds,
                           float velocity)
{
    for (const auto interval : intervals)
        addPresetPreviewNote(events,
                             foldPreviewNoteToRange(root + interval, 36, 84),
                             startSeconds,
                             durationSeconds,
                             velocity);
}

void buildPresetPreviewMidi(std::vector<PresetPreviewMidiEvent>& events, PresetPreviewRole role, int rootNote)
{
    switch (role)
    {
        case PresetPreviewRole::bass:
        {
            const auto base = foldPreviewNoteToRange(rootNote - 12, 36, 48);
            addPresetPreviewNote(events, base, 0.00, 0.18, 0.94f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 7, 36, 52), 0.28, 0.15, 0.76f);
            addPresetPreviewNote(events, base, 0.55, 0.16, 0.88f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 10, 36, 52), 0.84, 0.18, 0.80f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 12, 36, 52), 1.18, 0.48, 0.92f);
            break;
        }

        case PresetPreviewRole::chord:
        {
            const auto base = foldPreviewNoteToRange(rootNote, 48, 60);
            addPresetPreviewChord(events, base, { 0, 3, 7, 10 }, 0.00, 0.30, 0.82f);
            addPresetPreviewChord(events, base + 5, { 0, 3, 7 }, 0.52, 0.25, 0.70f);
            addPresetPreviewChord(events, base + 7, { 0, 3, 7, 10 }, 1.02, 0.46, 0.76f);
            break;
        }

        case PresetPreviewRole::chop:
        {
            const auto base = foldPreviewNoteToRange(rootNote + 12, 60, 72);
            addPresetPreviewNote(events, base, 0.00, 0.11, 0.84f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 3, 60, 76), 0.16, 0.10, 0.72f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 7, 60, 76), 0.34, 0.13, 0.82f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 10, 60, 76), 0.57, 0.11, 0.70f);
            addPresetPreviewNote(events, base, 0.78, 0.28, 0.86f);
            break;
        }

        case PresetPreviewRole::sequence:
        {
            const auto base = foldPreviewNoteToRange(rootNote, 48, 60);
            for (auto step = 0; step < 8; ++step)
                addPresetPreviewNote(events,
                                     foldPreviewNoteToRange(base + std::array<int, 8> { 0, 3, 7, 10, 12, 10, 7, 3 }[static_cast<size_t>(step)], 48, 76),
                                     static_cast<double>(step) * 0.15,
                                     0.10,
                                     step % 2 == 0 ? 0.84f : 0.68f);
            break;
        }

        case PresetPreviewRole::fx:
        {
            const auto base = foldPreviewNoteToRange(rootNote + 12, 60, 72);
            addPresetPreviewNote(events, base, 0.00, 0.65, 0.78f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 12, 60, 84), 0.72, 0.78, 0.70f);
            break;
        }

        case PresetPreviewRole::lead:
        {
            const auto base = foldPreviewNoteToRange(rootNote + 12, 60, 72);
            addPresetPreviewNote(events, base, 0.00, 0.22, 0.82f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 3, 60, 84), 0.28, 0.18, 0.74f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 7, 60, 84), 0.54, 0.30, 0.86f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 12, 60, 84), 0.96, 0.42, 0.78f);
            break;
        }

        case PresetPreviewRole::pluck:
        {
            const auto base = foldPreviewNoteToRange(rootNote + 12, 60, 72);
            addPresetPreviewNote(events, base, 0.00, 0.12, 0.86f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 7, 60, 84), 0.24, 0.12, 0.70f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 10, 60, 84), 0.48, 0.14, 0.76f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 12, 60, 84), 0.76, 0.22, 0.82f);
            break;
        }

        case PresetPreviewRole::general:
        default:
        {
            const auto base = foldPreviewNoteToRange(rootNote, 48, 60);
            addPresetPreviewNote(events, base, 0.00, 0.28, 0.82f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 7, 48, 76), 0.42, 0.24, 0.72f);
            addPresetPreviewNote(events, foldPreviewNoteToRange(base + 12, 48, 76), 0.84, 0.42, 0.78f);
            break;
        }
    }

    std::stable_sort(events.begin(),
                     events.end(),
                     [] (const auto& left, const auto& right)
                     {
                         return left.sample < right.sample;
                     });
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

int sanitiseSequencerSceneChainClipBars(int barCount)
{
    return barCount == 2 || barCount == 4 ? barCount : 0;
}

float readPresetParameterValue(const juce::ValueTree& state, const char* parameterID, float fallback)
{
    if (const auto child = state.getChildWithProperty("id", parameterID); child.isValid())
        return static_cast<float>(child.getProperty("value", fallback));

    return fallback;
}

Sequencer::Step readSequencerExportStep(const juce::ValueTree& state, int stepIndex)
{
    const auto prefix = "seq_step_" + juce::String(juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1, stepIndex));
    Sequencer::Step step;
    step.enabled = static_cast<bool>(state.getProperty(prefix + "_enabled", false));
    step.noteOffset = static_cast<int>(state.getProperty(prefix + "_note", 0));
    step.velocity = static_cast<float>(state.getProperty(prefix + "_velocity", 0.8f));
    step.probability = static_cast<float>(state.getProperty(prefix + "_probability", 1.0f));
    step.timing = static_cast<float>(state.getProperty(prefix + "_timing", 0.0f));
    step.length = static_cast<float>(state.getProperty(prefix + "_length", 1.0f));
    step.lock = static_cast<float>(state.getProperty(prefix + "_lock", 0.0f));
    step.ratchet = static_cast<int>(state.getProperty(prefix + "_ratchet", 1));
    step.condition = static_cast<int>(state.getProperty(prefix + "_condition", 0));
    step.slide = static_cast<bool>(state.getProperty(prefix + "_slide", false));
    return step;
}

Sequencer::PatternSequencer::SceneControls readSequencerSceneControls(const juce::ValueTree& state)
{
    Sequencer::PatternSequencer::SceneControls controls;
    controls.root = readPresetParameterValue(state, Parameters::ID::sequencerRoot, 36.0f);
    controls.gate = readPresetParameterValue(state, Parameters::ID::sequencerGate, 0.55f);
    controls.swing = readPresetParameterValue(state, Parameters::ID::sequencerSwing, 0.0f);
    controls.grooveMode = readPresetParameterValue(state, Parameters::ID::sequencerGrooveMode, 0.0f);
    controls.scale = readPresetParameterValue(state, Parameters::ID::sequencerScale, 0.0f);
    controls.chordMode = readPresetParameterValue(state, Parameters::ID::sequencerChordMode, 0.0f);
    controls.chordVoicing = readPresetParameterValue(state, Parameters::ID::sequencerChordVoicing, 0.0f);
    controls.chordStrum = readPresetParameterValue(state, Parameters::ID::sequencerChordStrum, 0.0f);
    controls.accent = readPresetParameterValue(state, Parameters::ID::sequencerAccent, 0.35f);
    controls.octave = readPresetParameterValue(state, Parameters::ID::sequencerOctave, 0.0f);
    controls.probability = readPresetParameterValue(state, Parameters::ID::sequencerProbability, 1.0f);
    return controls;
}

void writeSequencerExportStep(juce::ValueTree& state, int stepIndex, Sequencer::Step step)
{
    const auto safeIndex = juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1, stepIndex);
    const auto prefix = "seq_step_" + juce::String(safeIndex);
    state.setProperty(prefix + "_enabled", step.enabled, nullptr);
    state.setProperty(prefix + "_note",
                      juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                                   Sequencer::PatternSequencer::maxNoteOffset,
                                   step.noteOffset),
                      nullptr);
    state.setProperty(prefix + "_velocity", juce::jlimit(0.0f, 1.0f, step.velocity), nullptr);
    state.setProperty(prefix + "_probability", juce::jlimit(0.0f, 1.0f, step.probability), nullptr);
    state.setProperty(prefix + "_timing", juce::jlimit(0.0f, 1.0f, step.timing), nullptr);
    state.setProperty(prefix + "_length", juce::jlimit(0.05f, 1.0f, step.length), nullptr);
    state.setProperty(prefix + "_lock", juce::jlimit(0.0f, 1.0f, step.lock), nullptr);
    state.setProperty(prefix + "_ratchet",
                      juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                   Sequencer::PatternSequencer::maxRatchet,
                                   step.ratchet),
                      nullptr);
    state.setProperty(prefix + "_condition",
                      juce::jlimit(Sequencer::PatternSequencer::minCondition,
                                   Sequencer::PatternSequencer::maxCondition,
                                   step.condition),
                      nullptr);
    state.setProperty(prefix + "_slide", step.enabled && step.ratchet <= 1 && step.slide, nullptr);
}

void writePresetParameterValue(juce::ValueTree& state, const char* parameterID, float value)
{
    auto child = state.getChildWithProperty("id", parameterID);
    if (! child.isValid())
    {
        child = juce::ValueTree("PARAM");
        child.setProperty("id", parameterID, nullptr);
        state.addChild(child, -1, nullptr);
    }

    child.setProperty("value", value, nullptr);
}

bool sequencerExportConditionAllows(int condition, int cycleIndex, int cycleCount)
{
    cycleIndex = juce::jmax(0, cycleIndex);
    cycleCount = juce::jmax(1, cycleCount);

    switch (juce::jlimit(Sequencer::PatternSequencer::minCondition,
                         Sequencer::PatternSequencer::maxCondition,
                         condition))
    {
        case 1:
            return (cycleIndex % 2) == 0;

        case 2:
            return (cycleIndex % 2) != 0;

        case 3:
            return cycleCount == 1 || cycleIndex == cycleCount - 1;

        case 0:
        default:
            return true;
    }
}

bool sequencerExportOffsetInScale(int noteOffset, int scaleMode)
{
    const auto pitchClass = ((noteOffset % 12) + 12) % 12;
    auto contains = [pitchClass] (std::initializer_list<int> scale)
    {
        for (const auto degree : scale)
            if (pitchClass == degree)
                return true;

        return false;
    };

    switch (scaleMode)
    {
        case 1: return contains({ 0, 2, 4, 5, 7, 9, 11 });
        case 2: return contains({ 0, 2, 3, 5, 7, 8, 10 });
        case 3: return contains({ 0, 2, 3, 5, 7, 9, 10 });
        case 4: return contains({ 0, 3, 5, 7, 10 });
        default: return true;
    }
}

int quantizeSequencerExportNoteOffset(int noteOffset, int scaleMode)
{
    noteOffset = juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                              Sequencer::PatternSequencer::maxNoteOffset,
                              noteOffset);
    if (scaleMode <= 0)
        return noteOffset;

    auto bestOffset = noteOffset;
    auto bestDistance = 128;
    for (auto candidate = -24; candidate <= 24; ++candidate)
    {
        if (! sequencerExportOffsetInScale(candidate, scaleMode))
            continue;

        const auto distance = std::abs(candidate - noteOffset);
        if (distance < bestDistance || (distance == bestDistance && std::abs(candidate) < std::abs(bestOffset)))
        {
            bestOffset = candidate;
            bestDistance = distance;
        }
    }

    return juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                        Sequencer::PatternSequencer::maxNoteOffset,
                        bestOffset);
}

int sequencerExportChordIntervals(int chordMode, std::array<int, Sequencer::PatternSequencer::maxChordNotes>& intervals)
{
    intervals.fill(0);
    switch (chordMode)
    {
        case 1: intervals = { 0, 7, 0, 0, 0 }; return 2;
        case 2: intervals = { 0, 3, 7, 0, 0 }; return 3;
        case 3: intervals = { 0, 3, 7, 10, 0 }; return 4;
        case 4: intervals = { 0, 4, 7, 0, 0 }; return 3;
        case 5: intervals = { 0, 3, 7, 10, 14 }; return 5;
        case 6: intervals = { 0, 4, 7, 11, 0 }; return 4;
        case 7: intervals = { 0, 4, 7, 10, 0 }; return 4;
        case 8: intervals = { 0, 5, 7, 0, 0 }; return 3;
        case 9: intervals = { 0, 4, 7, 10, 14 }; return 5;
        case 10: intervals = { 0, 7, 10, 14, 17 }; return 5;
        case 0:
        default: intervals = { 0, 0, 0, 0, 0 }; return 1;
    }
}

void applySequencerExportVoicing(int voicing, std::array<int, Sequencer::PatternSequencer::maxChordNotes>& intervals, int& intervalCount)
{
    if (intervalCount <= 1)
        return;

    switch (voicing)
    {
        case 1:
        {
            const auto first = intervals[0] + 12;
            for (auto index = 0; index < intervalCount - 1; ++index)
                intervals[static_cast<size_t>(index)] = intervals[static_cast<size_t>(index + 1)];
            intervals[static_cast<size_t>(intervalCount - 1)] = first;
            break;
        }

        case 2:
        {
            const auto moveCount = juce::jmin(2, intervalCount - 1);
            std::array<int, Sequencer::PatternSequencer::maxChordNotes> voiced {};
            auto writeIndex = 0;
            for (auto index = moveCount; index < intervalCount; ++index)
                voiced[static_cast<size_t>(writeIndex++)] = intervals[static_cast<size_t>(index)];
            for (auto index = 0; index < moveCount; ++index)
                voiced[static_cast<size_t>(writeIndex++)] = intervals[static_cast<size_t>(index)] + 12;
            intervals = voiced;
            break;
        }

        case 3:
            for (auto index = 1; index < intervalCount; index += 2)
                intervals[static_cast<size_t>(index)] += 12;
            std::sort(intervals.begin(), intervals.begin() + intervalCount);
            break;

        case 4:
            if (intervalCount >= 3)
            {
                intervals[static_cast<size_t>(intervalCount - 2)] -= 12;
                std::sort(intervals.begin(), intervals.begin() + intervalCount);
            }
            break;

        case 0:
        default:
            break;
    }
}

Sequencer::PatternSequencer::ChordNoteArray sequencerExportChordNotes(int rootNote,
                                                                      int octaveOffset,
                                                                      int noteOffset,
                                                                      int scaleMode,
                                                                      int chordMode,
                                                                      int voicing,
                                                                      int& noteCount)
{
    Sequencer::PatternSequencer::ChordNoteArray notes {};
    notes.fill(-1);
    noteCount = 0;

    std::array<int, Sequencer::PatternSequencer::maxChordNotes> intervals {};
    auto intervalCount = sequencerExportChordIntervals(chordMode, intervals);
    applySequencerExportVoicing(voicing, intervals, intervalCount);
    const auto baseNoteNumber = rootNote + octaveOffset + quantizeSequencerExportNoteOffset(noteOffset, scaleMode);

    auto addInterval = [&notes, &noteCount, baseNoteNumber] (int interval)
    {
        if (noteCount >= Sequencer::PatternSequencer::maxChordNotes)
            return;

        const auto noteNumber = juce::jlimit(0, 127, baseNoteNumber + interval);
        for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
            if (notes[static_cast<size_t>(noteIndex)] == noteNumber)
                return;

        notes[static_cast<size_t>(noteCount++)] = noteNumber;
    };

    for (auto intervalIndex = 0; intervalIndex < intervalCount; ++intervalIndex)
        addInterval(intervals[static_cast<size_t>(intervalIndex)]);

    return notes;
}

float sequencerExportChordNoteVelocity(float velocity, int noteIndex)
{
    if (noteIndex <= 0)
        return juce::jlimit(0.0f, 1.0f, velocity);

    const auto trim = 0.92f - (0.04f * static_cast<float>(noteIndex - 1));
    return juce::jlimit(0.0f, 1.0f, velocity * juce::jmax(0.72f, trim));
}

int sequencerExportChordStrumOffset(int stepTicks, int noteIndex, int noteCount, float strumAmount)
{
    if (noteIndex <= 0 || noteCount <= 1 || stepTicks <= 1)
        return 0;

    strumAmount = juce::jlimit(0.0f, 1.0f, strumAmount);
    if (strumAmount <= 0.0f)
        return 0;

    const auto maxSpread = static_cast<float>(stepTicks) * 0.18f * strumAmount;
    const auto notePosition = static_cast<float>(noteIndex) / static_cast<float>(juce::jmax(1, noteCount - 1));
    return juce::roundToInt(maxSpread * notePosition);
}

const std::array<const char*, 8>& presetMacroLabels()
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

    return labels;
}

const std::array<const char*, 8>& presetMacroParameterIDs()
{
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

    return ids;
}

const std::array<const char*, 17>& sequencerSceneParameterIDs()
{
    static constexpr std::array<const char*, 17> ids {
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
    };

    return ids;
}

const std::array<const char*, 4>& sequencerPatternSceneLabels()
{
    static constexpr std::array<const char*, 4> labels { "A", "B", "Fill", "Drop" };
    return labels;
}

std::array<float, 8> presetMacroValues(const juce::ValueTree& state)
{
    std::array<float, 8> values {};
    const auto& ids = presetMacroParameterIDs();

    for (size_t index = 0; index < ids.size(); ++index)
        values[index] = juce::jlimit(0.0f, 1.0f, readPresetParameterValue(state, ids[index], 0.0f));

    return values;
}

juce::String presetMacroSummary(const std::array<float, 8>& macroValues, float& intensity)
{
    struct MacroPreview
    {
        juce::String label;
        float value = 0.0f;
    };

    std::array<MacroPreview, 8> previews {};
    const auto& labels = presetMacroLabels();
    intensity = 0.0f;

    for (size_t index = 0; index < macroValues.size(); ++index)
    {
        const auto value = juce::jlimit(0.0f, 1.0f, macroValues[index]);
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
    : AudioProcessor(BusesProperties()
                         .withInput("Host Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", Parameters::createLayout()),
      synthEngine(parameters),
      randomizer(parameters),
      samplePlayer(parameters),
      patternSequencer(parameters),
      effectsRack(parameters),
      sampleRandomEngine(std::random_device{}())
{
    outputGain = parameters.getRawParameterValue(Parameters::ID::outputGain);
    sampleRecordSource = parameters.getRawParameterValue(Parameters::ID::sampleRecordSource);
    sampleRecordStart = parameters.getRawParameterValue(Parameters::ID::sampleRecordStart);
    sampleRecordLength = parameters.getRawParameterValue(Parameters::ID::sampleRecordLength);
    sampleRecordPreRoll = parameters.getRawParameterValue(Parameters::ID::sampleRecordPreRoll);
    sequencerChordMemory = parameters.getRawParameterValue(Parameters::ID::sequencerChordMemory);
    sequencerLockDestination = parameters.getRawParameterValue(Parameters::ID::sequencerLockDestination);
    sequencerLockDepth = parameters.getRawParameterValue(Parameters::ID::sequencerLockDepth);
    clearChordMemoryActiveNotes();
}

void NateVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    meterSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    preparedSamplesPerBlock = samplesPerBlock > 0 ? samplesPerBlock : 512;
    chordMemoryScratchMidi.clear();
    chordMemoryScratchMidi.ensureSize(static_cast<size_t>(juce::jmax(8192, preparedSamplesPerBlock * 8)));
    synthEngine.prepare(sampleRate, samplesPerBlock);
    samplePlayer.prepare(sampleRate);
    patternSequencer.prepare(sampleRate);
    effectsRack.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    sampleCaptureEnabled.store(false, std::memory_order_release);
    invalidateSampleCaptureSession();
    waitForSampleCaptureWritersToFinish();
    sampleCaptureSampleRate = meterSampleRate;
    const auto captureChannels = juce::jlimit(1,
                                              2,
                                              juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels()));
    for (auto& captureBuffer : sampleCaptureBuffers)
    {
        captureBuffer.setSize(captureChannels,
                              juce::jmax(1, static_cast<int>(std::round(sampleCaptureMaxSeconds * sampleCaptureSampleRate))));
        captureBuffer.clear();
    }
    sampleCapturePreRollBuffer.setSize(captureChannels,
                                       juce::jmax(1, static_cast<int>(std::round(sampleCaptureMaxPreRollSeconds * sampleCaptureSampleRate))));
    sampleCapturePreRollBuffer.clear();
    activeSampleCaptureBufferIndex.store(0, std::memory_order_relaxed);
    sampleCaptureWritePosition.store(0, std::memory_order_relaxed);
    sampleCaptureSamplesRecorded.store(0, std::memory_order_relaxed);
    sampleCaptureTargetSamples.store(0, std::memory_order_relaxed);
    sampleCapturePreRollWritePosition.store(0, std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_relaxed);
    sampleCaptureSourcePeak.store(0.0f, std::memory_order_relaxed);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_relaxed);
    outputMeterPeakLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterPeakRight.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsLeft.store(0.0f, std::memory_order_relaxed);
    outputMeterRmsRight.store(0.0f, std::memory_order_relaxed);
    stereoFieldCorrelation.store(0.0f, std::memory_order_relaxed);
    stereoFieldWidth.store(0.0f, std::memory_order_relaxed);
    stereoFieldBalance.store(0.0f, std::memory_order_relaxed);
    lowEndSubRms.store(0.0f, std::memory_order_relaxed);
    lowEndStereoRisk.store(0.0f, std::memory_order_relaxed);
    lowEndOutputPeak.store(0.0f, std::memory_order_relaxed);
    for (auto& sample : outputSpectrumSamples)
        sample.store(0.0f, std::memory_order_relaxed);
    outputSpectrumWriteCursor = 0;
    outputSpectrumWriteIndex.store(0, std::memory_order_relaxed);
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
    sampleCaptureEnabled.store(false, std::memory_order_relaxed);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_relaxed);
    invalidateSampleCaptureSession();
    sampleCaptureTargetSamples.store(0, std::memory_order_relaxed);
    sampleCapturePreRollWritePosition.store(0, std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_relaxed);
    effectsRack.reset();
    patternSequencer.reset();
    clearChordMemoryActiveNotes();
    hostSyncPlaying.store(false, std::memory_order_relaxed);
    hostSyncPpqAvailable.store(false, std::memory_order_relaxed);
}

bool NateVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto inputSupported = mainInput.isDisabled()
        || mainInput == juce::AudioChannelSet::mono()
        || mainInput == juce::AudioChannelSet::stereo();
    const auto outputSupported = mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
    return inputSupported && outputSupported;
}

void NateVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto captureSource = getSampleCaptureSourceIndex();
    if (captureSource == 1)
    {
        updateSampleCaptureSourcePeak(buffer, getTotalNumInputChannels());
        appendToSampleCapture(buffer, getTotalNumInputChannels());
    }

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
    const auto syncedPpqPosition = hostPosition.isPlaying ? hostPosition.ppqPosition : std::nullopt;
    synthEngine.render(buffer, midiMessages, hostBpm, syncedPpqPosition);
    samplePlayer.render(buffer,
                        midiMessages,
                        hostBpm,
                        syncedPpqPosition);
    effectsRack.process(buffer,
                        midiMessages,
                        outputGain != nullptr ? outputGain->load() : -8.0f,
                        hostBpm,
                        syncedPpqPosition);
    if (captureSource == 0)
    {
        updateSampleCaptureSourcePeak(buffer);
        appendToSampleCapture(buffer);
    }
    mixPresetPreviewPlayback(buffer);
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

    auto& expandedMidi = chordMemoryScratchMidi;
    expandedMidi.clear();

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
    return 4.0;
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

void NateVSTAudioProcessor::captureGlobalEditState(const juce::String& label)
{
    auto state = createPluginState();
    if (globalUndoStack.empty() || ! globalUndoStack.back().state.isEquivalentTo(state))
    {
        globalUndoStack.push_back({ state.createCopy(), label.trim().isNotEmpty() ? label.trim() : juce::String("Edit") });
        while (globalUndoStack.size() > static_cast<size_t>(maxGlobalEditHistoryDepth))
            globalUndoStack.erase(globalUndoStack.begin());
    }

    globalRedoStack.clear();
}

bool NateVSTAudioProcessor::undoGlobalEdit()
{
    auto currentState = createPluginState();
    while (! globalUndoStack.empty() && globalUndoStack.back().state.isEquivalentTo(currentState))
        globalUndoStack.pop_back();

    if (globalUndoStack.empty())
        return false;

    const auto snapshot = globalUndoStack.back();
    globalUndoStack.pop_back();
    globalRedoStack.push_back({ currentState.createCopy(), snapshot.label });
    restorePluginState(snapshot.state.createCopy());
    return true;
}

bool NateVSTAudioProcessor::redoGlobalEdit()
{
    auto currentState = createPluginState();
    while (! globalRedoStack.empty() && globalRedoStack.back().state.isEquivalentTo(currentState))
        globalRedoStack.pop_back();

    if (globalRedoStack.empty())
        return false;

    const auto snapshot = globalRedoStack.back();
    globalRedoStack.pop_back();
    globalUndoStack.push_back({ currentState.createCopy(), snapshot.label });
    while (globalUndoStack.size() > static_cast<size_t>(maxGlobalEditHistoryDepth))
        globalUndoStack.erase(globalUndoStack.begin());

    restorePluginState(snapshot.state.createCopy());
    return true;
}

bool NateVSTAudioProcessor::canUndoGlobalEdit() const
{
    return ! globalUndoStack.empty();
}

bool NateVSTAudioProcessor::canRedoGlobalEdit() const
{
    return ! globalRedoStack.empty();
}

juce::String NateVSTAudioProcessor::getGlobalEditHistorySummary() const
{
    const auto undoText = globalUndoStack.empty() ? juce::String("Undo: none")
                                                  : "Undo: " + globalUndoStack.back().label;
    const auto redoText = globalRedoStack.empty() ? juce::String("Redo: none")
                                                  : "Redo: " + globalRedoStack.back().label;
    return undoText + " | " + redoText;
}

bool NateVSTAudioProcessor::hasRandomCandidate(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(randomCandidateSnapshots.size()))
        return false;

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    return candidate.valid && candidate.state.isValid();
}

juce::String NateVSTAudioProcessor::getRandomCandidateSummary(int slotIndex) const
{
    if (! hasRandomCandidate(slotIndex))
        return {};

    return randomCandidateSnapshots[static_cast<size_t>(slotIndex)].label;
}

juce::String NateVSTAudioProcessor::getRandomCandidateCompareSummary(int slotIndex)
{
    if (! hasRandomCandidate(slotIndex))
        return {};

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    const auto current = createPluginState(false);
    juce::StringArray changes;

    auto addDirection = [&changes] (float candidateValue,
                                    float currentValue,
                                    float threshold,
                                    const juce::String& higher,
                                    const juce::String& lower)
    {
        const auto delta = candidateValue - currentValue;
        if (delta > threshold)
            changes.add(higher);
        else if (delta < -threshold)
            changes.add(lower);
    };

    const auto candidateCutoff = readStateParameterValue(candidate.state, Parameters::ID::filterCutoff, 1000.0f);
    const auto currentCutoff = readStateParameterValue(current, Parameters::ID::filterCutoff, 1000.0f);
    if (candidateCutoff > currentCutoff * 1.25f)
        changes.add("brighter");
    else if (candidateCutoff < currentCutoff * 0.80f)
        changes.add("darker");

    const auto candidateDirt = readStateParameterValue(candidate.state, Parameters::ID::driveAmount, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::fxDistortionAmount, 0.0f)
        + (readStateParameterValue(candidate.state, Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.25f)
        + readStateParameterValue(candidate.state, Parameters::ID::fxBitcrushMix, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::macroDirt, 0.0f);
    const auto currentDirt = readStateParameterValue(current, Parameters::ID::driveAmount, 0.0f)
        + readStateParameterValue(current, Parameters::ID::fxDistortionAmount, 0.0f)
        + (readStateParameterValue(current, Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.25f)
        + readStateParameterValue(current, Parameters::ID::fxBitcrushMix, 0.0f)
        + readStateParameterValue(current, Parameters::ID::macroDirt, 0.0f);
    addDirection(candidateDirt, currentDirt, 0.20f, "dirtier", "cleaner");

    const auto candidateMotion = readStateParameterValue(candidate.state, Parameters::ID::macroMotion, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::lfo1Depth, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::lfo2Depth, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::fxPumpDepth, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::sequencerRandomAmount, 0.0f);
    const auto currentMotion = readStateParameterValue(current, Parameters::ID::macroMotion, 0.0f)
        + readStateParameterValue(current, Parameters::ID::lfo1Depth, 0.0f)
        + readStateParameterValue(current, Parameters::ID::lfo2Depth, 0.0f)
        + readStateParameterValue(current, Parameters::ID::fxPumpDepth, 0.0f)
        + readStateParameterValue(current, Parameters::ID::sequencerRandomAmount, 0.0f);
    addDirection(candidateMotion, currentMotion, 0.25f, "more motion", "less motion");

    const auto candidateSpace = readStateParameterValue(candidate.state, Parameters::ID::macroSpace, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::macroThrow, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::fxDelayMix, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::fxReverbMix, 0.0f);
    const auto currentSpace = readStateParameterValue(current, Parameters::ID::macroSpace, 0.0f)
        + readStateParameterValue(current, Parameters::ID::macroThrow, 0.0f)
        + readStateParameterValue(current, Parameters::ID::fxDelayMix, 0.0f)
        + readStateParameterValue(current, Parameters::ID::fxReverbMix, 0.0f);
    addDirection(candidateSpace, currentSpace, 0.20f, "more space", "drier");

    const auto candidateWidth = readStateParameterValue(candidate.state, Parameters::ID::fxWidthAmount, 1.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::unisonSpread, 0.0f)
        + readStateParameterValue(candidate.state, Parameters::ID::macroBounce, 0.0f);
    const auto currentWidth = readStateParameterValue(current, Parameters::ID::fxWidthAmount, 1.0f)
        + readStateParameterValue(current, Parameters::ID::unisonSpread, 0.0f)
        + readStateParameterValue(current, Parameters::ID::macroBounce, 0.0f);
    addDirection(candidateWidth, currentWidth, 0.18f, "wider", "narrower");

    addDirection(readStateParameterValue(candidate.state, Parameters::ID::outputGain, 0.8f),
                 readStateParameterValue(current, Parameters::ID::outputGain, 0.8f),
                 0.08f,
                 "hotter",
                 "safer level");

    while (changes.size() > 4)
        changes.remove(changes.size() - 1);

    return changes.isEmpty() ? juce::String("similar") : changes.joinIntoString(", ");
}

juce::String NateVSTAudioProcessor::getRandomCandidateChangedSectionsSummary(int slotIndex)
{
    const auto sections = getRandomCandidateChangedSections(slotIndex);
    return sections.isEmpty() ? juce::String("No section changes") : sections.joinIntoString(", ");
}

int NateVSTAudioProcessor::getRandomCandidateChangedSectionCount(int slotIndex)
{
    return getRandomCandidateChangedSections(slotIndex).size();
}

juce::String NateVSTAudioProcessor::getRandomCandidateDiffSummary(int slotIndex)
{
    if (! hasRandomCandidate(slotIndex))
        return {};

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    const auto current = createPluginState(false);
    juce::StringArray diffs;

    auto formatPercent = [] (float value)
    {
        return juce::String(juce::roundToInt(juce::jlimit(0.0f, 1.0f, value) * 100.0f)) + "%";
    };

    auto formatHz = [] (float value)
    {
        if (value >= 1000.0f)
            return juce::String(value / 1000.0f, 1) + "k";

        return juce::String(juce::roundToInt(value));
    };

    auto formatDb = [] (float value)
    {
        return juce::String(value, 1) + "dB";
    };

    auto formatSemitone = [] (float value)
    {
        const auto rounded = juce::roundToInt(value);
        return (rounded >= 0 ? juce::String("+") : juce::String()) + juce::String(rounded) + "st";
    };

    auto addDiff = [&] (const juce::String& label,
                        const char* parameterID,
                        float fallback,
                        float threshold,
                        auto formatValue)
    {
        if (diffs.size() >= 5)
            return;

        const auto currentValue = readStateParameterValue(current, parameterID, fallback);
        const auto candidateValue = readStateParameterValue(candidate.state, parameterID, fallback);
        if (std::abs(candidateValue - currentValue) <= threshold)
            return;

        diffs.add(label + " " + formatValue(currentValue) + "->" + formatValue(candidateValue));
    };

    const auto currentCutoff = readStateParameterValue(current, Parameters::ID::filterCutoff, 1000.0f);
    const auto candidateCutoff = readStateParameterValue(candidate.state, Parameters::ID::filterCutoff, 1000.0f);
    if (std::abs(candidateCutoff - currentCutoff) > juce::jmax(120.0f, currentCutoff * 0.08f))
        diffs.add("Cutoff " + formatHz(currentCutoff) + "->" + formatHz(candidateCutoff));

    addDiff("Drive", Parameters::ID::driveAmount, 0.0f, 0.05f, formatPercent);
    addDiff("FX drive", Parameters::ID::fxDistortionAmount, 0.0f, 0.05f, formatPercent);
    addDiff("Bass safe", Parameters::ID::fxDistortionBassSafe, 0.0f, 0.08f, formatPercent);
    addDiff("Glue", Parameters::ID::fxGuardGlue, 0.0f, 0.05f, formatPercent);
    addDiff("Punch", Parameters::ID::fxGuardPunch, 0.0f, 0.05f, formatPercent);
    addDiff("Clip", Parameters::ID::fxGuardClipMix, 1.0f, 0.08f, formatPercent);
    addDiff("Motion", Parameters::ID::macroMotion, 0.0f, 0.08f, formatPercent);
    addDiff("LFO", Parameters::ID::lfo1Depth, 0.0f, 0.08f, formatPercent);
    addDiff("Delay", Parameters::ID::fxDelayMix, 0.0f, 0.05f, formatPercent);
    addDiff("Reverb", Parameters::ID::fxReverbMix, 0.0f, 0.05f, formatPercent);
    addDiff("Delay send", Parameters::ID::fxSendDelay, 0.0f, 0.05f, formatPercent);
    addDiff("Reverb send", Parameters::ID::fxSendReverb, 0.0f, 0.05f, formatPercent);
    addDiff("Width", Parameters::ID::fxWidthAmount, 1.0f, 0.08f, formatPercent);
    addDiff("Output", Parameters::ID::outputGain, 0.8f, 0.05f, formatPercent);

    if (diffs.size() < 5)
    {
        const auto currentStart = readStateParameterValue(current, Parameters::ID::sampleStart, 0.0f);
        const auto currentEnd = readStateParameterValue(current, Parameters::ID::sampleEnd, 1.0f);
        const auto candidateStart = readStateParameterValue(candidate.state, Parameters::ID::sampleStart, 0.0f);
        const auto candidateEnd = readStateParameterValue(candidate.state, Parameters::ID::sampleEnd, 1.0f);
        if (std::abs(candidateStart - currentStart) > 0.03f || std::abs(candidateEnd - currentEnd) > 0.03f)
        {
            diffs.add("Sample "
                      + formatPercent(currentStart) + "-" + formatPercent(currentEnd)
                      + "->" + formatPercent(candidateStart) + "-" + formatPercent(candidateEnd));
        }
    }

    addDiff("Pitch", Parameters::ID::sampleTranspose, 0.0f, 0.5f, formatSemitone);
    addDiff("Gain", Parameters::ID::sampleGain, -6.0f, 0.75f, formatDb);

    if (diffs.size() < 5)
    {
        auto enabledSteps = [] (const juce::ValueTree& state)
        {
            auto count = 0;
            for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
                if (static_cast<bool>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_enabled", false)))
                    ++count;

            return count;
        };

        const auto currentSteps = enabledSteps(current);
        const auto candidateSteps = enabledSteps(candidate.state);
        if (currentSteps != candidateSteps)
            diffs.add("Seq " + juce::String(currentSteps) + "->" + juce::String(candidateSteps) + " steps");
    }

    while (diffs.size() > 5)
        diffs.remove(diffs.size() - 1);

    return diffs.isEmpty() ? juce::String("no major value diff") : diffs.joinIntoString(", ");
}

juce::String NateVSTAudioProcessor::getRandomCandidateValidationSummary(int slotIndex) const
{
    if (! hasRandomCandidate(slotIndex))
        return {};

    return randomCandidateSnapshots[static_cast<size_t>(slotIndex)].validationSummary;
}

juce::String NateVSTAudioProcessor::getLastRandomValidationSummary() const
{
    return lastRandomValidationSummary;
}

int NateVSTAudioProcessor::getActiveRandomCandidateIndex() const noexcept
{
    return activeRandomCandidateSlot;
}

bool NateVSTAudioProcessor::recallRandomCandidate(int slotIndex)
{
    if (! hasRandomCandidate(slotIndex))
        return false;

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    randomUndoState = createPluginState(false);
    randomUndoLabel = "Recall " + juce::String(slotIndex + 1);
    hasRandomUndoState = true;
    randomRedoState = {};
    randomRedoLabel.clear();
    hasRandomRedoState = false;

    restorePluginState(candidate.state.createCopy(), false);
    activeRandomCandidateSlot = slotIndex;
    return true;
}

bool NateVSTAudioProcessor::beginRandomCandidateAudition(int slotIndex)
{
    if (! hasRandomCandidate(slotIndex))
        return false;

    endRandomCandidateAudition();

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    randomCandidateAuditionReturnState = createPluginState(false);
    restorePluginState(candidate.state.createCopy(), false);
    activeRandomCandidateSlot = slotIndex;
    auditioningRandomCandidateSlot = slotIndex;
    return true;
}

bool NateVSTAudioProcessor::endRandomCandidateAudition()
{
    if (auditioningRandomCandidateSlot < 0 || ! randomCandidateAuditionReturnState.isValid())
        return false;

    restorePluginState(randomCandidateAuditionReturnState.createCopy(), false);
    randomCandidateAuditionReturnState = {};
    auditioningRandomCandidateSlot = -1;
    return true;
}

bool NateVSTAudioProcessor::promoteRandomCandidateToPerformanceSnapshot(int candidateSlotIndex, int snapshotSlotIndex)
{
    if (! hasRandomCandidate(candidateSlotIndex)
        || snapshotSlotIndex < 0
        || snapshotSlotIndex >= static_cast<int>(performanceSnapshots.size()))
        return false;

    performanceSnapshots[static_cast<size_t>(snapshotSlotIndex)] =
        randomCandidateSnapshots[static_cast<size_t>(candidateSlotIndex)].state.createCopy();
    return true;
}

bool NateVSTAudioProcessor::loadSampleFile(const juce::File& file)
{
    sampleCaptureEnabled.store(false, std::memory_order_relaxed);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_relaxed);
    invalidateSampleCaptureSession();
    sampleCaptureTargetSamples.store(0, std::memory_order_relaxed);
    sampleCapturePreRollWritePosition.store(0, std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_relaxed);
    if (! samplePlayer.loadFile(file))
        return false;

    resetSampleParametersForNewSource(file.getFullPathName());
    return true;
}

void NateVSTAudioProcessor::clearSample()
{
    sampleCaptureEnabled.store(false, std::memory_order_relaxed);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_relaxed);
    invalidateSampleCaptureSession();
    sampleCaptureTargetSamples.store(0, std::memory_order_relaxed);
    sampleCapturePreRollWritePosition.store(0, std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_relaxed);
    loadedSamplePath.clear();
    samplePlayer.clear();
    setParameterPlainValue(Parameters::ID::sampleEnabled, 0.0f);
}

bool NateVSTAudioProcessor::hasLoadedSample() const
{
    return samplePlayer.hasSample();
}

bool NateVSTAudioProcessor::hasMissingSampleReference() const
{
    return loadedSamplePath.isNotEmpty() && ! samplePlayer.hasSample();
}

void NateVSTAudioProcessor::beginSampleCapture()
{
    sampleCaptureEnabled.store(false, std::memory_order_release);
    invalidateSampleCaptureSession();
    waitForSampleCaptureWritersToFinish();

    const auto nextCaptureBufferIndex = getNextSampleCaptureBufferIndex();
    auto& captureBuffer = sampleCaptureBuffers[static_cast<size_t>(nextCaptureBufferIndex)];
    if (captureBuffer.getNumSamples() <= 0 || captureBuffer.getNumChannels() <= 0)
        return;

    captureBuffer.clear();
    sampleCapturePreRollBuffer.clear();
    activeSampleCaptureBufferIndex.store(nextCaptureBufferIndex, std::memory_order_release);
    sampleCaptureWritePosition.store(0, std::memory_order_relaxed);
    sampleCaptureSamplesRecorded.store(0, std::memory_order_relaxed);
    sampleCaptureSourcePeak.store(0.0f, std::memory_order_relaxed);
    sampleCaptureTargetSamples.store(calculateSampleCaptureTargetSamples(), std::memory_order_release);
    sampleCapturePreRollWritePosition.store(0, std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_relaxed);
    sampleCaptureWaitingForThreshold.store(getSampleCaptureStartModeIndex() > 0, std::memory_order_release);
    sampleCaptureEnabled.store(true, std::memory_order_release);
}

void NateVSTAudioProcessor::stopSampleCapture()
{
    sampleCaptureEnabled.store(false, std::memory_order_release);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_release);
    invalidateSampleCaptureSession();
    sampleCapturePreRollWritePosition.store(0, std::memory_order_release);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_release);
}

bool NateVSTAudioProcessor::isSampleCaptureEnabled() const noexcept
{
    return sampleCaptureEnabled.load(std::memory_order_acquire);
}

bool NateVSTAudioProcessor::isSampleCaptureWaitingForThreshold() const noexcept
{
    return sampleCaptureWaitingForThreshold.load(std::memory_order_acquire);
}

float NateVSTAudioProcessor::getSampleCaptureDurationSeconds() const noexcept
{
    const auto sampleRate = sampleCaptureSampleRate > 0.0 ? sampleCaptureSampleRate : 44100.0;
    return static_cast<float>(sampleCaptureSamplesRecorded.load(std::memory_order_acquire)) / static_cast<float>(sampleRate);
}

float NateVSTAudioProcessor::getSampleCaptureCapacitySeconds() const noexcept
{
    const auto sampleRate = sampleCaptureSampleRate > 0.0 ? sampleCaptureSampleRate : 44100.0;
    return static_cast<float>(getActiveSampleCaptureBuffer().getNumSamples()) / static_cast<float>(sampleRate);
}

int NateVSTAudioProcessor::getSampleCaptureSourceIndex() const noexcept
{
    const auto sourceValue = sampleRecordSource != nullptr
        ? sampleRecordSource->load(std::memory_order_relaxed)
        : 0.0f;
    return juce::jlimit(0, 1, juce::roundToInt(sourceValue));
}

int NateVSTAudioProcessor::getSampleCaptureStartModeIndex() const noexcept
{
    const auto startValue = sampleRecordStart != nullptr
        ? sampleRecordStart->load(std::memory_order_relaxed)
        : 0.0f;
    return juce::jlimit(0, 3, juce::roundToInt(startValue));
}

int NateVSTAudioProcessor::getSampleCaptureLengthModeIndex() const noexcept
{
    const auto lengthValue = sampleRecordLength != nullptr
        ? sampleRecordLength->load(std::memory_order_relaxed)
        : 0.0f;
    return juce::jlimit(0, 4, juce::roundToInt(lengthValue));
}

int NateVSTAudioProcessor::getSampleCapturePreRollModeIndex() const noexcept
{
    const auto preRollValue = sampleRecordPreRoll != nullptr
        ? sampleRecordPreRoll->load(std::memory_order_relaxed)
        : 0.0f;
    return juce::jlimit(0, 3, juce::roundToInt(preRollValue));
}

float NateVSTAudioProcessor::getSampleCaptureThresholdDb() const noexcept
{
    switch (getSampleCaptureStartModeIndex())
    {
        case 1: return -36.0f;
        case 2: return -24.0f;
        case 3: return -12.0f;
        default: break;
    }

    return -96.0f;
}

float NateVSTAudioProcessor::getSampleCaptureSourcePeak() const noexcept
{
    return sampleCaptureSourcePeak.load(std::memory_order_acquire);
}

float NateVSTAudioProcessor::getSampleCaptureTargetDurationSeconds() const noexcept
{
    const auto targetSamples = sampleCaptureTargetSamples.load(std::memory_order_acquire);
    if (targetSamples <= 0)
        return 0.0f;

    const auto sampleRate = sampleCaptureSampleRate > 0.0 ? sampleCaptureSampleRate : 44100.0;
    return static_cast<float>(targetSamples) / static_cast<float>(sampleRate);
}

float NateVSTAudioProcessor::getSampleCapturePreRollDurationSeconds() const noexcept
{
    switch (getSampleCapturePreRollModeIndex())
    {
        case 1: return 0.1f;
        case 2: return 0.25f;
        case 3: return 0.5f;
        default: break;
    }

    return 0.0f;
}

juce::String NateVSTAudioProcessor::getSampleCaptureSourceName() const
{
    const auto choices = Parameters::sampleRecordSourceChoices();
    const auto sourceIndex = getSampleCaptureSourceIndex();
    return juce::isPositiveAndBelow(sourceIndex, choices.size()) ? choices[sourceIndex]
                                                                 : juce::String("Post-FX Output");
}

juce::String NateVSTAudioProcessor::getSampleCaptureStartModeName() const
{
    const auto choices = Parameters::sampleRecordStartChoices();
    const auto startIndex = getSampleCaptureStartModeIndex();
    return juce::isPositiveAndBelow(startIndex, choices.size()) ? choices[startIndex]
                                                                : juce::String("Immediate");
}

juce::String NateVSTAudioProcessor::getSampleCaptureLengthModeName() const
{
    const auto choices = Parameters::sampleRecordLengthChoices();
    const auto lengthIndex = getSampleCaptureLengthModeIndex();
    return juce::isPositiveAndBelow(lengthIndex, choices.size()) ? choices[lengthIndex]
                                                                 : juce::String("Free");
}

juce::String NateVSTAudioProcessor::getSampleCapturePreRollModeName() const
{
    const auto choices = Parameters::sampleRecordPreRollChoices();
    const auto preRollIndex = getSampleCapturePreRollModeIndex();
    return juce::isPositiveAndBelow(preRollIndex, choices.size()) ? choices[preRollIndex]
                                                                  : juce::String("Pre Off");
}

bool NateVSTAudioProcessor::commitSampleCaptureToSampler()
{
    sampleCaptureEnabled.store(false, std::memory_order_release);
    sampleCaptureWaitingForThreshold.store(false, std::memory_order_release);
    invalidateSampleCaptureSession();
    sampleCapturePreRollWritePosition.store(0, std::memory_order_release);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_release);
    waitForSampleCaptureWritersToFinish();

    const auto& captureBuffer = getActiveSampleCaptureBuffer();
    const auto availableSamples = juce::jlimit(0,
                                               captureBuffer.getNumSamples(),
                                               sampleCaptureSamplesRecorded.load(std::memory_order_acquire));
    if (availableSamples < 64 || captureBuffer.getNumChannels() <= 0)
        return false;

    const auto captureChannels = juce::jlimit(1, 2, captureBuffer.getNumChannels());
    const auto maxSamples = captureBuffer.getNumSamples();
    const auto writePosition = juce::jlimit(0,
                                            juce::jmax(0, maxSamples - 1),
                                            sampleCaptureWritePosition.load(std::memory_order_acquire));
    auto sourceStart = writePosition - availableSamples;
    while (sourceStart < 0)
        sourceStart += maxSamples;

    juce::AudioBuffer<float> captured(captureChannels, availableSamples);
    captured.clear();

    const auto firstSpan = juce::jmin(availableSamples, maxSamples - sourceStart);
    const auto secondSpan = availableSamples - firstSpan;
    for (auto channel = 0; channel < captureChannels; ++channel)
    {
        captured.copyFrom(channel, 0, captureBuffer, channel, sourceStart, firstSpan);
        if (secondSpan > 0)
            captured.copyFrom(channel, firstSpan, captureBuffer, channel, 0, secondSpan);
    }

    const auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
    const auto captureName = "Recorded Snippet " + timestamp;
    const auto capturesDirectory = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Nate VST Captures");
    const auto captureFile = capturesDirectory.getNonexistentChildFile(captureName, ".wav", false);

    if (writePresetPreviewFile(captureFile, captured, sampleCaptureSampleRate)
        && samplePlayer.loadFile(captureFile))
    {
        resetSampleParametersForNewSource(captureFile.getFullPathName());
        rememberSampleCaptureTake(captureFile);
        autoTrimSampleToContent();
        return true;
    }

    if (! samplePlayer.loadBuffer(captured, sampleCaptureSampleRate, captureName))
        return false;

    resetSampleParametersForNewSource({});
    autoTrimSampleToContent();
    return true;
}

int NateVSTAudioProcessor::getSampleCaptureTakeCount() const
{
    const juce::ScopedLock lock(sampleCaptureTakeLock);
    return static_cast<int>(std::count_if(sampleCaptureTakeFiles.begin(),
                                          sampleCaptureTakeFiles.end(),
                                          [] (const juce::File& takeFile)
                                          {
                                              return takeFile.existsAsFile();
                                          }));
}

juce::StringArray NateVSTAudioProcessor::getSampleCaptureTakeNames(int limit) const
{
    juce::StringArray takeNames;
    const auto safeLimit = juce::jlimit(0, sampleCaptureTakeHistoryLimit, limit);
    if (safeLimit <= 0)
        return takeNames;

    const juce::ScopedLock lock(sampleCaptureTakeLock);
    for (const auto& takeFile : sampleCaptureTakeFiles)
    {
        if (! takeFile.existsAsFile())
            continue;

        takeNames.add(takeFile.getFileNameWithoutExtension());
        if (takeNames.size() >= safeLimit)
            break;
    }

    return takeNames;
}

int NateVSTAudioProcessor::getSelectedSampleCaptureTakeIndex() const
{
    const juce::ScopedLock lock(sampleCaptureTakeLock);
    auto validIndex = 0;
    for (const auto& takeFile : sampleCaptureTakeFiles)
    {
        if (! takeFile.existsAsFile())
            continue;

        if (selectedSampleCaptureTakePath.isNotEmpty()
            && takeFile.getFullPathName() == selectedSampleCaptureTakePath)
        {
            return validIndex;
        }

        ++validIndex;
    }

    return 0;
}

bool NateVSTAudioProcessor::selectSampleCaptureTake(int takeIndex)
{
    const auto safeIndex = juce::jmax(0, takeIndex);
    const juce::ScopedLock lock(sampleCaptureTakeLock);
    auto validIndex = 0;
    for (const auto& takeFile : sampleCaptureTakeFiles)
    {
        if (! takeFile.existsAsFile())
            continue;

        if (validIndex == safeIndex)
        {
            selectedSampleCaptureTakePath = takeFile.getFullPathName();
            return true;
        }

        ++validIndex;
    }

    return false;
}

juce::String NateVSTAudioProcessor::getSelectedSampleCaptureTakePath() const
{
    const juce::ScopedLock lock(sampleCaptureTakeLock);

    if (selectedSampleCaptureTakePath.isNotEmpty())
    {
        const auto selectedFile = juce::File(selectedSampleCaptureTakePath);
        if (selectedFile.existsAsFile())
            return selectedFile.getFullPathName();
    }

    for (const auto& takeFile : sampleCaptureTakeFiles)
        if (takeFile.existsAsFile())
            return takeFile.getFullPathName();

    return {};
}

juce::String NateVSTAudioProcessor::getLatestSampleCaptureTakePath() const
{
    const juce::ScopedLock lock(sampleCaptureTakeLock);
    for (const auto& takeFile : sampleCaptureTakeFiles)
        if (takeFile.existsAsFile())
            return takeFile.getFullPathName();

    return {};
}

juce::AudioBuffer<float>& NateVSTAudioProcessor::getActiveSampleCaptureBuffer() noexcept
{
    const auto activeIndex = juce::jlimit(0,
                                         static_cast<int>(sampleCaptureBufferBankSize) - 1,
                                         activeSampleCaptureBufferIndex.load(std::memory_order_acquire));
    return sampleCaptureBuffers[static_cast<size_t>(activeIndex)];
}

const juce::AudioBuffer<float>& NateVSTAudioProcessor::getActiveSampleCaptureBuffer() const noexcept
{
    const auto activeIndex = juce::jlimit(0,
                                         static_cast<int>(sampleCaptureBufferBankSize) - 1,
                                         activeSampleCaptureBufferIndex.load(std::memory_order_acquire));
    return sampleCaptureBuffers[static_cast<size_t>(activeIndex)];
}

int NateVSTAudioProcessor::getNextSampleCaptureBufferIndex() const noexcept
{
    const auto activeIndex = juce::jlimit(0,
                                         static_cast<int>(sampleCaptureBufferBankSize) - 1,
                                         activeSampleCaptureBufferIndex.load(std::memory_order_acquire));
    return (activeIndex + 1) % static_cast<int>(sampleCaptureBufferBankSize);
}

void NateVSTAudioProcessor::invalidateSampleCaptureSession() noexcept
{
    sampleCaptureSessionSerial.fetch_add(1, std::memory_order_acq_rel);
}

void NateVSTAudioProcessor::waitForSampleCaptureWritersToFinish()
{
    auto spins = 0;
    while (sampleCaptureActiveWriters.load(std::memory_order_acquire) > 0)
    {
        if (++spins < 64)
            juce::Thread::yield();
        else
            juce::Thread::sleep(1);
    }
}

void NateVSTAudioProcessor::rememberSampleCaptureTake(const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    const juce::ScopedLock lock(sampleCaptureTakeLock);
    const auto fullPath = file.getFullPathName();
    sampleCaptureTakeFiles.erase(std::remove_if(sampleCaptureTakeFiles.begin(),
                                                sampleCaptureTakeFiles.end(),
                                                [&fullPath] (const juce::File& candidate)
                                                {
                                                    return candidate.getFullPathName() == fullPath;
                                                }),
                                 sampleCaptureTakeFiles.end());
    sampleCaptureTakeFiles.insert(sampleCaptureTakeFiles.begin(), file);
    selectedSampleCaptureTakePath = fullPath;

    if (sampleCaptureTakeFiles.size() > sampleCaptureTakeHistoryLimit)
        sampleCaptureTakeFiles.resize(sampleCaptureTakeHistoryLimit);
}

bool NateVSTAudioProcessor::autoTrimSampleToContent()
{
    if (! samplePlayer.hasSample())
        return false;

    const auto range = samplePlayer.findContentRange(0.006f, 12.0);
    if (! range.has_value())
        return false;

    if (range->end - range->start < 0.002f)
        return false;

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, juce::jlimit(0.0f, 1.0f, range->start));
    setParameterPlainValue(Parameters::ID::sampleEnd, juce::jlimit(0.0f, 1.0f, range->end));
    return true;
}

bool NateVSTAudioProcessor::spliceSampleToSlices()
{
    if (! samplePlayer.hasSample())
        return false;

    const auto detected = detectSampleTransientSlices();
    if (detected >= 0)
        return true;

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 2.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleEnd, 1.0f);

    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        const auto equalEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        setParameterPlainValue(Parameters::ID::sampleSliceCustom[index], 1.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceStart[index], equalStart);
        setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], equalEnd);
    }

    return true;
}

bool NateVSTAudioProcessor::randomizeRecordedSample()
{
    if (isRandomLockEnabled(Parameters::ID::randomLockSample) || ! samplePlayer.hasSample())
        return false;

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_real_distribution<float> gainDistribution(-10.0f, -2.0f);
    std::uniform_real_distribution<float> panDistribution(-0.55f, 0.55f);
    std::uniform_real_distribution<float> probabilityDistribution(0.62f, 1.0f);
    std::uniform_real_distribution<float> nudgeDistribution(-3.0f, 3.0f);
    std::uniform_real_distribution<float> fadeDistribution(0.0f, 0.55f);
    std::uniform_int_distribution<int> pitchDistribution(0, 8);
    std::uniform_int_distribution<int> stutterRepeatsDistribution(2, 6);
    std::uniform_int_distribution<int> sliceStyleDistribution(0, 4);
    std::uniform_int_distribution<int> engineModeDistribution(0, 3);
    constexpr std::array<float, 9> pitchChoices { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 5.0f, 7.0f, 10.0f, 12.0f };

    autoTrimSampleToContent();
    spliceSampleToSlices();

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 2.0f);
    setParameterPlainValue(Parameters::ID::sampleEngineMode, static_cast<float>(engineModeDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleGrainSize, 0.025f + (chance(sampleRandomEngine) * 0.16f));
    setParameterPlainValue(Parameters::ID::sampleGrainSpray, chance(sampleRandomEngine) * 0.88f);
    setParameterPlainValue(Parameters::ID::sampleSpectralFreeze, chance(sampleRandomEngine) < 0.38f ? 0.25f + (chance(sampleRandomEngine) * 0.68f) : 0.0f);
    setParameterPlainValue(Parameters::ID::sampleSliceStyle, static_cast<float>(sliceStyleDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleReverse, chance(sampleRandomEngine) < 0.16f ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterEnabled, chance(sampleRandomEngine) < 0.58f ? 1.0f : 0.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterRate, chance(sampleRandomEngine) < 0.62f ? 1.0f : 2.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterRepeats, static_cast<float>(stutterRepeatsDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleTranspose, pitchChoices[static_cast<size_t>(pitchDistribution(sampleRandomEngine))] * 0.5f);
    setParameterPlainValue(Parameters::ID::samplePitchRamp, pitchChoices[static_cast<size_t>(pitchDistribution(sampleRandomEngine))]);
    setParameterPlainValue(Parameters::ID::sampleGain, gainDistribution(sampleRandomEngine));
    setParameterPlainValue(Parameters::ID::sampleMix, 0.72f + (chance(sampleRandomEngine) * 0.26f));

    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        setParameterPlainValue(Parameters::ID::sampleSliceCustom[index], 1.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceReverse[index], chance(sampleRandomEngine) < 0.20f ? 1.0f : 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceTranspose[index], pitchChoices[static_cast<size_t>(pitchDistribution(sampleRandomEngine))]);
        setParameterPlainValue(Parameters::ID::sampleSliceGain[index], gainDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::sampleSlicePan[index], panDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::sampleSliceProbability[index], probabilityDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::sampleSliceStutter[index], chance(sampleRandomEngine) < 0.34f ? 1.0f : 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceChoke[index], index % 2 == 0 ? 1.0f : (chance(sampleRandomEngine) < 0.35f ? 1.0f : 0.0f));
        setParameterPlainValue(Parameters::ID::sampleSliceStutterRepeats[index], static_cast<float>(stutterRepeatsDistribution(sampleRandomEngine)));
        setParameterPlainValue(Parameters::ID::sampleSliceNudge[index], nudgeDistribution(sampleRandomEngine));
        setParameterPlainValue(Parameters::ID::sampleSliceFade[index], fadeDistribution(sampleRandomEngine));
    }

    if (! isRandomLockEnabled(Parameters::ID::randomLockFx))
    {
        setParameterPlainValue(Parameters::ID::fxPumpEnabled, 1.0f);
        setParameterPlainValue(Parameters::ID::fxPumpDepth, 0.18f + chance(sampleRandomEngine) * 0.24f);
        setParameterPlainValue(Parameters::ID::fxDelayEnabled, 1.0f);
        setParameterPlainValue(Parameters::ID::fxDelaySync, 1.0f);
        setParameterPlainValue(Parameters::ID::fxDelayRate, chance(sampleRandomEngine) < 0.5f ? 2.0f : 3.0f);
        setParameterPlainValue(Parameters::ID::fxDelayMix, 0.06f + chance(sampleRandomEngine) * 0.14f);
        setParameterPlainValue(Parameters::ID::fxGuardEnabled, 1.0f);
    }

    return true;
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

int NateVSTAudioProcessor::detectSampleTransientSlices()
{
    if (! samplePlayer.hasSample())
        return -1;

    const auto detection = samplePlayer.detectTransientSliceRegions();
    if (! detection.valid)
        return -1;

    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 2.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, detection.regions[0].start);
    setParameterPlainValue(Parameters::ID::sampleEnd, detection.regions[0].end);

    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto& region = detection.regions[index];
        auto orderedStart = juce::jlimit(0.0f, 1.0f, juce::jmin(region.start, region.end));
        auto orderedEnd = juce::jlimit(0.0f, 1.0f, juce::jmax(region.start, region.end));
        if (orderedEnd - orderedStart < 0.002f)
            orderedStart = juce::jmax(0.0f, orderedEnd - 0.002f);

        setParameterPlainValue(Parameters::ID::sampleSliceCustom[index], 1.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceStart[index], orderedStart);
        setParameterPlainValue(Parameters::ID::sampleSliceEnd[index], orderedEnd);
    }

    return detection.transientCount;
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
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_int_distribution<int> pitchDistribution(0, 4);
    std::uniform_int_distribution<int> rampDistribution(0, 5);
    std::uniform_int_distribution<int> stutterRateDistribution(1, 2);
    std::uniform_int_distribution<int> stutterRepeatsDistribution(2, 5);
    std::uniform_int_distribution<int> engineModeDistribution(0, 3);
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
    setParameterPlainValue(Parameters::ID::sampleEngineMode, static_cast<float>(engineModeDistribution(sampleRandomEngine)));
    setParameterPlainValue(Parameters::ID::sampleGrainSize, 0.018f + (chance(sampleRandomEngine) * 0.18f));
    setParameterPlainValue(Parameters::ID::sampleGrainSpray, chance(sampleRandomEngine) * 0.78f);
    setParameterPlainValue(Parameters::ID::sampleSpectralFreeze, chance(sampleRandomEngine) < 0.34f ? 0.18f + (chance(sampleRandomEngine) * 0.72f) : 0.0f);
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

juce::String NateVSTAudioProcessor::getLoadedSamplePath() const
{
    return loadedSamplePath;
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
    if (step.enabled)
        setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);

    if (isSequencerSceneChainPlaybackEnabled() && getSequencerSceneChainPlaybackLength() <= 0)
        refreshSequencerSceneChainPlayback();
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
            {
                step.timing = juce::jlimit(0.0f, 1.0f, step.timing + smallMove(sampleRandomEngine) * amount * 0.18f);
                if (chance(sampleRandomEngine) < amount * 0.12f)
                    step.ratchet = step.ratchet <= 1 ? 2 : 1 + std::uniform_int_distribution<int>(0, Sequencer::PatternSequencer::maxRatchet - 1)(sampleRandomEngine);
            }

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
            step.ratchet = chance(sampleRandomEngine) < amount * 0.14f ? 2 : 1;
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

    sequencerSceneChainPlaybackEnabled.store(static_cast<bool>(sequencerUndoState.getProperty(sequencerSceneChainPlaybackProperty, false)) ? 1 : 0,
                                             std::memory_order_release);
    sequencerSceneChainClipBars.store(sanitiseSequencerSceneChainClipBars(static_cast<int>(sequencerUndoState.getProperty(sequencerSceneChainClipBarsProperty, 0))),
                                      std::memory_order_release);
    restoreSequencerScenesFromState(sequencerUndoState);
    restoreSequencerFromState(sequencerUndoState);
    refreshSequencerSceneChainPlayback();
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

    auto setStep = [this] (int index, int noteOffset, float velocity, float probability, float timing = 0.0f, float length = 1.0f, float lock = 0.0f, int ratchet = 1, int condition = 0, bool slide = false)
    {
        Sequencer::Step step;
        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = velocity;
        step.probability = probability;
        step.timing = timing;
        step.length = length;
        step.lock = lock;
        step.ratchet = ratchet;
        step.condition = condition;
        step.slide = slide;
        patternSequencer.setStep(index, step);
    };

    switch (presetIndex)
    {
        case 14:
            setStep(0, 0, 0.92f, 1.0f, 0.0f, 0.72f, 0.08f);
            setStep(3, 0, 0.54f, 0.7f, 0.42f, 0.42f, 0.22f);
            setStep(5, -2, 0.74f, 0.86f, 0.34f, 0.48f, 0.34f);
            setStep(7, 3, 0.58f, 0.68f, 0.58f, 0.36f, 0.42f, 2);
            setStep(8, 0, 0.88f, 1.0f, 0.0f, 0.68f, 0.1f);
            setStep(11, 5, 0.66f, 0.76f, 0.26f, 0.44f, 0.3f);
            setStep(13, 7, 0.76f, 0.9f, 0.48f, 0.52f, 0.38f);
            setStep(15, -5, 0.56f, 0.64f, 0.7f, 0.32f, 0.46f, 3);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.32f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.22f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.74f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.92f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.34f);
            break;

        case 13:
            setStep(2, 0, 0.86f, 1.0f, 0.12f, 0.58f, 0.12f);
            setStep(6, -5, 0.8f, 0.98f, 0.18f, 0.52f, 0.24f);
            setStep(10, 0, 0.88f, 1.0f, 0.08f, 0.58f, 0.16f);
            setStep(14, 7, 0.76f, 0.9f, 0.28f, 0.46f, 0.32f, 2);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.34f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.18f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.68f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.98f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.22f);
            break;

        case 12:
            setStep(2, 0, 0.78f, 1.0f, 0.18f, 0.74f, 0.32f);
            setStep(6, 5, 0.86f, 0.94f, 0.24f, 0.68f, 0.44f);
            setStep(10, -2, 0.7f, 0.88f, 0.14f, 0.6f, 0.28f);
            setStep(14, 7, 0.82f, 0.86f, 0.34f, 0.58f, 0.5f, 2);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.3f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 10.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.48f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.9f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.36f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 6.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.38f);
            break;

        case 11:
            setStep(0, 0, 0.82f, 1.0f, 0.0f, 0.76f, 0.14f);
            setStep(4, 7, 0.74f, 0.96f, 0.0f, 0.66f, 0.24f);
            setStep(7, 4, 0.58f, 0.72f, 0.46f, 0.42f, 0.34f);
            setStep(10, 9, 0.78f, 0.88f, 0.18f, 0.58f, 0.42f);
            setStep(14, 2, 0.66f, 0.78f, 0.38f, 0.48f, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate, 0.4f);
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerScale, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 9.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.16f);
            setParameterPlainValue(Parameters::ID::sequencerAccent, 0.54f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerProbability, 0.94f);
            setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.32f);
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.28f);
            break;

        case 10:
            setStep(0, 0, 0.94f, 1.0f);
            setStep(2, 0, 0.52f, 0.72f, 0.08f, 1.0f, 0.24f);
            setStep(4, 0, 0.86f, 1.0f);
            setStep(6, 7, 0.5f, 0.68f, 0.08f, 1.0f, 0.38f);
            setStep(8, 0, 0.9f, 1.0f);
            setStep(10, -5, 0.54f, 0.74f, 0.08f, 1.0f, 0.28f);
            setStep(12, 0, 0.88f, 1.0f);
            setStep(14, 3, 0.5f, 0.68f, 0.08f, 1.0f, 0.42f, 2);
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
            setStep(15, 10, 0.5f, 0.58f, 0.72f, 1.0f, 0.48f, 2);
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

    if (! hasEnabledStep && transformIndex != sequencerChordStabPaintTransformIndex)
        return false;

    captureSequencerUndoState();

    if (transformIndex == sequencerFourBarChainTransformIndex)
    {
        setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
        buildSequencerFourBarSceneChain();
        setSequencerSceneChainPlaybackEnabled(true);
        return true;
    }

    if (transformIndex == sequencerChordStabPaintTransformIndex)
    {
        patternSequencer.clear();

        auto paintStab = [this] (int index,
                                 int noteOffset,
                                 float velocity,
                                 float probability,
                                 float timing,
                                 float length,
                                 float lock,
                                 int ratchet = 1,
                                 int condition = 0)
        {
            Sequencer::Step step;
            step.enabled = true;
            step.noteOffset = juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                                           Sequencer::PatternSequencer::maxNoteOffset,
                                           noteOffset);
            step.velocity = juce::jlimit(0.0f, 1.0f, velocity);
            step.probability = juce::jlimit(0.0f, 1.0f, probability);
            step.timing = juce::jlimit(0.0f, 1.0f, timing);
            step.length = juce::jlimit(0.18f, 1.0f, length);
            step.lock = juce::jlimit(0.0f, 1.0f, lock);
            step.ratchet = juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                        Sequencer::PatternSequencer::maxRatchet,
                                        ratchet);
            step.condition = juce::jlimit(Sequencer::PatternSequencer::minCondition,
                                          Sequencer::PatternSequencer::maxCondition,
                                          condition);
            step.slide = false;
            patternSequencer.setStep(index, step);
        };

        paintStab(2, 0, 0.82f, 1.0f, 0.14f, 0.38f, 0.30f);
        paintStab(6, 5, 0.9f, 0.96f, 0.24f, 0.34f, 0.48f);
        paintStab(10, -2, 0.76f, 0.88f, 0.18f, 0.32f, 0.36f);
        paintStab(14, 7, 0.86f, 0.92f, 0.34f, 0.3f, 0.54f, 2);
        paintStab(15, 10, 0.68f, 0.72f, 0.72f, 0.22f, 0.62f, 2, 3);

        setParameterPlainValue(Parameters::ID::sequencerEnabled, 1.0f);
        setParameterPlainValue(Parameters::ID::sequencerRate, 1.0f);
        setParameterPlainValue(Parameters::ID::sequencerGate, 0.3f);
        setParameterPlainValue(Parameters::ID::sequencerSwing, 0.26f);
        setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 4.0f);
        setParameterPlainValue(Parameters::ID::sequencerScale, 1.0f);
        setParameterPlainValue(Parameters::ID::sequencerChordMode, 9.0f);
        setParameterPlainValue(Parameters::ID::sequencerChordVoicing, 4.0f);
        setParameterPlainValue(Parameters::ID::sequencerChordStrum, 0.18f);
        setParameterPlainValue(Parameters::ID::sequencerAccent, 0.58f);
        setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
        setParameterPlainValue(Parameters::ID::sequencerProbability, 0.92f);
        setParameterPlainValue(Parameters::ID::sequencerRandomAmount, 0.32f);
        setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
        setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.34f);
        patternSequencer.reset();
        return true;
    }

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
    constexpr std::array<int, stepCount> bassContourOffsets {
        0, -12, -10, -7,
        -5, -7, -12, -10,
        0, 2, 3, 2,
        -5, -7, -2, -7
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
                    step.ratchet = 1;
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
                    if (isLateStabStep(stepIndex))
                        step.ratchet = juce::jmax(step.ratchet, 2);
                }
                break;

            case 4: // Vocal Push
                if (isVocalPushStep(stepIndex))
                {
                    step.timing = juce::jmax(step.timing, 0.78f);
                    step.probability = juce::jlimit(0.35f, 0.92f, step.probability * 0.9f);
                    step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.58f));
                    step.lock = juce::jmax(step.lock, 0.52f);
                    step.ratchet = juce::jmax(step.ratchet, 2);
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

            case 6: // House Shuffle
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = 1.0f;
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.06f);
                    step.length = juce::jmax(step.length, 0.62f);
                    step.lock = juce::jlimit(0.0f, 1.0f, step.lock * 0.65f);
                    step.ratchet = 1;
                }
                else
                {
                    step.timing = juce::jmax(step.timing, isVocalPushStep(stepIndex) ? 0.48f : (isOffbeatStep(stepIndex) ? 0.56f : 0.18f));
                    step.velocity = juce::jlimit(0.25f, 1.0f, isOffbeatStep(stepIndex) ? step.velocity * 0.94f : step.velocity * 0.9f);
                    step.probability = juce::jlimit(0.45f, 1.0f, step.probability * (isOffbeatStep(stepIndex) ? 0.96f : 0.9f));
                    step.length = juce::jlimit(0.22f, 1.0f, juce::jmin(step.length, isVocalPushStep(stepIndex) ? 0.58f : 0.68f));
                    step.lock = juce::jmax(step.lock, isOffbeatStep(stepIndex) ? 0.3f : 0.18f);
                }
                break;

            case 7: // UKG 2-Step Push
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = 1.0f;
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.04f);
                    step.length = juce::jmax(step.length, 0.56f);
                    step.lock = 0.0f;
                }
                else if (isVocalPushStep(stepIndex))
                {
                    step.timing = juce::jmax(step.timing, 0.82f);
                    step.probability = juce::jlimit(0.38f, 0.94f, step.probability * 0.9f);
                    step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity * 0.92f);
                    step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.52f));
                    step.lock = juce::jmax(step.lock, 0.54f);
                    step.ratchet = juce::jmax(step.ratchet, 2);
                }
                else if (isLateStabStep(stepIndex))
                {
                    step.timing = juce::jmax(step.timing, 0.36f);
                    step.probability = juce::jlimit(0.45f, 1.0f, step.probability);
                    step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.64f));
                    step.lock = juce::jmax(step.lock, 0.32f);
                }
                else
                {
                    step.timing = juce::jmax(step.timing, isOffbeatStep(stepIndex) ? 0.64f : 0.2f);
                    step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity * 0.88f);
                    step.probability = juce::jlimit(0.35f, 1.0f, step.probability * 0.88f);
                    step.length = juce::jlimit(0.18f, 1.0f, step.length * 0.82f);
                    step.lock = juce::jmax(step.lock, 0.22f);
                }
                break;

            case 8: // Tech House Tight
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = 1.0f;
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.08f);
                    step.length = juce::jlimit(0.28f, 1.0f, juce::jmin(step.length, 0.74f));
                    step.lock = juce::jlimit(0.0f, 1.0f, step.lock * 0.45f);
                }
                else
                {
                    step.timing = juce::jmax(step.timing, isOffbeatStep(stepIndex) ? 0.28f : 0.1f);
                    step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity + (isOffbeatStep(stepIndex) ? 0.02f : -0.04f));
                    step.probability = juce::jlimit(0.55f, 1.0f, step.probability + 0.06f);
                    step.length = juce::jlimit(0.2f, 1.0f, juce::jmin(step.length, 0.58f));
                    step.lock = juce::jmax(step.lock, isOffbeatStep(stepIndex) ? 0.24f : 0.14f);
                }
                break;

            case 9: // Minimal Skip
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = juce::jlimit(0.8f, 1.0f, step.probability);
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity * 0.96f);
                    step.length = juce::jlimit(0.24f, 1.0f, juce::jmin(step.length, 0.62f));
                    step.lock = juce::jlimit(0.0f, 1.0f, step.lock * 0.55f);
                    step.ratchet = 1;
                }
                else
                {
                    const auto skipStep = stepIndex == 5 || stepIndex == 7 || stepIndex == 11 || stepIndex == 13 || stepIndex == 15;
                    step.timing = juce::jmax(step.timing, skipStep ? 0.48f : 0.14f);
                    step.velocity = juce::jlimit(0.25f, 0.86f, step.velocity * (skipStep ? 0.9f : 0.82f));
                    step.probability = juce::jlimit(0.35f, 0.88f, step.probability * (skipStep ? 0.82f : 0.74f));
                    step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, skipStep ? 0.46f : 0.34f));
                    step.lock = juce::jmax(step.lock, skipStep ? 0.32f : 0.18f);
                    if (skipStep)
                        step.ratchet = juce::jmax(step.ratchet, 2);
                }
                break;

            case 10: // Techno Drive
                if (isAnchorStep(stepIndex))
                {
                    step.timing = 0.0f;
                    step.probability = 1.0f;
                    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.1f);
                    step.length = juce::jlimit(0.28f, 1.0f, juce::jmax(step.length, 0.58f));
                    step.lock = juce::jlimit(0.0f, 1.0f, step.lock * 0.7f);
                    step.ratchet = 1;
                }
                else
                {
                    step.timing = juce::jmax(step.timing, isVocalPushStep(stepIndex) ? 0.26f : 0.12f);
                    step.velocity = juce::jlimit(0.35f, 1.0f, step.velocity + 0.04f);
                    step.probability = juce::jlimit(0.62f, 1.0f, step.probability + 0.08f);
                    step.length = juce::jlimit(0.22f, 1.0f, juce::jmin(step.length, 0.72f));
                    step.lock = juce::jmax(step.lock, isVocalPushStep(stepIndex) ? 0.34f : 0.22f);
                }
                break;

            case 11: // Bass Contour
            {
                const auto anchor = isAnchorStep(stepIndex);
                const auto offbeat = isOffbeatStep(stepIndex);
                const auto pickup = stepIndex == 3 || stepIndex == 7 || stepIndex == 11 || stepIndex == 15;
                const auto fill = stepIndex == 14 || stepIndex == 15;

                step.noteOffset = bassContourOffsets[static_cast<size_t>(stepIndex)];
                step.timing = anchor ? 0.0f : juce::jmax(step.timing, pickup ? 0.46f : (offbeat ? 0.34f : 0.18f));
                step.velocity = juce::jlimit(0.25f, 1.0f, anchor ? juce::jmax(step.velocity, 0.88f)
                                                                  : (step.velocity * (pickup ? 0.86f : 0.92f)));
                step.probability = anchor ? 1.0f : juce::jlimit(0.55f, 0.94f, step.probability * (pickup ? 0.82f : 0.9f));
                step.length = juce::jlimit(0.22f, 1.0f, anchor ? juce::jmax(step.length, 0.58f)
                                                               : juce::jmin(step.length, pickup ? 0.42f : 0.54f));
                step.lock = juce::jmax(step.lock, anchor ? 0.12f : (pickup ? 0.34f : 0.24f));
                step.ratchet = fill ? juce::jmax(step.ratchet, 2) : 1;
                step.condition = fill ? 3 : 0;
                step.slide = ! anchor && ! fill && (pickup || offbeat);
                break;
            }

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

        case 6:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.28f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.48f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.54f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.9f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.26f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 7:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.46f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 2.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.3f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.58f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmin(0.93f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.34f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 8:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.12f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 5.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.36f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.7f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.96f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.22f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 9:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.2f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 6.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.28f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmin(0.52f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmin(0.84f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 7.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.24f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 10:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.1f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 7.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.46f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.78f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.98f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 3.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.3f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
            break;

        case 11:
            setParameterPlainValue(Parameters::ID::sequencerSwing, 0.24f);
            setParameterPlainValue(Parameters::ID::sequencerGrooveMode, 4.0f);
            setParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
            setParameterPlainValue(Parameters::ID::sequencerOctave, -1.0f);
            setParameterPlainValue(Parameters::ID::sequencerGate,
                                   juce::jlimit(0.05f, 0.95f, juce::jmin(0.48f, getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f))));
            setParameterPlainValue(Parameters::ID::sequencerAccent,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.62f, getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f))));
            setParameterPlainValue(Parameters::ID::sequencerProbability,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.94f, getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f))));
            setParameterPlainValue(Parameters::ID::sequencerLockDestination, 1.0f);
            setParameterPlainValue(Parameters::ID::sequencerLockDepth,
                                   juce::jlimit(0.0f, 1.0f, juce::jmax(0.26f, getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f))));
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

void NateVSTAudioProcessor::buildSequencerFourBarSceneChain()
{
    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;

    std::array<Sequencer::Step, stepCount> baseSteps {};
    auto rootOffset = 0;
    auto foundRoot = false;
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        baseSteps[static_cast<size_t>(stepIndex)] = patternSequencer.getStep(stepIndex);
        const auto& step = baseSteps[static_cast<size_t>(stepIndex)];
        if (step.enabled && (! foundRoot || stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12))
        {
            rootOffset = step.noteOffset;
            foundRoot = true;
            if (stepIndex == 0)
                break;
        }
    }

    auto clampNote = [] (int note)
    {
        return juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                            Sequencer::PatternSequencer::maxNoteOffset,
                            note);
    };

    auto reinforceAnchors = [] (std::array<Sequencer::Step, stepCount>& steps)
    {
        for (auto stepIndex = 0; stepIndex < stepCount; stepIndex += 4)
        {
            auto& step = steps[static_cast<size_t>(stepIndex)];
            if (! step.enabled)
                continue;

            step.timing = 0.0f;
            step.probability = 1.0f;
            step.velocity = juce::jmax(step.velocity, 0.84f);
            step.length = juce::jmax(step.length, 0.58f);
            step.ratchet = 1;
            step.condition = 0;
        }
    };

    auto ensureStep = [&clampNote] (std::array<Sequencer::Step, stepCount>& steps,
                                    int index,
                                    int note,
                                    float velocity,
                                    float timing,
                                    float length,
                                    float lock,
                                    int ratchet,
                                    int condition)
    {
        auto& step = steps[static_cast<size_t>(juce::jlimit(0, stepCount - 1, index))];
        step.enabled = true;
        step.noteOffset = clampNote(note);
        step.velocity = juce::jmax(step.velocity, velocity);
        step.probability = juce::jmax(step.probability, 0.86f);
        step.timing = juce::jmax(step.timing, timing);
        step.length = juce::jmax(step.length, length);
        step.lock = juce::jmax(step.lock, lock);
        step.ratchet = juce::jmax(step.ratchet, ratchet);
        step.condition = condition;
    };

    auto bSteps = baseSteps;
    auto fillSteps = baseSteps;
    auto dropSteps = baseSteps;
    const auto chordMode = getParameterPlainValue(Parameters::ID::sequencerChordMode, 0.0f);
    const auto isChordPatch = chordMode > 0.5f;

    reinforceAnchors(bSteps);
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        auto& step = bSteps[static_cast<size_t>(stepIndex)];
        if (! step.enabled || (stepIndex % 4) == 0)
            continue;

        const auto offbeat = (stepIndex % 2) != 0;
        step.noteOffset = clampNote(step.noteOffset + (offbeat ? 2 : -2));
        step.timing = juce::jmax(step.timing, offbeat ? 0.42f : 0.18f);
        step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity * (offbeat ? 0.95f : 0.9f));
        step.probability = juce::jlimit(0.45f, 1.0f, step.probability * 0.94f);
        step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, offbeat ? 0.62f : 0.52f));
        step.lock = juce::jmax(step.lock, offbeat ? 0.28f : 0.18f);
        step.condition = 0;
        if (stepIndex == 7 || stepIndex == 11 || stepIndex == 15)
            step.ratchet = juce::jmax(step.ratchet, 2);
    }
    ensureStep(bSteps, 7, rootOffset + (isChordPatch ? 5 : 7), 0.66f, 0.58f, 0.42f, 0.28f, 1, 0);

    reinforceAnchors(fillSteps);
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        auto& step = fillSteps[static_cast<size_t>(stepIndex)];
        if (! step.enabled)
            continue;

        if ((stepIndex % 4) != 0)
        {
            step.timing = juce::jmax(step.timing, (stepIndex % 2) != 0 ? 0.52f : 0.22f);
            step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.52f));
            step.lock = juce::jmax(step.lock, 0.32f);
        }
        step.condition = 0;
    }
    ensureStep(fillSteps, 14, rootOffset + (isChordPatch ? 10 : -2), 0.72f, 0.56f, 0.34f, 0.44f, 3, 0);
    ensureStep(fillSteps, 15, rootOffset + (isChordPatch ? 12 : -7), 0.68f, 0.82f, 0.24f, 0.56f, 2, 0);

    reinforceAnchors(dropSteps);
    for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
    {
        auto& step = dropSteps[static_cast<size_t>(stepIndex)];
        if (! step.enabled)
            continue;

        if ((stepIndex % 4) == 0)
        {
            step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity + 0.08f);
            step.length = juce::jmax(step.length, 0.66f);
            step.lock = juce::jmax(step.lock, 0.12f);
        }
        else
        {
            step.velocity = juce::jlimit(0.25f, 1.0f, step.velocity * 0.88f);
            step.probability = juce::jlimit(0.45f, 1.0f, step.probability * 0.86f);
            step.length = juce::jlimit(0.18f, 1.0f, juce::jmin(step.length, 0.48f));
            step.lock = juce::jmax(step.lock, 0.22f);
        }
        step.condition = 0;
    }
    ensureStep(dropSteps, 12, rootOffset + (isChordPatch ? 12 : 0), 0.9f, 0.0f, 0.68f, 0.28f, 1, 0);
    ensureStep(dropSteps, 15, rootOffset + (isChordPatch ? 7 : -5), 0.74f, 0.72f, 0.3f, 0.56f, 3, 3);

    auto sceneFromSteps = [this] (const std::array<Sequencer::Step, stepCount>& steps)
    {
        auto scene = createSequencerSceneState();
        for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
            writeSequencerExportStep(scene, stepIndex, steps[static_cast<size_t>(stepIndex)]);
        return scene;
    };

    auto sceneA = sceneFromSteps(baseSteps);
    auto sceneB = sceneFromSteps(bSteps);
    auto sceneFill = sceneFromSteps(fillSteps);
    auto sceneDrop = sceneFromSteps(dropSteps);

    const auto swing = getParameterPlainValue(Parameters::ID::sequencerSwing, 0.0f);
    const auto gate = getParameterPlainValue(Parameters::ID::sequencerGate, 0.55f);
    const auto accent = getParameterPlainValue(Parameters::ID::sequencerAccent, 0.35f);
    const auto probability = getParameterPlainValue(Parameters::ID::sequencerProbability, 1.0f);
    const auto lockDepth = getParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f);

    writePresetParameterValue(sceneA, Parameters::ID::sequencerEnabled, 1.0f);
    writePresetParameterValue(sceneB, Parameters::ID::sequencerEnabled, 1.0f);
    writePresetParameterValue(sceneFill, Parameters::ID::sequencerEnabled, 1.0f);
    writePresetParameterValue(sceneDrop, Parameters::ID::sequencerEnabled, 1.0f);

    writePresetParameterValue(sceneB, Parameters::ID::sequencerSwing, juce::jlimit(0.0f, 0.65f, juce::jmax(0.22f, swing + 0.05f)));
    writePresetParameterValue(sceneB, Parameters::ID::sequencerProbability, juce::jlimit(0.0f, 1.0f, juce::jmin(0.96f, probability)));
    writePresetParameterValue(sceneFill, Parameters::ID::sequencerGate, juce::jlimit(0.05f, 0.95f, juce::jmin(0.42f, gate)));
    writePresetParameterValue(sceneFill, Parameters::ID::sequencerLockDepth, juce::jlimit(0.0f, 1.0f, juce::jmax(0.4f, lockDepth)));
    writePresetParameterValue(sceneFill, Parameters::ID::sequencerLockDestination, 5.0f);
    writePresetParameterValue(sceneDrop, Parameters::ID::sequencerAccent, juce::jlimit(0.0f, 1.0f, juce::jmax(0.68f, accent)));
    writePresetParameterValue(sceneDrop, Parameters::ID::sequencerGate, juce::jlimit(0.05f, 0.95f, juce::jmax(0.5f, gate)));
    writePresetParameterValue(sceneDrop, Parameters::ID::sequencerLockDepth, juce::jlimit(0.0f, 1.0f, juce::jmax(0.32f, lockDepth)));

    sequencerPatternScenes[0] = sceneA;
    sequencerPatternScenes[1] = sceneB;
    sequencerPatternScenes[2] = sceneFill;
    sequencerPatternScenes[3] = sceneDrop;
    refreshSequencerSceneChainPlayback();
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

void NateVSTAudioProcessor::captureSequencerPatternScene(int slotIndex)
{
    const auto safeSlot = juce::jlimit(0, static_cast<int>(sequencerPatternScenes.size()) - 1, slotIndex);
    sequencerPatternScenes[static_cast<size_t>(safeSlot)] = createSequencerSceneState();
    refreshSequencerSceneChainPlayback();
}

bool NateVSTAudioProcessor::recallSequencerPatternScene(int slotIndex)
{
    const auto safeSlot = juce::jlimit(0, static_cast<int>(sequencerPatternScenes.size()) - 1, slotIndex);
    const auto& scene = sequencerPatternScenes[static_cast<size_t>(safeSlot)];
    if (! scene.isValid())
        return false;

    captureSequencerUndoState();
    restoreSequencerFromState(scene);
    patternSequencer.reset();
    return true;
}

bool NateVSTAudioProcessor::hasSequencerPatternScene(int slotIndex) const
{
    const auto safeSlot = juce::jlimit(0, static_cast<int>(sequencerPatternScenes.size()) - 1, slotIndex);
    return sequencerPatternScenes[static_cast<size_t>(safeSlot)].isValid();
}

juce::String NateVSTAudioProcessor::getSequencerPatternSceneSummary(int slotIndex) const
{
    const auto safeSlot = juce::jlimit(0, static_cast<int>(sequencerPatternScenes.size()) - 1, slotIndex);
    const auto& scene = sequencerPatternScenes[static_cast<size_t>(safeSlot)];
    const auto label = sequencerPatternSceneLabels()[static_cast<size_t>(safeSlot)];
    if (! scene.isValid())
        return juce::String(label) + " empty";

    auto enabledCount = 0;
    auto lockCount = 0;
    auto ratchetCount = 0;
    auto conditionCount = 0;
    auto slideCount = 0;
    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        const auto prefix = "seq_step_" + juce::String(stepIndex);
        if (static_cast<bool>(scene.getProperty(prefix + "_enabled", false)))
            ++enabledCount;
        if (static_cast<float>(scene.getProperty(prefix + "_lock", 0.0f)) > 0.01f)
            ++lockCount;
        if (static_cast<int>(scene.getProperty(prefix + "_ratchet", 1)) > 1)
            ++ratchetCount;
        if (static_cast<int>(scene.getProperty(prefix + "_condition", 0)) > 0)
            ++conditionCount;
        if (static_cast<bool>(scene.getProperty(prefix + "_slide", false)))
            ++slideCount;
    }

    const auto chordMode = juce::roundToInt(readStateParameterValue(scene, Parameters::ID::sequencerChordMode, 0.0f));
    const auto grooveMode = juce::roundToInt(readStateParameterValue(scene, Parameters::ID::sequencerGrooveMode, 0.0f));
    return juce::String(label) + ": " + juce::String(enabledCount) + " steps"
        + (chordMode > 0 ? " chord" : "")
        + (grooveMode > 0 ? " groove" : "")
        + (ratchetCount > 0 ? " " + juce::String(ratchetCount) + " rats" : "")
        + (conditionCount > 0 ? " " + juce::String(conditionCount) + " conds" : "")
        + (slideCount > 0 ? " " + juce::String(slideCount) + " slides" : "")
        + (lockCount > 0 ? " " + juce::String(lockCount) + " locks" : "");
}

void NateVSTAudioProcessor::setSequencerSceneChainPlaybackEnabled(bool shouldEnable)
{
    sequencerSceneChainPlaybackEnabled.store(shouldEnable ? 1 : 0, std::memory_order_release);
    refreshSequencerSceneChainPlayback();
    patternSequencer.reset();
}

bool NateVSTAudioProcessor::isSequencerSceneChainPlaybackEnabled() const noexcept
{
    return sequencerSceneChainPlaybackEnabled.load(std::memory_order_acquire) > 0;
}

void NateVSTAudioProcessor::setSequencerSceneChainClipBars(int barCount)
{
    sequencerSceneChainClipBars.store(sanitiseSequencerSceneChainClipBars(barCount), std::memory_order_release);
    refreshSequencerSceneChainPlayback();
    if (isSequencerSceneChainPlaybackEnabled())
        patternSequencer.reset();
}

int NateVSTAudioProcessor::getSequencerSceneChainClipBars() const noexcept
{
    return sanitiseSequencerSceneChainClipBars(sequencerSceneChainClipBars.load(std::memory_order_acquire));
}

int NateVSTAudioProcessor::getSequencerSceneChainPlaybackLength() const
{
    auto count = 0;
    for (const auto& scene : sequencerPatternScenes)
        if (scene.isValid())
            ++count;

    const auto forcedBars = getSequencerSceneChainClipBars();
    if (forcedBars > 0 && (count > 0 || isSequencerSceneChainPlaybackEnabled()))
        return forcedBars;

    return count;
}

bool NateVSTAudioProcessor::exportSequencerMidiFile(const juce::File& destination) const
{
    return writeSequencerMidiFile(destination, { createSequencerSceneState() });
}

bool NateVSTAudioProcessor::exportSequencerSceneChainMidiFile(const juce::File& destination) const
{
    return writeSequencerMidiFile(destination, createSequencerSceneChainSegments(getSequencerSceneChainClipBars()));
}

bool NateVSTAudioProcessor::writeSequencerMidiFile(const juce::File& destination, const std::vector<juce::ValueTree>& segments) const
{
    if (destination == juce::File{})
        return false;

    auto outputFile = destination.hasFileExtension(".mid;.midi") ? destination : destination.withFileExtension(".mid");
    if (! outputFile.getParentDirectory().createDirectory())
        return false;

    constexpr auto ticksPerQuarterNote = 960;
    constexpr auto stepCount = Sequencer::PatternSequencer::numSteps;
    struct SequencerExportNote
    {
        int noteNumber = 60;
        float velocity = 0.8f;
        int startTicks = 0;
        int endTicks = 1;
    };

    auto stepDelayTicks = [] (const juce::ValueTree& segment, int stepTicks, float swing, int grooveMode, int stepIndex)
    {
        const auto safeIndex = juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1, stepIndex);
        const auto step = readSequencerExportStep(segment, safeIndex);
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

            case 4:
                if (! isAnchorStep)
                    weight = step.timing > 0.0f
                        ? step.timing
                        : (safeIndex == 3 || safeIndex == 7 || safeIndex == 11 || safeIndex == 15 ? 0.42f
                            : isOffbeatStep ? 0.58f
                                            : 0.16f);
                break;

            case 5:
                weight = isAnchorStep ? 0.0f
                                      : (step.timing > 0.0f ? step.timing * 0.72f
                                                           : (isOffbeatStep ? 0.28f : 0.08f));
                break;

            case 6:
                if (! isAnchorStep)
                    weight = step.timing > 0.0f
                        ? step.timing * 0.82f
                        : (safeIndex == 5 || safeIndex == 7 || safeIndex == 11 || safeIndex == 13 || safeIndex == 15 ? 0.48f
                                                                                                                       : 0.14f);
                break;

            case 7:
                weight = isAnchorStep ? 0.0f
                                      : (step.timing > 0.0f ? step.timing * 0.55f
                                                           : (safeIndex == 3 || safeIndex == 7 || safeIndex == 11 || safeIndex == 15 ? 0.26f
                                                                                                                                    : 0.12f));
                break;

            case 0:
            default:
                weight = isOffbeatStep ? 1.0f : 0.0f;
                break;
        }

        return juce::roundToInt(maxDelay * juce::jlimit(0.0f, 1.0f, weight));
    };

    std::vector<SequencerExportNote> exportedMidiNotes;
    std::vector<size_t> previousNoteGroup;
    auto previousNoteGroupShouldSlide = false;
    auto segmentStartTicks = 0;
    const auto segmentCount = static_cast<int>(segments.size());

    for (auto segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        const auto& segment = segments[static_cast<size_t>(segmentIndex)];
        if (! segment.isValid())
            continue;

        const auto rateIndex = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerRate, 1.0f));
        const auto stepTicks = rateIndex == 0 ? ticksPerQuarterNote / 2
            : rateIndex == 2 ? ticksPerQuarterNote / 8
            : ticksPerQuarterNote / 4;
        const auto root = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerRoot, 36.0f));
        const auto octaveOffset = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerOctave, 0.0f)) * 12;
        const auto gate = juce::jlimit(0.05f, 0.95f, readPresetParameterValue(segment, Parameters::ID::sequencerGate, 0.55f));
        const auto swing = juce::jlimit(0.0f, 0.65f, readPresetParameterValue(segment, Parameters::ID::sequencerSwing, 0.0f));
        const auto grooveMode = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerGrooveMode, 0.0f));
        const auto scaleMode = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerScale, 0.0f));
        const auto chordMode = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerChordMode, 0.0f));
        const auto chordVoicing = juce::roundToInt(readPresetParameterValue(segment, Parameters::ID::sequencerChordVoicing, 0.0f));
        const auto chordStrum = readPresetParameterValue(segment, Parameters::ID::sequencerChordStrum, 0.0f);
        const auto accent = juce::jlimit(0.0f, 1.0f, readPresetParameterValue(segment, Parameters::ID::sequencerAccent, 0.35f));

        for (auto stepIndex = 0; stepIndex < stepCount; ++stepIndex)
        {
            const auto step = readSequencerExportStep(segment, stepIndex);
            if (! step.enabled || ! sequencerExportConditionAllows(step.condition, segmentIndex, segmentCount))
                continue;

            const auto delayTicks = stepDelayTicks(segment, stepTicks, swing, grooveMode, stepIndex);
            const auto nextDelayTicks = stepDelayTicks(segment, stepTicks, swing, grooveMode, (stepIndex + 1) % stepCount);
            const auto durationTicks = juce::jmax(1, stepTicks + nextDelayTicks - delayTicks);
            const auto stepGate = juce::jlimit(0.05f, 1.0f, gate * juce::jlimit(0.1f, 1.0f, step.length));
            const auto gateTicks = juce::jlimit(1, juce::jmax(1, durationTicks - 1),
                                               juce::roundToInt(static_cast<float>(durationTicks) * stepGate));
            const auto isAnchorStep = stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12;
            const auto velocity = isAnchorStep
                ? juce::jlimit(0.0f, 1.0f, step.velocity + ((1.0f - step.velocity) * accent))
                : juce::jlimit(0.0f, 1.0f, step.velocity * (1.0f - (accent * 0.12f)));
            const auto startTicks = segmentStartTicks + (stepIndex * stepTicks) + delayTicks;
            auto noteCount = 0;
            const auto notes = sequencerExportChordNotes(root, octaveOffset, step.noteOffset, scaleMode, chordMode, chordVoicing, noteCount);
            const auto ratchetCount = juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                                   Sequencer::PatternSequencer::maxRatchet,
                                                   step.ratchet);
            const auto ratchetSpacingTicks = static_cast<double>(durationTicks) / static_cast<double>(ratchetCount);
            const auto ratchetGateTicks = juce::jlimit(1,
                                                       juce::jmax(1, juce::roundToInt(ratchetSpacingTicks) - 1),
                                                       gateTicks);

            for (auto ratchetIndex = 0; ratchetIndex < ratchetCount; ++ratchetIndex)
            {
                const auto hitStartTicks = startTicks + juce::roundToInt(static_cast<double>(ratchetIndex) * ratchetSpacingTicks);
                const auto hitVelocity = juce::jlimit(0.0f,
                                                      1.0f,
                                                      velocity * (1.0f - (0.07f * static_cast<float>(ratchetIndex))));
                std::vector<size_t> currentNoteGroup;

                for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
                {
                    const auto noteNumber = notes[static_cast<size_t>(noteIndex)];
                    const auto noteVelocity = sequencerExportChordNoteVelocity(hitVelocity, noteIndex);
                    const auto strumOffset = ratchetCount > 1 ? 0 : sequencerExportChordStrumOffset(stepTicks, noteIndex, noteCount, chordStrum);
                    const auto noteStartTicks = hitStartTicks + strumOffset;
                    const auto noteEndTicks = juce::jmax(noteStartTicks + 1, hitStartTicks + ratchetGateTicks);
                    currentNoteGroup.push_back(exportedMidiNotes.size());
                    exportedMidiNotes.push_back({ noteNumber, noteVelocity, noteStartTicks, noteEndTicks });
                }

                if (previousNoteGroupShouldSlide && ratchetIndex == 0 && ! previousNoteGroup.empty() && ! currentNoteGroup.empty())
                {
                    const auto slideTargetTicks = exportedMidiNotes[currentNoteGroup.front()].startTicks;
                    for (const auto previousIndex : previousNoteGroup)
                        if (previousIndex < exportedMidiNotes.size())
                            exportedMidiNotes[previousIndex].endTicks = juce::jmax(exportedMidiNotes[previousIndex].endTicks,
                                                                                   slideTargetTicks + 1);
                }

                if (! currentNoteGroup.empty())
                {
                    previousNoteGroup = std::move(currentNoteGroup);
                    previousNoteGroupShouldSlide = step.slide && ratchetCount <= 1;
                }
            }
        }

        segmentStartTicks += stepCount * stepTicks;
    }

    if (exportedMidiNotes.empty())
        return false;

    juce::MidiMessageSequence sequence;
    for (const auto& note : exportedMidiNotes)
    {
        sequence.addEvent(juce::MidiMessage::noteOn(1, note.noteNumber, note.velocity), static_cast<double>(note.startTicks));
        sequence.addEvent(juce::MidiMessage::noteOff(1, note.noteNumber), static_cast<double>(juce::jmax(note.startTicks + 1, note.endTicks)));
    }

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
    return savePresetState(presetName, options, createPluginState());
}

bool NateVSTAudioProcessor::saveRandomCandidatePreset(int slotIndex,
                                                      const juce::String& presetName,
                                                      const PresetSaveOptions& options)
{
    if (! hasRandomCandidate(slotIndex))
        return false;

    auto candidateOptions = options;
    candidateOptions.generated = true;
    if (candidateOptions.generatedRecipe.trim().isEmpty())
        candidateOptions.generatedRecipe = randomCandidateSnapshots[static_cast<size_t>(slotIndex)].label;

    return savePresetState(presetName,
                           candidateOptions,
                           randomCandidateSnapshots[static_cast<size_t>(slotIndex)].state.createCopy());
}

bool NateVSTAudioProcessor::userPresetExists(const juce::String& presetName, const juce::String& category) const
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    const auto storedCategory = normalisedPresetCategory(presetTextOrFallback(category, "User"));
    return presetFileForName(trimmedName, storedCategory).existsAsFile();
}

bool NateVSTAudioProcessor::savePresetState(const juce::String& presetName,
                                            const PresetSaveOptions& options,
                                            juce::ValueTree state)
{
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
        return false;

    const auto storedCategory = normalisedPresetCategory(presetTextOrFallback(options.category, "User"));
    const auto storedAuthor = presetTextOrFallback(options.author, "User");
    const auto storedPack = presetTextOrFallback(options.pack, "User Pack");
    const auto storedKey = presetTextOrFallback(options.key, "Any Key");
    const auto storedBpm = normalisePresetBpm(options.bpm);
    const auto storedGeneratedRecipe = presetTextOrFallback(options.generatedRecipe, "Random Lab");
    const auto storedNotes = options.notes.trim().isNotEmpty()
        ? options.notes.trim()
        : (options.generated
               ? "Recipe: " + storedGeneratedRecipe
                    + "\nSource: Random Lab"
                    + "\nCategory: " + storedCategory
                    + "\nPack: " + storedPack
               : juce::String());
    const auto directory = presetDirectoryForCategory(storedCategory);
    if (! directory.createDirectory())
        return false;

    state.setProperty("preset_name", trimmedName, nullptr);
    state.setProperty("preset_category", storedCategory, nullptr);
    state.setProperty("preset_author", storedAuthor, nullptr);
    state.setProperty("preset_source", options.generated ? "Generated" : "User", nullptr);
    state.setProperty("preset_pack", storedPack, nullptr);
    state.setProperty("preset_key", storedKey, nullptr);
    state.setProperty("preset_bpm", storedBpm, nullptr);
    state.setProperty("preset_tags",
                      storedPack.equalsIgnoreCase(storedCategory)
                          ? storedCategory + (options.generated ? ", Generated, Random Lab, " + storedGeneratedRecipe : "")
                          : storedCategory + ", " + storedPack + (options.generated ? ", Generated, Random Lab, " + storedGeneratedRecipe : ""),
                      nullptr);
    state.setProperty("preset_folder", presetCategoryPathSegments(storedCategory).joinIntoString("/"), nullptr);
    if (options.generated)
        state.setProperty("preset_generated_recipe", storedGeneratedRecipe, nullptr);
    if (storedNotes.isNotEmpty())
        state.setProperty("preset_notes", storedNotes, nullptr);

    if (auto xml = state.createXml())
    {
        const auto destination = presetFileForName(trimmedName, storedCategory);
        juce::TemporaryFile temporaryFile(destination);
        if (! xml->writeTo(temporaryFile.getFile()))
            return false;

        if (! temporaryFile.getFile().existsAsFile() || temporaryFile.getFile().getSize() == 0)
            return false;

        if (! temporaryFile.overwriteTargetFileWithTemporary())
            return false;

        invalidatePresetLibraryCache();
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
    if (presetLibraryCacheValid)
        return presetLibraryCache;

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

    auto collectFromDirectory = [this, &presets, &seenNames, &favorites, &ratingForPreset] (const juce::File& directory, bool isFactory)
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
            juce::String notes;
            std::array<float, 8> macroValues {};
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

                    macroValues = presetMacroValues(state);
                    macroSummary = presetMacroSummary(macroValues, macroIntensity);

                    const auto storedNotes = state.getProperty("preset_notes").toString().trim();
                    if (storedNotes.isNotEmpty())
                        notes = storedNotes;

                    const auto storedFolder = state.getProperty("preset_folder").toString().trim();
                    if (storedFolder.isNotEmpty())
                        folder = storedFolder.replaceCharacter('\\', '/');
                }
            }

            if (folder.isNotEmpty() && category == "User")
                category = folder.fromLastOccurrenceOf("/", false, true);

            const auto previewFile = presetPreviewFileForName(name);
            auto previewAvailable = previewFile.existsAsFile();
            auto previewLastModified = previewAvailable ? previewFile.getLastModificationTime().toMilliseconds() : 0;
            auto previewStale = previewAvailable && previewLastModified < file.getLastModificationTime().toMilliseconds();
            auto previewPeak = 0.0f;
            auto previewRms = 0.0f;
            auto previewDurationSeconds = 0.0;
            if (previewAvailable)
            {
                juce::AudioBuffer<float> previewBuffer;
                if (readPresetPreviewFile(previewFile, previewBuffer))
                {
                    const auto measured = measurePresetPreviewBuffer(previewBuffer, presetPreviewSampleRate);
                    previewPeak = measured.peak;
                    previewRms = measured.rms;
                    previewDurationSeconds = measured.durationSeconds;
                }
                else
                {
                    previewAvailable = false;
                    previewLastModified = 0;
                    previewStale = false;
                }
            }

            presets.push_back({ name,
                                category,
                                source,
                                tags,
                                folder,
                                author,
                                pack,
                                key,
                                macroSummary,
                                notes,
                                macroValues,
                                bpm,
                                ratingForPreset(name),
                                macroIntensity,
                                file.getLastModificationTime().toMilliseconds(),
                                isFactory,
                                favorites.contains(name),
                                previewAvailable,
                                previewStale,
                                previewPeak,
                                previewRms,
                                previewDurationSeconds,
                                previewLastModified });
            seenNames.add(name);
        }
    };

    collectFromDirectory(getPresetDirectory(), false);
    collectFromDirectory(getFactoryPresetDirectory(), true);

    std::sort(presets.begin(), presets.end(), [] (const PresetInfo& left, const PresetInfo& right)
    {
        return left.name.compareIgnoreCase(right.name) < 0;
    });

    presetLibraryCache = presets;
    presetLibraryCacheValid = true;
    return presetLibraryCache;
}

void NateVSTAudioProcessor::invalidatePresetLibraryCache() const
{
    presetLibraryCache.clear();
    presetLibraryCacheValid = false;
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

juce::File NateVSTAudioProcessor::getPresetPreviewDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Nate VST")
        .getChildFile("Preset Previews");
}

NateVSTAudioProcessor::PresetPreviewInfo NateVSTAudioProcessor::getPresetPreviewInfo(const juce::String& presetName) const
{
    PresetPreviewInfo info;
    info.file = presetPreviewFileForName(presetName);

    auto presetFile = presetFileForName(presetName);
    if (! presetFile.existsAsFile())
        presetFile = factoryPresetFileForName(presetName);

    if (! presetFile.existsAsFile())
    {
        info.status = "Preset file missing";
        return info;
    }

    info.available = info.file.existsAsFile();
    info.lastModifiedMs = info.available ? info.file.getLastModificationTime().toMilliseconds() : 0;
    info.stale = info.available && info.lastModifiedMs < presetFile.getLastModificationTime().toMilliseconds();

    if (! info.available)
    {
        info.status = "Preview not rendered";
        return info;
    }

    juce::AudioBuffer<float> buffer;
    if (! readPresetPreviewFile(info.file, buffer))
    {
        info.available = false;
        info.status = "Preview file unreadable";
        return info;
    }

    const auto measured = measurePresetPreviewBuffer(buffer, presetPreviewSampleRate);
    info.peak = measured.peak;
    info.rms = measured.rms;
    info.durationSeconds = measured.durationSeconds;
    info.status = info.stale ? "Preview stale | " + previewLevelText(info.peak, info.rms)
                             : "Preview ready | " + previewLevelText(info.peak, info.rms);
    return info;
}

NateVSTAudioProcessor::PresetPreviewInfo NateVSTAudioProcessor::ensurePresetPreview(const juce::String& presetName)
{
    auto info = getPresetPreviewInfo(presetName);
    if (info.available && ! info.stale && info.peak > 0.001f && info.rms > 0.00005f)
        return info;

    auto renderResult = renderPresetPreviewBuffer(presetName);
    if (! renderResult.rendered)
        return renderResult.info;

    renderResult.info.file = presetPreviewFileForName(presetName);
    if (! writePresetPreviewFile(renderResult.info.file, renderResult.buffer, presetPreviewSampleRate))
    {
        renderResult.info.available = false;
        renderResult.info.status = "Preview render could not be cached";
        return renderResult.info;
    }

    renderResult.info.available = true;
    renderResult.info.stale = false;
    renderResult.info.lastModifiedMs = renderResult.info.file.getLastModificationTime().toMilliseconds();
    invalidatePresetLibraryCache();
    return renderResult.info;
}

NateVSTAudioProcessor::PresetPreviewBatchResult NateVSTAudioProcessor::ensurePresetPreviews(const juce::StringArray& presetNames,
                                                                                            bool regenerateExisting)
{
    PresetPreviewBatchResult result;
    juce::StringArray uniqueNames;

    for (const auto& presetName : presetNames)
    {
        const auto trimmedName = presetName.trim();
        if (trimmedName.isNotEmpty())
            uniqueNames.addIfNotAlreadyThere(trimmedName);
    }

    result.requested = uniqueNames.size();

    auto updateStatus = [&result]
    {
        result.status = "Preview batch: " + juce::String(result.rendered) + " rendered"
            + " | " + juce::String(result.alreadyReady) + " ready"
            + " | " + juce::String(result.stale) + " stale"
            + " | " + juce::String(result.failed) + " failed";
    };

    auto previewIsUsable = [] (const PresetPreviewInfo& info)
    {
        return info.available
            && ! info.stale
            && info.peak > 0.001f
            && info.rms > 0.00005f
            && std::isfinite(info.peak)
            && std::isfinite(info.rms);
    };

    for (const auto& presetName : uniqueNames)
    {
        const auto existingInfo = getPresetPreviewInfo(presetName);
        if (! regenerateExisting && previewIsUsable(existingInfo))
        {
            ++result.alreadyReady;
            result.readyPresetNames.add(presetName);
            continue;
        }

        if (existingInfo.stale)
        {
            ++result.stale;
            result.stalePresetNames.add(presetName);
        }

        auto renderResult = renderPresetPreviewBuffer(presetName);
        if (! renderResult.rendered)
        {
            ++result.failed;
            result.failedPresetNames.add(presetName);
            continue;
        }

        renderResult.info.file = presetPreviewFileForName(presetName);
        if (! writePresetPreviewFile(renderResult.info.file, renderResult.buffer, presetPreviewSampleRate))
        {
            ++result.failed;
            result.failedPresetNames.add(presetName);
            continue;
        }

        ++result.rendered;
        result.renderedPresetNames.add(presetName);
        invalidatePresetLibraryCache();
    }

    updateStatus();
    return result;
}

NateVSTAudioProcessor::PresetPreviewBatchResult NateVSTAudioProcessor::ensurePresetPreviews(int maxRenderCount,
                                                                                            bool regenerateExisting)
{
    PresetPreviewBatchResult result;
    const auto renderLimit = juce::jmax(0, maxRenderCount);
    if (renderLimit <= 0)
    {
        result.status = "Preview batch: no render slots requested";
        return result;
    }

    juce::StringArray namesToRender;
    for (const auto& preset : getPresetLibrary())
    {
        const auto ready = preset.previewAvailable
            && ! preset.previewStale
            && preset.previewPeak > 0.001f
            && preset.previewRms > 0.00005f
            && std::isfinite(preset.previewPeak)
            && std::isfinite(preset.previewRms);

        if (ready && ! regenerateExisting)
        {
            ++result.alreadyReady;
            result.readyPresetNames.add(preset.name);
            continue;
        }

        namesToRender.add(preset.name);
        if (namesToRender.size() >= renderLimit)
            break;
    }

    if (namesToRender.isEmpty())
    {
        result.requested = result.alreadyReady;
        result.status = "Preview batch: 0 rendered | " + juce::String(result.alreadyReady)
            + " ready | 0 stale | 0 failed";
        return result;
    }

    auto batchResult = ensurePresetPreviews(namesToRender, regenerateExisting);
    batchResult.alreadyReady += result.alreadyReady;
    batchResult.readyPresetNames.addArray(result.readyPresetNames);
    batchResult.status = "Preview batch: " + juce::String(batchResult.rendered) + " rendered"
        + " | " + juce::String(batchResult.alreadyReady) + " ready"
        + " | " + juce::String(batchResult.stale) + " stale"
        + " | " + juce::String(batchResult.failed) + " failed";
    return batchResult;
}

bool NateVSTAudioProcessor::startPresetPreviewPlayback(const juce::String& presetName)
{
    juce::AudioBuffer<float> playbackBuffer;

    auto info = getPresetPreviewInfo(presetName);
    if (! info.available || info.stale || info.peak <= 0.001f || info.rms <= 0.00005f)
    {
        auto renderResult = renderPresetPreviewBuffer(presetName);
        if (! renderResult.rendered)
            return false;

        playbackBuffer.makeCopyOf(renderResult.buffer);
        renderResult.info.file = presetPreviewFileForName(presetName);
        if (writePresetPreviewFile(renderResult.info.file, renderResult.buffer, presetPreviewSampleRate))
            invalidatePresetLibraryCache();
    }
    else if (! readPresetPreviewFile(info.file, playbackBuffer))
    {
        auto renderResult = renderPresetPreviewBuffer(presetName);
        if (! renderResult.rendered)
            return false;

        playbackBuffer.makeCopyOf(renderResult.buffer);
        renderResult.info.file = presetPreviewFileForName(presetName);
        if (writePresetPreviewFile(renderResult.info.file, renderResult.buffer, presetPreviewSampleRate))
            invalidatePresetLibraryCache();
    }

    if (playbackBuffer.getNumSamples() <= 0 || playbackBuffer.getNumChannels() <= 0)
        return false;

    const juce::SpinLock::ScopedLockType lock(presetPreviewLock);
    presetPreviewPlaybackBuffer.makeCopyOf(playbackBuffer);
    presetPreviewPlaybackPosition = 0;
    presetPreviewPlaying.store(true, std::memory_order_release);
    return true;
}

void NateVSTAudioProcessor::stopPresetPreviewPlayback()
{
    const juce::SpinLock::ScopedLockType lock(presetPreviewLock);
    presetPreviewPlaying.store(false, std::memory_order_release);
    presetPreviewPlaybackPosition = 0;
}

bool NateVSTAudioProcessor::isPresetPreviewPlaying() const noexcept
{
    return presetPreviewPlaying.load(std::memory_order_acquire);
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

    const auto saved = saveLibraryState(state);
    if (saved)
        invalidatePresetLibraryCache();
    return saved;
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

    const auto saved = saveLibraryState(state);
    if (saved)
        invalidatePresetLibraryCache();
    return saved;
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

bool NateVSTAudioProcessor::morphPerformanceSnapshots(int leftSlotIndex, int rightSlotIndex, float amount)
{
    if (leftSlotIndex < 0
        || rightSlotIndex < 0
        || leftSlotIndex >= static_cast<int>(performanceSnapshots.size())
        || rightSlotIndex >= static_cast<int>(performanceSnapshots.size()))
        return false;

    const auto& leftSnapshot = performanceSnapshots[static_cast<size_t>(leftSlotIndex)];
    const auto& rightSnapshot = performanceSnapshots[static_cast<size_t>(rightSlotIndex)];
    if (! leftSnapshot.isValid() || ! rightSnapshot.isValid())
        return false;

    const auto blend = juce::jlimit(0.0f, 1.0f, amount);
    for (auto childIndex = 0; childIndex < leftSnapshot.getNumChildren(); ++childIndex)
    {
        const auto leftParameterState = leftSnapshot.getChild(childIndex);
        const auto parameterID = leftParameterState.getProperty("id").toString();
        if (parameterID.isEmpty())
            continue;

        auto* parameter = parameters.getParameter(parameterID);
        if (parameter == nullptr)
            continue;

        const auto rightParameterState = rightSnapshot.getChildWithProperty("id", parameterID);
        if (! rightParameterState.isValid())
            continue;

        const auto leftValue = static_cast<float>(leftParameterState.getProperty("value", getParameterPlainValue(parameterID, 0.0f)));
        const auto rightValue = static_cast<float>(rightParameterState.getProperty("value", leftValue));
        const auto shouldStep = parameter->isDiscrete() || parameter->getNumSteps() <= 128;
        const auto morphedValue = shouldStep
            ? (blend < 0.5f ? leftValue : rightValue)
            : leftValue + ((rightValue - leftValue) * blend);

        setParameterPlainValue(parameterID, morphedValue);
    }

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

void NateVSTAudioProcessor::getOutputSpectrumSnapshot(std::array<float, outputSpectrumSnapshotSize>& destination) const noexcept
{
    const auto writeIndex = outputSpectrumWriteIndex.load(std::memory_order_relaxed);

    for (size_t offset = 0; offset < destination.size(); ++offset)
    {
        const auto sampleIndex = (static_cast<size_t>(writeIndex) + offset) % outputSpectrumSamples.size();
        destination[offset] = outputSpectrumSamples[sampleIndex].load(std::memory_order_relaxed);
    }
}

void NateVSTAudioProcessor::getStereoFieldLevels(float& correlation,
                                                 float& width,
                                                 float& balance,
                                                 float& lowStereoRisk) const noexcept
{
    correlation = stereoFieldCorrelation.load(std::memory_order_relaxed);
    width = stereoFieldWidth.load(std::memory_order_relaxed);
    balance = stereoFieldBalance.load(std::memory_order_relaxed);
    lowStereoRisk = lowEndStereoRisk.load(std::memory_order_relaxed);
}

void NateVSTAudioProcessor::getLowEndMeterLevels(float& subRms, float& lowStereoRisk, float& outputPeak) const noexcept
{
    subRms = lowEndSubRms.load(std::memory_order_relaxed);
    lowStereoRisk = lowEndStereoRisk.load(std::memory_order_relaxed);
    outputPeak = lowEndOutputPeak.load(std::memory_order_relaxed);
}

void NateVSTAudioProcessor::getPumpMeterLevels(float& phase, float& gain, float& reduction, bool& active) const noexcept
{
    effectsRack.getPumpMeterLevels(phase, gain, reduction, active);
}

void NateVSTAudioProcessor::getGuardMeterLevels(float& drive, float& reduction, bool& active) const noexcept
{
    effectsRack.getGuardMeterLevels(drive, reduction, active);
}

void NateVSTAudioProcessor::requestFxSendTailKill() noexcept
{
    effectsRack.requestSendTailKill();
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
    static constexpr auto maxRandomValidationAttempts = 3;
    randomUndoState = snapshot.createCopy();
    randomUndoLabel = randomActionLabel(action);
    hasRandomUndoState = true;
    randomRedoState = {};
    randomRedoLabel.clear();
    hasRandomRedoState = false;

    auto applyAction = [this, action]
    {
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
    };

    RandomValidationResult validation;
    auto acceptedAttempt = 0;

    for (auto attempt = 1; attempt <= maxRandomValidationAttempts; ++attempt)
    {
        if (attempt > 1)
            restorePluginState(snapshot.createCopy());

        applyAction();

        if (mutationScope == RandomMutationScope::sample)
        {
            if (action == RandomAction::wild)
                randomizeUkgVocalChop();
            else
                randomizeSampleCut();
        }

        applyRandomSectionIntensities(snapshot, mutationScope);
        restoreSectionsOutsideMutationScope(snapshot, mutationScope);
        restoreLockedSectionsFromState(snapshot);
        validation = applyRandomGenerationGuardrails(mutationScope);

        if (! validation.shouldRetry)
        {
            acceptedAttempt = attempt;
            break;
        }
    }

    if (acceptedAttempt > 1)
    {
        validation.summary = "auto retry accepted attempt " + juce::String(acceptedAttempt)
            + (validation.summary.isNotEmpty() ? " / " + validation.summary : juce::String());
    }
    else if (acceptedAttempt == 0)
    {
        const auto fallbackValidation = applyRandomExhaustionFallback(snapshot, mutationScope);
        if (! fallbackValidation.shouldRetry)
            acceptedAttempt = maxRandomValidationAttempts + 1;

        validation.summary = "auto retry exhausted " + juce::String(maxRandomValidationAttempts) + " attempts"
            + (fallbackValidation.summary.isNotEmpty() ? " / " + fallbackValidation.summary : juce::String())
            + (validation.summary.isNotEmpty() ? " / last rejected: " + validation.summary : juce::String());
    }

    lastRandomValidationSummary = validation.summary;
    captureRandomCandidateSnapshot(action, mutationScope);
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

juce::String NateVSTAudioProcessor::randomMutationScopeLabel(RandomMutationScope mutationScope)
{
    switch (mutationScope)
    {
        case RandomMutationScope::source: return "Source";
        case RandomMutationScope::envelope: return "Env";
        case RandomMutationScope::filter: return "Filter";
        case RandomMutationScope::sample: return "Sample";
        case RandomMutationScope::effects: return "FX";
        case RandomMutationScope::sequencer: return "Seq";
        case RandomMutationScope::macros: return "Macros";
        case RandomMutationScope::all: return "All";
    }

    return "All";
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

float NateVSTAudioProcessor::readStateParameterValue(const juce::ValueTree& state,
                                                     const char* parameterID,
                                                     float fallback)
{
    const auto parameterState = state.getChildWithProperty("id", parameterID);
    if (parameterState.isValid())
        return static_cast<float>(parameterState.getProperty("value", fallback));

    return fallback;
}

juce::StringArray NateVSTAudioProcessor::getRandomCandidateChangedSections(int slotIndex)
{
    if (! hasRandomCandidate(slotIndex))
        return {};

    const auto& candidate = randomCandidateSnapshots[static_cast<size_t>(slotIndex)];
    const auto current = createPluginState(false);
    juce::StringArray sections;

    auto parameterChanged = [&] (const char* parameterID, float fallback, float threshold = 0.001f)
    {
        return std::abs(readStateParameterValue(candidate.state, parameterID, fallback)
                        - readStateParameterValue(current, parameterID, fallback)) > threshold;
    };

    auto anyChanged = [&] (std::initializer_list<const char*> parameterIDs, float threshold = 0.001f)
    {
        for (const auto* parameterID : parameterIDs)
            if (parameterChanged(parameterID, 0.0f, threshold))
                return true;

        return false;
    };

    auto anyArrayChanged = [&] (const auto& parameterIDs, float threshold = 0.001f)
    {
        for (const auto* parameterID : parameterIDs)
            if (parameterChanged(parameterID, 0.0f, threshold))
                return true;

        return false;
    };

    if (anyChanged({ Parameters::ID::oscWave,
                    Parameters::ID::oscOctave,
                    Parameters::ID::oscTune,
                    Parameters::ID::osc1Level,
                    Parameters::ID::osc2Wave,
                    Parameters::ID::osc2Octave,
                    Parameters::ID::osc2Tune,
                    Parameters::ID::osc2Level,
                    Parameters::ID::subLevel,
                    Parameters::ID::noiseLevel,
                    Parameters::ID::noiseType,
                    Parameters::ID::noiseDecay,
                    Parameters::ID::oscWarp,
                    Parameters::ID::oscWarpB,
                    Parameters::ID::osc2Warp,
                    Parameters::ID::osc2WarpB,
                    Parameters::ID::oscWarpMode,
                    Parameters::ID::oscWarpBMode,
                    Parameters::ID::osc2WarpMode,
                    Parameters::ID::osc2WarpBMode,
                    Parameters::ID::oscWavetablePosition,
                    Parameters::ID::osc2WavetablePosition,
                    Parameters::ID::monoMode,
                    Parameters::ID::glideTime,
                    Parameters::ID::unisonVoices,
                    Parameters::ID::unisonDetune,
                    Parameters::ID::unisonBlend,
                    Parameters::ID::unisonSpread })
        || anyArrayChanged(Parameters::ID::oscCustomWave)
        || anyArrayChanged(Parameters::ID::osc2CustomWave))
        sections.add("Source");

    if (anyChanged({ Parameters::ID::ampAttack,
                    Parameters::ID::ampDecay,
                    Parameters::ID::ampSustain,
                    Parameters::ID::ampRelease,
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
                    Parameters::ID::modEnv1Depth })
        || anyArrayChanged(Parameters::ID::lfo1Curve))
        sections.add("Env");

    if (anyChanged({ Parameters::ID::filterCutoff,
                    Parameters::ID::filterResonance,
                    Parameters::ID::filterEnvAmount,
                    Parameters::ID::filterMode,
                    Parameters::ID::filterCharacter,
                    Parameters::ID::filterSlope,
                    Parameters::ID::driveAmount }))
        sections.add("Filter");

    if (anyChanged({ Parameters::ID::sampleEnabled,
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
                    Parameters::ID::sampleSliceStyle })
        || anyArrayChanged(Parameters::ID::sampleSliceCustom)
        || anyArrayChanged(Parameters::ID::sampleSliceStart)
        || anyArrayChanged(Parameters::ID::sampleSliceEnd)
        || anyArrayChanged(Parameters::ID::sampleSliceReverse)
        || anyArrayChanged(Parameters::ID::sampleSliceTranspose)
        || anyArrayChanged(Parameters::ID::sampleSliceGain)
        || anyArrayChanged(Parameters::ID::sampleSlicePan)
        || anyArrayChanged(Parameters::ID::sampleSliceProbability)
        || anyArrayChanged(Parameters::ID::sampleSliceStutter)
        || anyArrayChanged(Parameters::ID::sampleSliceChoke)
        || anyArrayChanged(Parameters::ID::sampleSliceStutterRepeats)
        || anyArrayChanged(Parameters::ID::sampleSliceNudge)
        || anyArrayChanged(Parameters::ID::sampleSliceFade))
        sections.add("Sample");

    if (anyChanged({ Parameters::ID::fxDistortionEnabled,
                    Parameters::ID::fxDistortionAmount,
                    Parameters::ID::fxDistortionBassSafe,
                    Parameters::ID::fxBitcrushEnabled,
                    Parameters::ID::fxBitcrushBits,
                    Parameters::ID::fxBitcrushDownsample,
                    Parameters::ID::fxBitcrushMix,
                    Parameters::ID::fxPumpEnabled,
                    Parameters::ID::fxPumpRate,
                    Parameters::ID::fxPumpCurve,
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
                    Parameters::ID::fxSendDelay,
                    Parameters::ID::fxReverbEnabled,
                    Parameters::ID::fxReverbSize,
                    Parameters::ID::fxReverbDamping,
                    Parameters::ID::fxReverbMix,
                    Parameters::ID::fxSendReverb,
                    Parameters::ID::fxSendTailKill,
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
                    Parameters::ID::fxGuardGlue,
                    Parameters::ID::fxGuardPunch,
                    Parameters::ID::fxGuardClipMix,
                    Parameters::ID::fxGuardCeiling,
                    Parameters::ID::fxFlangerEnabled,
                    Parameters::ID::fxFlangerRate,
                    Parameters::ID::fxFlangerDepth,
                    Parameters::ID::fxFlangerFeedback,
                    Parameters::ID::fxFlangerMix,
                    Parameters::ID::outputGain })
        || anyArrayChanged(Parameters::ID::fxPumpCustomCurve)
        || anyArrayChanged(Parameters::ID::fxOrder))
        sections.add("FX");

    auto sequencerStepsChanged = [&]
    {
        static constexpr std::array<const char*, 10> suffixes {
            "_enabled",
            "_note",
            "_velocity",
            "_probability",
            "_timing",
            "_length",
            "_lock",
            "_ratchet",
            "_condition",
            "_slide"
        };

        for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
        {
            for (const auto* suffix : suffixes)
            {
                const auto key = "seq_step_" + juce::String(stepIndex) + suffix;
                if (std::abs(static_cast<double>(candidate.state.getProperty(key, 0.0))
                             - static_cast<double>(current.getProperty(key, 0.0))) > 0.001)
                    return true;
            }
        }

        return false;
    };

    if (anyChanged({ Parameters::ID::sequencerEnabled,
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
                    Parameters::ID::sequencerLockDepth })
        || sequencerStepsChanged())
        sections.add("Seq");

    if (anyChanged({ Parameters::ID::macroTone,
                    Parameters::ID::macroDirt,
                    Parameters::ID::macroMotion,
                    Parameters::ID::macroSpace,
                    Parameters::ID::macroWeight,
                    Parameters::ID::macroBounce,
                    Parameters::ID::macroWarp,
                    Parameters::ID::macroThrow }))
        sections.add("Macros");

    return sections;
}

juce::String NateVSTAudioProcessor::currentRandomRecipeName() const
{
    const auto choices = Parameters::randomRecipeChoices();
    if (choices.isEmpty())
        return "Random";

    const auto recipeIndex = juce::jlimit(0,
                                         choices.size() - 1,
                                         juce::roundToInt(getParameterPlainValue(Parameters::ID::randomRecipe, 0.0f)));
    return choices[recipeIndex];
}

NateVSTAudioProcessor::RandomValidationResult NateVSTAudioProcessor::applyRandomGenerationGuardrails(RandomMutationScope)
{
    juce::StringArray fixes;
    const auto recipeName = currentRandomRecipeName();
    const auto bassRole = recipeName.containsIgnoreCase("Bass")
        || recipeName.containsIgnoreCase("Dred")
        || recipeName.containsIgnoreCase("Reece")
        || recipeName.containsIgnoreCase("Rumble")
        || recipeName.containsIgnoreCase("Log")
        || recipeName.containsIgnoreCase("Acid");
    const auto pitchedMidRole = recipeName.containsIgnoreCase("Stab")
        || recipeName.containsIgnoreCase("Organ")
        || recipeName.containsIgnoreCase("Chord")
        || recipeName.containsIgnoreCase("Bell")
        || recipeName.containsIgnoreCase("Pluck")
        || recipeName.containsIgnoreCase("Keys")
        || recipeName.containsIgnoreCase("Pad")
        || recipeName.containsIgnoreCase("Chop")
        || recipeName.containsIgnoreCase("Blip");

    const auto sourceLocked = isRandomLockEnabled(Parameters::ID::randomLockSource);
    const auto sequencerLocked = isRandomLockEnabled(Parameters::ID::randomLockSequencer);
    const auto fxLocked = isRandomLockEnabled(Parameters::ID::randomLockFx);

    const auto osc1 = getParameterPlainValue(Parameters::ID::osc1Level, 1.0f);
    const auto osc2 = getParameterPlainValue(Parameters::ID::osc2Level, 0.0f);
    const auto sub = getParameterPlainValue(Parameters::ID::subLevel, 0.0f);
    const auto noise = getParameterPlainValue(Parameters::ID::noiseLevel, 0.0f);
    const auto sampleContribution = getParameterPlainValue(Parameters::ID::sampleEnabled, 0.0f) > 0.5f
            && samplePlayer.hasSample()
        ? getParameterPlainValue(Parameters::ID::sampleMix, 0.0f)
            * juce::Decibels::decibelsToGain(getParameterPlainValue(Parameters::ID::sampleGain, -6.0f))
        : 0.0f;
    const auto sourceEnergy = osc1 + osc2 + sub + (noise * 0.65f);

    if (! sourceLocked && sourceEnergy < 0.12f && sampleContribution < 0.08f)
    {
        setParameterPlainValue(Parameters::ID::osc1Level, 0.56f);
        setParameterPlainValue(Parameters::ID::ampSustain,
                               juce::jmax(0.28f, getParameterPlainValue(Parameters::ID::ampSustain, 0.55f)));
        setParameterPlainValue(Parameters::ID::ampRelease,
                               juce::jmax(0.08f, getParameterPlainValue(Parameters::ID::ampRelease, 0.18f)));
        fixes.add("source restored");
    }

    if (! sequencerLocked && getParameterPlainValue(Parameters::ID::sequencerEnabled, 0.0f) > 0.5f)
    {
        const auto root = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerRoot, 36.0f));
        const auto octave = juce::roundToInt(getParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f));
        const auto effectiveRoot = root + (octave * 12);

        if (bassRole && (effectiveRoot < 24 || effectiveRoot > 48))
        {
            setParameterPlainValue(Parameters::ID::sequencerRoot,
                                   static_cast<float>(juce::jlimit(36, 48, effectiveRoot)));
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            fixes.add("bass range corrected");
        }
        else if (pitchedMidRole && (effectiveRoot < 48 || effectiveRoot > 76))
        {
            setParameterPlainValue(Parameters::ID::sequencerRoot,
                                   static_cast<float>(juce::jlimit(48, 72, effectiveRoot)));
            setParameterPlainValue(Parameters::ID::sequencerOctave, 0.0f);
            fixes.add("mid range corrected");
        }
    }

    if (bassRole && ! fxLocked)
    {
        if (getParameterPlainValue(Parameters::ID::fxWidthEnabled, 0.0f) > 0.5f
            && getParameterPlainValue(Parameters::ID::fxWidthAmount, 1.0f) > 1.12f)
        {
            setParameterPlainValue(Parameters::ID::fxWidthAmount, 1.12f);
            fixes.add("bass width contained");
        }

        if (getParameterPlainValue(Parameters::ID::fxWidthEnabled, 0.0f) > 0.5f
            && getParameterPlainValue(Parameters::ID::fxWidthMonoCutoff, 120.0f) < 120.0f)
            setParameterPlainValue(Parameters::ID::fxWidthMonoCutoff, 120.0f);
    }

    if (! fxLocked)
    {
        const auto driveLoad = juce::jlimit(0.0f, 1.0f,
            getParameterPlainValue(Parameters::ID::driveAmount, 0.18f)
            + (getParameterPlainValue(Parameters::ID::fxDistortionAmount, 0.0f) * 0.4f)
            + (getParameterPlainValue(Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.12f)
            + (getParameterPlainValue(Parameters::ID::fxGuardPush, 0.0f) * 0.25f));

        if (driveLoad > 0.36f && getParameterPlainValue(Parameters::ID::fxGuardEnabled, 0.0f) < 0.5f)
        {
            setParameterPlainValue(Parameters::ID::fxGuardEnabled, 1.0f);
            fixes.add("Guard enabled");
        }

        const auto maxCeiling = driveLoad > 0.62f ? 0.92f : 0.95f;
        if (getParameterPlainValue(Parameters::ID::fxGuardEnabled, 0.0f) > 0.5f
            && getParameterPlainValue(Parameters::ID::fxGuardCeiling, 0.92f) > maxCeiling)
            setParameterPlainValue(Parameters::ID::fxGuardCeiling, maxCeiling);
    }

    const auto driveForGain = juce::jlimit(0.0f, 1.0f,
        getParameterPlainValue(Parameters::ID::driveAmount, 0.18f)
        + (getParameterPlainValue(Parameters::ID::fxDistortionAmount, 0.0f) * 0.45f)
        + (getParameterPlainValue(Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.10f)
        + (getParameterPlainValue(Parameters::ID::fxGuardPush, 0.0f) * 0.25f));
    const auto maxOutputGain = driveForGain > 0.62f ? -10.0f : -6.0f;
    if (getParameterPlainValue(Parameters::ID::outputGain, -8.0f) > maxOutputGain)
    {
        setParameterPlainValue(Parameters::ID::outputGain, maxOutputGain);
        fixes.add("output clamped");
    }

    if (getParameterPlainValue(Parameters::ID::sampleEnabled, 0.0f) > 0.5f)
    {
        const auto start = getParameterPlainValue(Parameters::ID::sampleStart, 0.0f);
        const auto end = getParameterPlainValue(Parameters::ID::sampleEnd, 1.0f);
        if (end - start < 0.04f)
        {
            const auto safeStart = juce::jlimit(0.0f, 0.92f, start);
            setParameterPlainValue(Parameters::ID::sampleStart, safeStart);
            setParameterPlainValue(Parameters::ID::sampleEnd, juce::jmin(1.0f, safeStart + 0.08f));
            fixes.add("sample window widened");
        }
    }

    const auto renderResult = applyRandomRenderValidation(fxLocked);
    if (renderResult.summary.isNotEmpty())
        fixes.add(renderResult.summary);

    return { fixes.isEmpty() ? juce::String("checked") : fixes.joinIntoString(" / "),
             renderResult.shouldRetry };
}

NateVSTAudioProcessor::RandomValidationResult NateVSTAudioProcessor::applyRandomRenderValidation(bool fxLocked)
{
    static constexpr auto minimumUsefulRenderPeak = 0.01f;
    static constexpr auto minimumUsefulRenderRms = 0.0008f;
    const auto outputLocked = isRandomLockEnabled(Parameters::ID::randomLockOutput);
    juce::StringArray renderNotes;
    auto metrics = renderRandomValidationSnippet();

    if (! metrics.finite || metrics.peak > 1.02f)
    {
        setParameterPlainValue(Parameters::ID::outputGain,
                               juce::jmin(getParameterPlainValue(Parameters::ID::outputGain, -8.0f) - 6.0f, -12.0f));
        if (! fxLocked)
        {
            setParameterPlainValue(Parameters::ID::fxGuardEnabled, 1.0f);
            setParameterPlainValue(Parameters::ID::fxGuardCeiling, 0.9f);
        }
        renderNotes.add(metrics.finite ? "render peak clamped" : "render non-finite reset");
        metrics = renderRandomValidationSnippet();
    }

    if (metrics.rms < minimumUsefulRenderRms || metrics.peak < minimumUsefulRenderPeak)
    {
        if (! isRandomLockEnabled(Parameters::ID::randomLockSource))
        {
            setParameterPlainValue(Parameters::ID::osc1Level,
                                   juce::jmax(0.68f, getParameterPlainValue(Parameters::ID::osc1Level, 0.0f)));
            setParameterPlainValue(Parameters::ID::ampSustain,
                                   juce::jmax(0.35f, getParameterPlainValue(Parameters::ID::ampSustain, 0.55f)));
            setParameterPlainValue(Parameters::ID::ampRelease,
                                   juce::jmax(0.12f, getParameterPlainValue(Parameters::ID::ampRelease, 0.18f)));
            if (! outputLocked)
            {
                setParameterPlainValue(Parameters::ID::outputGain,
                                       juce::jmax(-10.0f, getParameterPlainValue(Parameters::ID::outputGain, -8.0f)));
            }
            renderNotes.add("render quiet boosted");
            metrics = renderRandomValidationSnippet();
        }

        if (metrics.rms < minimumUsefulRenderRms || metrics.peak < minimumUsefulRenderPeak)
            renderNotes.add("render quiet warning");
    }

    if (! fxLocked && metrics.rms > 0.001f && metrics.tailRms > juce::jmax(0.08f, metrics.rms * 0.85f))
    {
        setParameterPlainValue(Parameters::ID::fxDelayFeedback,
                               juce::jmin(0.45f, getParameterPlainValue(Parameters::ID::fxDelayFeedback, 0.35f)));
        setParameterPlainValue(Parameters::ID::fxDelayMix,
                               juce::jmin(0.28f, getParameterPlainValue(Parameters::ID::fxDelayMix, 0.0f)));
        setParameterPlainValue(Parameters::ID::fxReverbMix,
                               juce::jmin(0.32f, getParameterPlainValue(Parameters::ID::fxReverbMix, 0.0f)));
        renderNotes.add("render tail contained");
        metrics = renderRandomValidationSnippet();
    }

    auto shouldRetry = false;
    const auto finalTailTooHeavy = metrics.rms > 0.001f
        && metrics.tailRms > juce::jmax(0.08f, metrics.rms * 0.85f);

    if (! metrics.finite)
    {
        renderNotes.add("render non-finite warning");
        shouldRetry = true;
    }

    if (metrics.peak > 1.02f)
    {
        renderNotes.add("render peak warning");
        shouldRetry = true;
    }

    if (metrics.rms < minimumUsefulRenderRms || metrics.peak < minimumUsefulRenderPeak)
        shouldRetry = true;

    if (finalTailTooHeavy)
    {
        renderNotes.add("render tail warning");
        shouldRetry = true;
    }

    if (! shouldRetry)
    {
        const auto peakDb = juce::Decibels::gainToDecibels(juce::jmax(metrics.peak, 0.000001f));
        renderNotes.add("render ok " + juce::String(peakDb, 1) + "dB");
    }

    return { renderNotes.joinIntoString(" / "), shouldRetry };
}

NateVSTAudioProcessor::RandomValidationResult NateVSTAudioProcessor::applyRandomExhaustionFallback(const juce::ValueTree& originalState,
                                                                                                   RandomMutationScope mutationScope)
{
    restorePluginState(originalState.createCopy());

    juce::StringArray notes;
    const auto recipeName = currentRandomRecipeName();
    const auto sourceLocked = isRandomLockEnabled(Parameters::ID::randomLockSource);
    const auto fxLocked = isRandomLockEnabled(Parameters::ID::randomLockFx);
    const auto outputLocked = isRandomLockEnabled(Parameters::ID::randomLockOutput);
    const auto bassRole = recipeName.containsIgnoreCase("Bass")
        || recipeName.containsIgnoreCase("Dred")
        || recipeName.containsIgnoreCase("Reece")
        || recipeName.containsIgnoreCase("Rumble")
        || recipeName.containsIgnoreCase("Log")
        || recipeName.containsIgnoreCase("Acid");
    const auto midRole = recipeName.containsIgnoreCase("Stab")
        || recipeName.containsIgnoreCase("Organ")
        || recipeName.containsIgnoreCase("Chord")
        || recipeName.containsIgnoreCase("Bell")
        || recipeName.containsIgnoreCase("Pluck")
        || recipeName.containsIgnoreCase("Keys")
        || recipeName.containsIgnoreCase("Pad")
        || recipeName.containsIgnoreCase("Chop")
        || recipeName.containsIgnoreCase("Blip");
    const auto noiseRole = recipeName.containsIgnoreCase("Noise")
        || recipeName.containsIgnoreCase("Perc");

    const auto sourceEnergy = getParameterPlainValue(Parameters::ID::osc1Level, 1.0f)
        + getParameterPlainValue(Parameters::ID::osc2Level, 0.0f)
        + getParameterPlainValue(Parameters::ID::subLevel, 0.0f)
        + (getParameterPlainValue(Parameters::ID::noiseLevel, 0.0f) * 0.65f);

    if (sourceLocked && sourceEnergy < 0.12f)
        return { "fallback blocked by Source lock", true };

    if (! sourceLocked)
    {
        if (mutationScope != RandomMutationScope::all && mutationScope != RandomMutationScope::source)
            notes.add("fallback expanded Source scope");

        if (noiseRole)
        {
            setParameterPlainValue(Parameters::ID::osc1Level, 0.28f);
            setParameterPlainValue(Parameters::ID::osc2Level, 0.12f);
            setParameterPlainValue(Parameters::ID::subLevel, 0.0f);
            setParameterPlainValue(Parameters::ID::noiseLevel, 0.72f);
            setParameterPlainValue(Parameters::ID::noiseType, 5.0f);
            setParameterPlainValue(Parameters::ID::noiseDecay, 0.55f);
            setParameterPlainValue(Parameters::ID::oscWave, 0.0f);
            setParameterPlainValue(Parameters::ID::filterCutoff, 5200.0f);
            notes.add("fallback noise init");
        }
        else if (bassRole)
        {
            setParameterPlainValue(Parameters::ID::osc1Level, 0.76f);
            setParameterPlainValue(Parameters::ID::osc2Level, 0.28f);
            setParameterPlainValue(Parameters::ID::subLevel, 0.52f);
            setParameterPlainValue(Parameters::ID::noiseLevel, 0.0f);
            setParameterPlainValue(Parameters::ID::noiseType, 0.0f);
            setParameterPlainValue(Parameters::ID::noiseDecay, 0.18f);
            setParameterPlainValue(Parameters::ID::oscWave, 1.0f);
            setParameterPlainValue(Parameters::ID::monoMode, 1.0f);
            setParameterPlainValue(Parameters::ID::glideTime, 0.055f);
            setParameterPlainValue(Parameters::ID::filterCutoff, 980.0f);
            setParameterPlainValue(Parameters::ID::filterResonance, 0.42f);
            notes.add("fallback bass init");
        }
        else if (midRole)
        {
            setParameterPlainValue(Parameters::ID::osc1Level, 0.7f);
            setParameterPlainValue(Parameters::ID::osc2Level, 0.34f);
            setParameterPlainValue(Parameters::ID::subLevel, 0.06f);
            setParameterPlainValue(Parameters::ID::noiseLevel, 0.04f);
            setParameterPlainValue(Parameters::ID::noiseType, 4.0f);
            setParameterPlainValue(Parameters::ID::noiseDecay, 0.045f);
            setParameterPlainValue(Parameters::ID::oscWave, 2.0f);
            setParameterPlainValue(Parameters::ID::filterCutoff, 2400.0f);
            setParameterPlainValue(Parameters::ID::filterResonance, 0.38f);
            notes.add("fallback mid init");
        }
        else
        {
            setParameterPlainValue(Parameters::ID::osc1Level, 0.68f);
            setParameterPlainValue(Parameters::ID::osc2Level, 0.18f);
            setParameterPlainValue(Parameters::ID::subLevel, 0.18f);
            setParameterPlainValue(Parameters::ID::noiseLevel, 0.05f);
            setParameterPlainValue(Parameters::ID::noiseType, 3.0f);
            setParameterPlainValue(Parameters::ID::noiseDecay, 0.12f);
            setParameterPlainValue(Parameters::ID::filterCutoff, 1800.0f);
            notes.add("fallback safe init");
        }

        setParameterPlainValue(Parameters::ID::ampAttack, 0.004f);
        setParameterPlainValue(Parameters::ID::ampDecay, bassRole ? 0.24f : 0.18f);
        setParameterPlainValue(Parameters::ID::ampSustain, bassRole ? 0.52f : 0.42f);
        setParameterPlainValue(Parameters::ID::ampRelease, bassRole ? 0.16f : 0.2f);
        setParameterPlainValue(Parameters::ID::driveAmount, bassRole ? 0.18f : 0.12f);
        setParameterPlainValue(Parameters::ID::macroWeight, bassRole ? 0.32f : 0.12f);
    }

    if (outputLocked)
        notes.add("fallback relaxed Output lock");
    setParameterPlainValue(Parameters::ID::outputGain, bassRole ? -10.0f : -8.0f);

    if (fxLocked)
        notes.add("fallback relaxed FX lock");
    setParameterPlainValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::fxGuardCeiling, 0.9f);
    setParameterPlainValue(Parameters::ID::fxDelayMix, juce::jmin(0.2f, getParameterPlainValue(Parameters::ID::fxDelayMix, 0.0f)));
    setParameterPlainValue(Parameters::ID::fxDelayFeedback, juce::jmin(0.35f, getParameterPlainValue(Parameters::ID::fxDelayFeedback, 0.25f)));
    setParameterPlainValue(Parameters::ID::fxReverbMix, juce::jmin(0.24f, getParameterPlainValue(Parameters::ID::fxReverbMix, 0.0f)));
    setParameterPlainValue(Parameters::ID::fxWidthAmount, bassRole ? juce::jmin(1.08f, getParameterPlainValue(Parameters::ID::fxWidthAmount, 1.0f))
                                                                  : getParameterPlainValue(Parameters::ID::fxWidthAmount, 1.0f));

    const auto validation = applyRandomRenderValidation(false);
    if (validation.shouldRetry)
        notes.add("fallback still rejected");
    else
        notes.add("fallback accepted");

    if (validation.summary.isNotEmpty())
        notes.add(validation.summary);

    return { notes.joinIntoString(" / "), validation.shouldRetry };
}

NateVSTAudioProcessor::RandomRenderMetrics NateVSTAudioProcessor::renderRandomValidationSnippet()
{
    static constexpr auto validationNote = 48;
    const auto sampleRate = meterSampleRate > 0.0 ? meterSampleRate : 44100.0;
    const auto blockSize = juce::jmax(256, preparedSamplesPerBlock);
    const auto outputChannels = getTotalNumOutputChannels();
    const auto channelCount = outputChannels > 0 ? outputChannels : 2;
    const auto noteLengthSamples = juce::jmax(1, static_cast<int>(std::round(sampleRate * 0.18)));
    const auto totalSamples = juce::jmax(blockSize, static_cast<int>(std::round(sampleRate * 0.42)));
    const auto tailStartSample = juce::jlimit(0, totalSamples - 1, totalSamples - static_cast<int>(std::round(sampleRate * 0.08)));

    synthEngine.prepare(sampleRate, blockSize);
    samplePlayer.prepare(sampleRate);
    patternSequencer.prepare(sampleRate);
    effectsRack.prepare(sampleRate, blockSize, channelCount);
    resetRandomValidationRenderState();

    RandomRenderMetrics metrics;
    double sumSquares = 0.0;
    double tailSquares = 0.0;
    int sampleCount = 0;
    int tailSampleCount = 0;

    for (auto renderedSamples = 0; renderedSamples < totalSamples; renderedSamples += blockSize)
    {
        const auto currentBlockSize = juce::jmin(blockSize, totalSamples - renderedSamples);
        juce::AudioBuffer<float> buffer(channelCount, currentBlockSize);
        juce::MidiBuffer midi;

        if (renderedSamples == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, validationNote, static_cast<juce::uint8>(96)), 0);

        if (renderedSamples <= noteLengthSamples && noteLengthSamples < renderedSamples + currentBlockSize)
            midi.addEvent(juce::MidiMessage::noteOff(1, validationNote), noteLengthSamples - renderedSamples);

        processBlock(buffer, midi);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < currentBlockSize; ++sample)
            {
                const auto value = samples[sample];
                if (! std::isfinite(value))
                    metrics.finite = false;

                const auto absolute = std::abs(value);
                metrics.peak = juce::jmax(metrics.peak, absolute);
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;

                if (renderedSamples + sample >= tailStartSample)
                {
                    tailSquares += static_cast<double>(value) * static_cast<double>(value);
                    ++tailSampleCount;
                }
            }
        }
    }

    metrics.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    metrics.tailRms = tailSampleCount > 0 ? static_cast<float>(std::sqrt(tailSquares / static_cast<double>(tailSampleCount))) : 0.0f;
    resetRandomValidationRenderState();
    return metrics;
}

void NateVSTAudioProcessor::resetRandomValidationRenderState()
{
    midiKeyboardState.allNotesOff(0);
    synthEngine.allNotesOff();
    samplePlayer.stopAllVoices();
    effectsRack.reset();
    patternSequencer.reset();
    clearChordMemoryActiveNotes();
    panicRequested.store(false, std::memory_order_release);
}

void NateVSTAudioProcessor::captureRandomCandidateSnapshot(RandomAction action, RandomMutationScope mutationScope)
{
    const auto slot = static_cast<size_t>(juce::jlimit(0,
                                                       static_cast<int>(randomCandidateSnapshots.size()) - 1,
                                                       nextRandomCandidateSlot));
    auto& candidate = randomCandidateSnapshots[slot];
    candidate.state = createPluginState(false);
    candidate.label = randomActionLabel(action) + ": " + currentRandomRecipeName();
    candidate.validationSummary = lastRandomValidationSummary;
    if (mutationScope != RandomMutationScope::all)
        candidate.label += " / " + randomMutationScopeLabel(mutationScope);
    candidate.valid = true;

    activeRandomCandidateSlot = static_cast<int>(slot);
    nextRandomCandidateSlot = (static_cast<int>(slot) + 1) % static_cast<int>(randomCandidateSnapshots.size());
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

void NateVSTAudioProcessor::blendParameterFromState(const juce::ValueTree& state,
                                                     const juce::String& parameterID,
                                                     float intensity)
{
    intensity = juce::jlimit(0.0f, 1.0f, intensity);
    if (intensity >= 0.995f)
        return;

    const auto parameterState = state.getChildWithProperty("id", parameterID);
    if (! parameterState.isValid())
        return;

    const auto current = getParameterPlainValue(parameterID, 0.0f);
    const auto previous = static_cast<float>(parameterState.getProperty("value", current));
    setParameterPlainValue(parameterID, previous + ((current - previous) * intensity));
}

void NateVSTAudioProcessor::blendParameterGroupFromState(const juce::ValueTree& state,
                                                          std::initializer_list<const char*> parameterIDs,
                                                          float intensity)
{
    for (const auto* parameterID : parameterIDs)
        blendParameterFromState(state, parameterID, intensity);
}

void NateVSTAudioProcessor::restoreDiscreteParameterFromStateIfNeeded(const juce::ValueTree& state,
                                                                       const juce::String& parameterID,
                                                                       float intensity)
{
    if (juce::jlimit(0.0f, 1.0f, intensity) < 0.5f)
        restoreParameterFromState(state, parameterID);
}

void NateVSTAudioProcessor::restoreDiscreteParameterGroupFromStateIfNeeded(const juce::ValueTree& state,
                                                                            std::initializer_list<const char*> parameterIDs,
                                                                            float intensity)
{
    for (const auto* parameterID : parameterIDs)
        restoreDiscreteParameterFromStateIfNeeded(state, parameterID, intensity);
}

float NateVSTAudioProcessor::randomSectionIntensity(RandomMutationScope mutationScope) const
{
    switch (mutationScope)
    {
        case RandomMutationScope::source:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomSourceIntensity, 1.0f));
        case RandomMutationScope::envelope:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomEnvelopeIntensity, 1.0f));
        case RandomMutationScope::filter:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomFilterIntensity, 1.0f));
        case RandomMutationScope::sample:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomSampleIntensity, 1.0f));
        case RandomMutationScope::effects:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomFxIntensity, 1.0f));
        case RandomMutationScope::sequencer:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomSequencerIntensity, 1.0f));
        case RandomMutationScope::macros:
            return juce::jlimit(0.0f, 1.0f, getParameterPlainValue(Parameters::ID::randomMacroIntensity, 1.0f));
        case RandomMutationScope::all:
            break;
    }

    return 1.0f;
}

void NateVSTAudioProcessor::applyRandomSectionIntensities(const juce::ValueTree& state,
                                                           RandomMutationScope mutationScope)
{
    if (mutationScope != RandomMutationScope::all)
    {
        applyRandomSectionIntensity(state, mutationScope, randomSectionIntensity(mutationScope));
        return;
    }

    for (auto scopeIndex = 1; scopeIndex <= 7; ++scopeIndex)
    {
        const auto section = randomMutationScopeFromIndex(scopeIndex);
        applyRandomSectionIntensity(state, section, randomSectionIntensity(section));
    }
}

void NateVSTAudioProcessor::applyRandomSectionIntensity(const juce::ValueTree& state,
                                                         RandomMutationScope mutationScope,
                                                         float intensity)
{
    intensity = juce::jlimit(0.0f, 1.0f, intensity);
    if (intensity >= 0.995f)
        return;

    auto blendArray = [&] (const auto& parameterIDs)
    {
        for (const auto* parameterID : parameterIDs)
            blendParameterFromState(state, parameterID, intensity);
    };

    auto restoreDiscreteArray = [&] (const auto& parameterIDs)
    {
        for (const auto* parameterID : parameterIDs)
            restoreDiscreteParameterFromStateIfNeeded(state, parameterID, intensity);
    };

    switch (mutationScope)
    {
        case RandomMutationScope::source:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::oscWave,
                Parameters::ID::oscOctave,
                Parameters::ID::osc2Wave,
                Parameters::ID::osc2Octave,
                Parameters::ID::noiseType,
                Parameters::ID::oscWarpMode,
                Parameters::ID::oscWarpBMode,
                Parameters::ID::osc2WarpMode,
                Parameters::ID::osc2WarpBMode,
                Parameters::ID::monoMode,
                Parameters::ID::unisonVoices
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::oscTune,
                Parameters::ID::osc1Level,
                Parameters::ID::osc2Tune,
                Parameters::ID::osc2Level,
                Parameters::ID::subLevel,
                Parameters::ID::noiseLevel,
                Parameters::ID::noiseDecay,
                Parameters::ID::oscWarp,
                Parameters::ID::oscWarpB,
                Parameters::ID::osc2Warp,
                Parameters::ID::osc2WarpB,
                Parameters::ID::oscWavetablePosition,
                Parameters::ID::osc2WavetablePosition,
                Parameters::ID::glideTime,
                Parameters::ID::unisonDetune,
                Parameters::ID::unisonBlend,
                Parameters::ID::unisonSpread
            }, intensity);
            blendArray(Parameters::ID::oscCustomWave);
            blendArray(Parameters::ID::osc2CustomWave);
            break;

        case RandomMutationScope::envelope:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::lfo1Sync,
                Parameters::ID::lfo1SyncRate,
                Parameters::ID::lfo1Shape,
                Parameters::ID::lfo1Retrigger,
                Parameters::ID::lfo2Sync,
                Parameters::ID::lfo2SyncRate,
                Parameters::ID::lfo2Shape,
                Parameters::ID::lfo2Retrigger,
                Parameters::ID::stepLfoSync,
                Parameters::ID::stepLfoSyncRate
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::ampAttack,
                Parameters::ID::ampDecay,
                Parameters::ID::ampSustain,
                Parameters::ID::ampRelease,
                Parameters::ID::lfo1Rate,
                Parameters::ID::lfo1Depth,
                Parameters::ID::lfo1Phase,
                Parameters::ID::lfo2Rate,
                Parameters::ID::lfo2Depth,
                Parameters::ID::lfo2Phase,
                Parameters::ID::stepLfoRate,
                Parameters::ID::stepLfoDepth,
                Parameters::ID::stepLfoSlew,
                Parameters::ID::modEnv1Attack,
                Parameters::ID::modEnv1Decay,
                Parameters::ID::modEnv1Sustain,
                Parameters::ID::modEnv1Release,
                Parameters::ID::modEnv1Depth
            }, intensity);
            blendArray(Parameters::ID::lfo1Curve);
            blendArray(Parameters::ID::stepLfoValue);
            blendArray(Parameters::ID::modMatrixAmount);
            blendArray(Parameters::ID::modMatrixRangeMin);
            blendArray(Parameters::ID::modMatrixRangeMax);
            blendArray(Parameters::ID::modMatrixSlew);
            restoreDiscreteArray(Parameters::ID::modMatrixSource);
            restoreDiscreteArray(Parameters::ID::modMatrixDestination);
            restoreDiscreteArray(Parameters::ID::modMatrixEnabled);
            restoreDiscreteArray(Parameters::ID::modMatrixPolarity);
            restoreDiscreteArray(Parameters::ID::modMatrixCurve);
            break;

        case RandomMutationScope::filter:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::filterMode,
                Parameters::ID::filterCharacter,
                Parameters::ID::filterSlope
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::filterCutoff,
                Parameters::ID::filterResonance,
                Parameters::ID::filterEnvAmount,
                Parameters::ID::driveAmount
            }, intensity);
            break;

        case RandomMutationScope::sample:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::sampleEnabled,
                Parameters::ID::sampleReverse,
                Parameters::ID::samplePlaybackMode,
                Parameters::ID::sampleStutterEnabled,
                Parameters::ID::sampleStutterRate,
                Parameters::ID::sampleSliceStyle
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::sampleStart,
                Parameters::ID::sampleEnd,
                Parameters::ID::sampleTranspose,
                Parameters::ID::samplePitchRamp,
                Parameters::ID::sampleGain,
                Parameters::ID::sampleMix,
                Parameters::ID::sampleStutterRepeats
            }, intensity);
            restoreDiscreteArray(Parameters::ID::sampleSliceCustom);
            blendArray(Parameters::ID::sampleSliceStart);
            blendArray(Parameters::ID::sampleSliceEnd);
            restoreDiscreteArray(Parameters::ID::sampleSliceReverse);
            blendArray(Parameters::ID::sampleSliceTranspose);
            blendArray(Parameters::ID::sampleSliceGain);
            blendArray(Parameters::ID::sampleSlicePan);
            blendArray(Parameters::ID::sampleSliceProbability);
            restoreDiscreteArray(Parameters::ID::sampleSliceStutter);
            restoreDiscreteArray(Parameters::ID::sampleSliceChoke);
            blendArray(Parameters::ID::sampleSliceStutterRepeats);
            blendArray(Parameters::ID::sampleSliceNudge);
            blendArray(Parameters::ID::sampleSliceFade);
            break;

        case RandomMutationScope::effects:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::fxDistortionEnabled,
                Parameters::ID::fxBitcrushEnabled,
                Parameters::ID::fxPumpEnabled,
                Parameters::ID::fxPumpRate,
                Parameters::ID::fxPumpCurve,
                Parameters::ID::fxTremoloEnabled,
                Parameters::ID::fxTremoloRate,
                Parameters::ID::fxRingEnabled,
                Parameters::ID::fxCombEnabled,
                Parameters::ID::fxChorusEnabled,
                Parameters::ID::fxDelayEnabled,
                Parameters::ID::fxDelaySync,
                Parameters::ID::fxDelayRate,
                Parameters::ID::fxSendTailKill,
                Parameters::ID::fxReverbEnabled,
                Parameters::ID::fxWidthEnabled,
                Parameters::ID::fxToneEnabled,
                Parameters::ID::fxEqEnabled,
                Parameters::ID::fxPhaserEnabled,
                Parameters::ID::fxFlangerEnabled,
                Parameters::ID::fxGuardEnabled
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::fxDistortionAmount,
                Parameters::ID::fxDistortionBassSafe,
                Parameters::ID::fxBitcrushBits,
                Parameters::ID::fxBitcrushDownsample,
                Parameters::ID::fxBitcrushMix,
                Parameters::ID::fxPumpDepth,
                Parameters::ID::fxPumpShape,
                Parameters::ID::fxPumpPhase,
                Parameters::ID::fxTremoloDepth,
                Parameters::ID::fxTremoloPan,
                Parameters::ID::fxTremoloShape,
                Parameters::ID::fxTremoloPhase,
                Parameters::ID::fxRingFrequency,
                Parameters::ID::fxRingDepth,
                Parameters::ID::fxRingMix,
                Parameters::ID::fxRingBias,
                Parameters::ID::fxCombFrequency,
                Parameters::ID::fxCombFeedback,
                Parameters::ID::fxCombDamping,
                Parameters::ID::fxCombMix,
                Parameters::ID::fxChorusRate,
                Parameters::ID::fxChorusDepth,
                Parameters::ID::fxChorusMix,
                Parameters::ID::fxDelayTime,
                Parameters::ID::fxDelayFeedback,
                Parameters::ID::fxDelayMix,
                Parameters::ID::fxSendDelay,
                Parameters::ID::fxReverbSize,
                Parameters::ID::fxReverbDamping,
                Parameters::ID::fxReverbMix,
                Parameters::ID::fxSendReverb,
                Parameters::ID::fxWidthAmount,
                Parameters::ID::fxWidthMonoCutoff,
                Parameters::ID::fxToneTilt,
                Parameters::ID::fxToneLowCut,
                Parameters::ID::fxEqLowGain,
                Parameters::ID::fxEqMidGain,
                Parameters::ID::fxEqHighGain,
                Parameters::ID::fxEqTrim,
                Parameters::ID::fxPhaserRate,
                Parameters::ID::fxPhaserDepth,
                Parameters::ID::fxPhaserMix,
                Parameters::ID::fxFlangerRate,
                Parameters::ID::fxFlangerDepth,
                Parameters::ID::fxFlangerFeedback,
                Parameters::ID::fxFlangerMix,
                Parameters::ID::fxGuardPush,
                Parameters::ID::fxGuardGlue,
                Parameters::ID::fxGuardPunch,
                Parameters::ID::fxGuardClipMix,
                Parameters::ID::fxGuardCeiling,
                Parameters::ID::outputGain
            }, intensity);
            blendArray(Parameters::ID::fxPumpCustomCurve);
            restoreDiscreteArray(Parameters::ID::fxOrder);
            break;

        case RandomMutationScope::sequencer:
            restoreDiscreteParameterGroupFromStateIfNeeded(state, {
                Parameters::ID::sequencerEnabled,
                Parameters::ID::sequencerRate,
                Parameters::ID::sequencerRoot,
                Parameters::ID::sequencerGrooveMode,
                Parameters::ID::sequencerScale,
                Parameters::ID::sequencerChordMode,
                Parameters::ID::sequencerChordVoicing,
                Parameters::ID::sequencerChordMemory,
                Parameters::ID::sequencerOctave,
                Parameters::ID::sequencerLockDestination
            }, intensity);
            blendParameterGroupFromState(state, {
                Parameters::ID::sequencerGate,
                Parameters::ID::sequencerSwing,
                Parameters::ID::sequencerChordStrum,
                Parameters::ID::sequencerAccent,
                Parameters::ID::sequencerProbability,
                Parameters::ID::sequencerRandomAmount,
                Parameters::ID::sequencerLockDepth
            }, intensity);
            if (intensity < 0.5f)
                restoreSequencerFromState(state);
            break;

        case RandomMutationScope::macros:
            blendParameterGroupFromState(state, {
                Parameters::ID::macroTone,
                Parameters::ID::macroDirt,
                Parameters::ID::macroMotion,
                Parameters::ID::macroSpace,
                Parameters::ID::macroWeight,
                Parameters::ID::macroBounce,
                Parameters::ID::macroWarp,
                Parameters::ID::macroThrow
            }, intensity);
            break;

        case RandomMutationScope::all:
            break;
    }
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
                Parameters::ID::noiseType,
                Parameters::ID::noiseDecay,
                Parameters::ID::oscWarp,
                Parameters::ID::oscWarpB,
                Parameters::ID::osc2Warp,
                Parameters::ID::osc2WarpB,
                Parameters::ID::oscWarpMode,
                Parameters::ID::oscWarpBMode,
                Parameters::ID::osc2WarpMode,
                Parameters::ID::osc2WarpBMode,
                Parameters::ID::oscWavetablePosition,
                Parameters::ID::osc2WavetablePosition,
                Parameters::ID::monoMode,
                Parameters::ID::glideTime,
                Parameters::ID::unisonVoices,
                Parameters::ID::unisonDetune,
                Parameters::ID::unisonBlend,
                Parameters::ID::unisonSpread
            });

            for (const auto* parameterID : Parameters::ID::oscCustomWave)
                restoreParameterFromState(state, parameterID);

            for (const auto* parameterID : Parameters::ID::osc2CustomWave)
                restoreParameterFromState(state, parameterID);
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
                Parameters::ID::fxDistortionBassSafe,
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
                Parameters::ID::fxSendDelay,
                Parameters::ID::fxReverbEnabled,
                Parameters::ID::fxReverbSize,
                Parameters::ID::fxReverbDamping,
                Parameters::ID::fxReverbMix,
                Parameters::ID::fxSendReverb,
                Parameters::ID::fxSendTailKill,
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
                Parameters::ID::fxGuardGlue,
                Parameters::ID::fxGuardPunch,
                Parameters::ID::fxGuardClipMix,
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
        Parameters::ID::stepLfoSync,
        Parameters::ID::stepLfoSyncRate,
        Parameters::ID::stepLfoRate,
        Parameters::ID::stepLfoDepth,
        Parameters::ID::stepLfoSlew,
        Parameters::ID::modEnv1Attack,
        Parameters::ID::modEnv1Decay,
        Parameters::ID::modEnv1Sustain,
        Parameters::ID::modEnv1Release,
        Parameters::ID::modEnv1Depth
    });

    for (const auto* parameterID : Parameters::ID::lfo1Curve)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::stepLfoValue)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixSource)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixDestination)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixAmount)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixEnabled)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixPolarity)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixCurve)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixRangeMin)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixRangeMax)
        restoreParameterFromState(state, parameterID);

    for (const auto* parameterID : Parameters::ID::modMatrixSlew)
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
            Parameters::ID::noiseType,
            Parameters::ID::noiseDecay,
            Parameters::ID::oscWarp,
            Parameters::ID::oscWarpB,
            Parameters::ID::osc2Warp,
            Parameters::ID::osc2WarpB,
            Parameters::ID::oscWarpMode,
            Parameters::ID::oscWarpBMode,
            Parameters::ID::osc2WarpMode,
            Parameters::ID::osc2WarpBMode,
            Parameters::ID::oscWavetablePosition,
            Parameters::ID::osc2WavetablePosition,
            Parameters::ID::unisonVoices,
            Parameters::ID::unisonDetune,
            Parameters::ID::unisonBlend,
            Parameters::ID::unisonSpread
        });

        for (const auto* parameterID : Parameters::ID::oscCustomWave)
            restoreParameterFromState(state, parameterID);

        for (const auto* parameterID : Parameters::ID::osc2CustomWave)
            restoreParameterFromState(state, parameterID);
    }

    if (isRandomLockEnabled(Parameters::ID::randomLockSample))
        restoreSampleFromState(state);

    if (isRandomLockEnabled(Parameters::ID::randomLockFx))
    {
        restoreParameterGroupFromState(state, {
            Parameters::ID::fxDistortionEnabled,
            Parameters::ID::fxDistortionAmount,
            Parameters::ID::fxDistortionBassSafe,
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
            Parameters::ID::fxSendDelay,
            Parameters::ID::fxReverbEnabled,
            Parameters::ID::fxReverbSize,
            Parameters::ID::fxReverbDamping,
            Parameters::ID::fxReverbMix,
            Parameters::ID::fxSendReverb,
            Parameters::ID::fxSendTailKill,
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
            Parameters::ID::fxGuardGlue,
            Parameters::ID::fxGuardPunch,
            Parameters::ID::fxGuardClipMix,
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

bool NateVSTAudioProcessor::restoreSampleFileReference(const juce::String& samplePath)
{
    loadedSamplePath = samplePath;

    if (loadedSamplePath.isEmpty())
    {
        samplePlayer.clear();
        return false;
    }

    if (samplePlayer.loadFile(juce::File(loadedSamplePath)))
        return true;

    samplePlayer.clear();
    return false;
}

void NateVSTAudioProcessor::restoreSampleFromState(const juce::ValueTree& state)
{
    restoreSampleFileReference(state.getProperty("sample_file").toString());

    if (! state.getChildWithProperty("id", Parameters::ID::sampleSliceStyle).isValid())
        setParameterPlainValue(Parameters::ID::sampleSliceStyle, 0.0f);
    const auto hasSampleSliceRegions = state.getChildWithProperty("id", Parameters::ID::sampleSliceStart[0]).isValid()
        && state.getChildWithProperty("id", Parameters::ID::sampleSliceEnd[0]).isValid();
    const auto hasSampleSlicePanProbability = state.getChildWithProperty("id", Parameters::ID::sampleSlicePan[0]).isValid()
        && state.getChildWithProperty("id", Parameters::ID::sampleSliceProbability[0]).isValid();
    const auto hasSampleSliceNudgeFade = state.getChildWithProperty("id", Parameters::ID::sampleSliceNudge[0]).isValid()
        && state.getChildWithProperty("id", Parameters::ID::sampleSliceFade[0]).isValid();

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
    for (const auto* parameterID : Parameters::ID::sampleSliceNudge)
        restoreParameterFromState(state, parameterID);
    for (const auto* parameterID : Parameters::ID::sampleSliceFade)
        restoreParameterFromState(state, parameterID);
    if (! hasSampleSliceNudgeFade)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSliceNudge.size(); ++index)
        {
            setParameterPlainValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceFade[index], 0.0f);
        }
    }
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
        step.ratchet = static_cast<int>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_ratchet", 1));
        step.condition = static_cast<int>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_condition", 0));
        step.slide = static_cast<bool>(state.getProperty("seq_step_" + juce::String(stepIndex) + "_slide", false));
        patternSequencer.setStep(stepIndex, step);
    }
}

juce::ValueTree NateVSTAudioProcessor::createSequencerSceneState() const
{
    auto scene = juce::ValueTree(parameters.state.getType());
    scene.setProperty("nate_vst_scene_version", 1, nullptr);

    for (const auto* parameterID : sequencerSceneParameterIDs())
    {
        juce::ValueTree parameterChild("PARAM");
        parameterChild.setProperty("id", parameterID, nullptr);
        parameterChild.setProperty("value", getParameterPlainValue(parameterID, 0.0f), nullptr);
        scene.addChild(parameterChild, -1, nullptr);
    }

    for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
    {
        const auto step = patternSequencer.getStep(stepIndex);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_enabled", step.enabled, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_note", step.noteOffset, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_velocity", step.velocity, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_probability", step.probability, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_timing", step.timing, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_length", step.length, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_lock", step.lock, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_ratchet", step.ratchet, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_condition", step.condition, nullptr);
        scene.setProperty("seq_step_" + juce::String(stepIndex) + "_slide", step.slide, nullptr);
    }

    return scene;
}

void NateVSTAudioProcessor::restoreSequencerScenesFromState(const juce::ValueTree& state)
{
    for (auto& scene : sequencerPatternScenes)
        scene = {};

    for (auto childIndex = 0; childIndex < state.getNumChildren(); ++childIndex)
    {
        const auto child = state.getChild(childIndex);
        if (! child.hasType(sequencerPatternSceneType))
            continue;

        const auto slotIndex = static_cast<int>(child.getProperty("slot", -1));
        if (slotIndex < 0 || slotIndex >= static_cast<int>(sequencerPatternScenes.size()) || child.getNumChildren() <= 0)
            continue;

        auto scene = child.getChild(0).createCopy();
        removePerformanceSnapshotChildren(scene);
        removeSequencerSceneChildren(scene);

        if (scene.isValid() && scene.hasType(parameters.state.getType()))
            sequencerPatternScenes[static_cast<size_t>(slotIndex)] = scene;
    }

    refreshSequencerSceneChainPlayback();
}

void NateVSTAudioProcessor::appendSequencerScenesToState(juce::ValueTree& state) const
{
    for (size_t slotIndex = 0; slotIndex < sequencerPatternScenes.size(); ++slotIndex)
    {
        const auto& scene = sequencerPatternScenes[slotIndex];
        if (! scene.isValid())
            continue;

        juce::ValueTree child(sequencerPatternSceneType);
        child.setProperty("slot", static_cast<int>(slotIndex), nullptr);
        child.setProperty("label", sequencerPatternSceneLabels()[slotIndex], nullptr);
        child.addChild(scene.createCopy(), -1, nullptr);
        state.addChild(child, -1, nullptr);
    }
}

void NateVSTAudioProcessor::removeSequencerSceneChildren(juce::ValueTree& state) const
{
    for (auto childIndex = state.getNumChildren(); --childIndex >= 0;)
        if (state.getChild(childIndex).hasType(sequencerPatternSceneType))
            state.removeChild(childIndex, nullptr);
}

std::vector<juce::ValueTree> NateVSTAudioProcessor::createSequencerSceneChainSegments(int clipBars) const
{
    std::vector<juce::ValueTree> capturedScenes;
    capturedScenes.reserve(sequencerPatternScenes.size());

    for (const auto& scene : sequencerPatternScenes)
        if (scene.isValid())
            capturedScenes.push_back(scene);

    if (capturedScenes.empty())
        capturedScenes.push_back(createSequencerSceneState());

    const auto safeClipBars = sanitiseSequencerSceneChainClipBars(clipBars);
    if (safeClipBars <= 0)
        return capturedScenes;

    std::vector<juce::ValueTree> segments;
    segments.reserve(static_cast<size_t>(safeClipBars));
    for (auto segmentIndex = 0; segmentIndex < safeClipBars; ++segmentIndex)
        segments.push_back(capturedScenes[static_cast<size_t>(segmentIndex % static_cast<int>(capturedScenes.size()))]);

    return segments;
}

void NateVSTAudioProcessor::refreshSequencerSceneChainPlayback()
{
    if (! isSequencerSceneChainPlaybackEnabled())
    {
        patternSequencer.clearSceneChain();
        return;
    }

    Sequencer::PatternSequencer::SceneChain sceneChain {};
    Sequencer::PatternSequencer::SceneControlChain sceneControls {};
    const auto segments = createSequencerSceneChainSegments(getSequencerSceneChainClipBars());
    const auto sceneCount = juce::jmin(static_cast<int>(segments.size()), Sequencer::PatternSequencer::maxSceneChainLength);
    for (auto sceneIndex = 0; sceneIndex < sceneCount; ++sceneIndex)
    {
        const auto& scene = segments[static_cast<size_t>(sceneIndex)];
        if (! scene.isValid())
            continue;

        sceneControls[static_cast<size_t>(sceneIndex)] = readSequencerSceneControls(scene);

        for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
            sceneChain[static_cast<size_t>(sceneIndex)][static_cast<size_t>(stepIndex)] = readSequencerExportStep(scene, stepIndex);
    }

    patternSequencer.setSceneChain(sceneChain, sceneControls, sceneCount);
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

void NateVSTAudioProcessor::resetSampleParametersForNewSource(const juce::String& path)
{
    loadedSamplePath = path;
    setParameterPlainValue(Parameters::ID::sampleEnabled, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleStart, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleEnd, 1.0f);
    setParameterPlainValue(Parameters::ID::samplePlaybackMode, 1.0f);
    setParameterPlainValue(Parameters::ID::sampleEngineMode, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleGrainSize, 0.08f);
    setParameterPlainValue(Parameters::ID::sampleGrainSpray, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleSpectralFreeze, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleReverse, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleTranspose, 0.0f);
    setParameterPlainValue(Parameters::ID::samplePitchRamp, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleGain, -6.0f);
    setParameterPlainValue(Parameters::ID::sampleMix, 0.75f);
    setParameterPlainValue(Parameters::ID::sampleStutterEnabled, 0.0f);
    setParameterPlainValue(Parameters::ID::sampleStutterRepeats, 3.0f);
    setParameterPlainValue(Parameters::ID::sampleSliceStyle, 0.0f);
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
        setParameterPlainValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSliceFade[index], 0.0f);
    }
}

void NateVSTAudioProcessor::updateSampleCaptureSourcePeak(const juce::AudioBuffer<float>& buffer,
                                                          int sourceChannelLimit) noexcept
{
    const auto peak = calculateBufferPeak(buffer, sourceChannelLimit);
    const auto previous = sampleCaptureSourcePeak.load(std::memory_order_relaxed);
    sampleCaptureSourcePeak.store(juce::jmax(peak, previous * 0.86f), std::memory_order_release);
}

float NateVSTAudioProcessor::getSampleCaptureThresholdGain() const noexcept
{
    return juce::Decibels::decibelsToGain(getSampleCaptureThresholdDb());
}

int NateVSTAudioProcessor::calculateSampleCaptureTargetSamples() const
{
    const auto bars = [this]() noexcept
    {
        switch (getSampleCaptureLengthModeIndex())
        {
            case 1: return 1.0;
            case 2: return 2.0;
            case 3: return 4.0;
            case 4: return 8.0;
            default: break;
        }

        return 0.0;
    }();

    const auto& captureBuffer = getActiveSampleCaptureBuffer();
    if (bars <= 0.0 || captureBuffer.getNumSamples() <= 0)
        return 0;

    const auto bpm = juce::jlimit(30.0, 300.0, getHostBpm());
    const auto sampleRate = sampleCaptureSampleRate > 0.0 ? sampleCaptureSampleRate : 44100.0;
    const auto seconds = (bars * 4.0 * 60.0) / bpm;
    const auto targetSamples = static_cast<int>(std::round(seconds * sampleRate));
    return juce::jlimit(64, captureBuffer.getNumSamples(), targetSamples);
}

int NateVSTAudioProcessor::calculateSampleCapturePreRollSamples() const noexcept
{
    if (sampleCapturePreRollBuffer.getNumSamples() <= 0)
        return 0;

    const auto seconds = getSampleCapturePreRollDurationSeconds();
    if (seconds <= 0.0f)
        return 0;

    const auto sampleRate = sampleCaptureSampleRate > 0.0 ? sampleCaptureSampleRate : 44100.0;
    const auto preRollSamples = static_cast<int>(std::round(static_cast<double>(seconds) * sampleRate));
    return juce::jlimit(0, sampleCapturePreRollBuffer.getNumSamples(), preRollSamples);
}

int NateVSTAudioProcessor::appendSamplesToSampleCaptureBuffer(const juce::AudioBuffer<float>& source,
                                                              int sourceChannelLimit,
                                                              int sourceOffset,
                                                              int requestedSamples) noexcept
{
    auto& captureBuffer = getActiveSampleCaptureBuffer();
    const auto maxSamples = captureBuffer.getNumSamples();
    const auto captureChannels = captureBuffer.getNumChannels();
    const auto sourceChannels = sourceChannelLimit >= 0
        ? juce::jmin(source.getNumChannels(), sourceChannelLimit)
        : source.getNumChannels();
    const auto availableSourceSamples = source.getNumSamples() - sourceOffset;
    const auto samplesRequested = juce::jmin(requestedSamples, availableSourceSamples);
    if (maxSamples <= 0 || captureChannels <= 0 || sourceChannels <= 0 || sourceOffset < 0 || samplesRequested <= 0)
        return 0;

    const auto targetSamples = sampleCaptureTargetSamples.load(std::memory_order_acquire);
    const auto alreadyRecorded = sampleCaptureSamplesRecorded.load(std::memory_order_relaxed);
    const auto targetRemaining = targetSamples > 0 ? targetSamples - alreadyRecorded : samplesRequested;
    if (targetSamples > 0 && targetRemaining <= 0)
    {
        sampleCaptureEnabled.store(false, std::memory_order_release);
        sampleCaptureWaitingForThreshold.store(false, std::memory_order_release);
        return 0;
    }

    const auto samplesToWrite = targetSamples > 0 ? juce::jmin(samplesRequested, targetRemaining)
                                                  : samplesRequested;
    auto writePosition = juce::jlimit(0,
                                      juce::jmax(0, maxSamples - 1),
                                      sampleCaptureWritePosition.load(std::memory_order_relaxed));
    auto readOffset = sourceOffset;
    auto remaining = samplesToWrite;
    while (remaining > 0)
    {
        const auto span = juce::jmin(remaining, maxSamples - writePosition);
        for (auto channel = 0; channel < captureChannels; ++channel)
        {
            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            captureBuffer.copyFrom(channel, writePosition, source, sourceChannel, readOffset, span);
        }

        readOffset += span;
        remaining -= span;
        writePosition = (writePosition + span) % maxSamples;
    }

    sampleCaptureWritePosition.store(writePosition, std::memory_order_release);
    const auto recorded = sampleCaptureSamplesRecorded.load(std::memory_order_relaxed);
    const auto newRecorded = juce::jmin(maxSamples, recorded + samplesToWrite);
    sampleCaptureSamplesRecorded.store(newRecorded, std::memory_order_release);
    if (targetSamples > 0 && newRecorded >= targetSamples)
    {
        sampleCaptureEnabled.store(false, std::memory_order_release);
        sampleCaptureWaitingForThreshold.store(false, std::memory_order_release);
    }

    return samplesToWrite;
}

void NateVSTAudioProcessor::appendToSampleCapturePreRoll(const juce::AudioBuffer<float>& buffer,
                                                         int sourceChannelLimit) noexcept
{
    const auto maxPreRollSamples = calculateSampleCapturePreRollSamples();
    const auto preRollChannels = sampleCapturePreRollBuffer.getNumChannels();
    const auto sourceChannels = sourceChannelLimit >= 0
        ? juce::jmin(buffer.getNumChannels(), sourceChannelLimit)
        : buffer.getNumChannels();
    const auto blockSamples = buffer.getNumSamples();
    if (maxPreRollSamples <= 0 || preRollChannels <= 0 || sourceChannels <= 0 || blockSamples <= 0)
        return;

    const auto samplesToStore = juce::jmin(blockSamples, maxPreRollSamples);
    auto sourceOffset = blockSamples - samplesToStore;
    auto writePosition = juce::jlimit(0,
                                      juce::jmax(0, maxPreRollSamples - 1),
                                      sampleCapturePreRollWritePosition.load(std::memory_order_relaxed));
    auto remaining = samplesToStore;
    while (remaining > 0)
    {
        const auto span = juce::jmin(remaining, maxPreRollSamples - writePosition);
        for (auto channel = 0; channel < preRollChannels; ++channel)
        {
            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            sampleCapturePreRollBuffer.copyFrom(channel, writePosition, buffer, sourceChannel, sourceOffset, span);
        }

        sourceOffset += span;
        remaining -= span;
        writePosition = (writePosition + span) % maxPreRollSamples;
    }

    sampleCapturePreRollWritePosition.store(writePosition, std::memory_order_release);
    const auto ready = sampleCapturePreRollSamplesReady.load(std::memory_order_relaxed);
    sampleCapturePreRollSamplesReady.store(juce::jmin(maxPreRollSamples, ready + samplesToStore),
                                           std::memory_order_release);
}

void NateVSTAudioProcessor::flushSampleCapturePreRoll() noexcept
{
    const auto requestedPreRollSamples = calculateSampleCapturePreRollSamples();
    const auto availablePreRollSamples = juce::jmin(requestedPreRollSamples,
                                                    sampleCapturePreRollSamplesReady.load(std::memory_order_acquire));
    const auto preRollBufferSamples = sampleCapturePreRollBuffer.getNumSamples();
    const auto preRollChannels = sampleCapturePreRollBuffer.getNumChannels();
    if (availablePreRollSamples <= 0 || preRollBufferSamples <= 0 || preRollChannels <= 0)
        return;

    auto sourceStart = juce::jlimit(0,
                                    juce::jmax(0, requestedPreRollSamples - 1),
                                    sampleCapturePreRollWritePosition.load(std::memory_order_acquire))
        - availablePreRollSamples;
    while (sourceStart < 0)
        sourceStart += requestedPreRollSamples;

    const auto firstSpan = juce::jmin(availablePreRollSamples, requestedPreRollSamples - sourceStart);
    const auto secondSpan = availablePreRollSamples - firstSpan;
    appendSamplesToSampleCaptureBuffer(sampleCapturePreRollBuffer, preRollChannels, sourceStart, firstSpan);
    if (sampleCaptureEnabled.load(std::memory_order_acquire) && secondSpan > 0)
        appendSamplesToSampleCaptureBuffer(sampleCapturePreRollBuffer, preRollChannels, 0, secondSpan);

    sampleCapturePreRollWritePosition.store(0, std::memory_order_release);
    sampleCapturePreRollSamplesReady.store(0, std::memory_order_release);
}

void NateVSTAudioProcessor::appendToSampleCapture(const juce::AudioBuffer<float>& buffer, int sourceChannelLimit) noexcept
{
    const auto captureSession = sampleCaptureSessionSerial.load(std::memory_order_acquire);
    if (! sampleCaptureEnabled.load(std::memory_order_acquire))
        return;

    struct ScopedActiveCaptureWriter final
    {
        explicit ScopedActiveCaptureWriter(std::atomic<int>& activeWritersToUse) noexcept
            : activeWriters(activeWritersToUse)
        {
            activeWriters.fetch_add(1, std::memory_order_acq_rel);
        }

        ~ScopedActiveCaptureWriter() noexcept
        {
            activeWriters.fetch_sub(1, std::memory_order_release);
        }

        std::atomic<int>& activeWriters;
    };

    const ScopedActiveCaptureWriter activeWriter(sampleCaptureActiveWriters);
    if (! sampleCaptureEnabled.load(std::memory_order_acquire)
        || sampleCaptureSessionSerial.load(std::memory_order_acquire) != captureSession)
    {
        return;
    }

    const auto sourceChannels = sourceChannelLimit >= 0
        ? juce::jmin(buffer.getNumChannels(), sourceChannelLimit)
        : buffer.getNumChannels();
    const auto blockSamples = buffer.getNumSamples();
    if (sourceChannels <= 0 || blockSamples <= 0)
        return;

    if (sampleCaptureWaitingForThreshold.load(std::memory_order_acquire))
    {
        if (calculateBufferPeak(buffer, sourceChannelLimit) < getSampleCaptureThresholdGain())
        {
            appendToSampleCapturePreRoll(buffer, sourceChannelLimit);
            return;
        }

        flushSampleCapturePreRoll();
        sampleCaptureWaitingForThreshold.store(false, std::memory_order_release);
    }

    if (! sampleCaptureEnabled.load(std::memory_order_acquire))
        return;

    appendSamplesToSampleCaptureBuffer(buffer, sourceChannelLimit, 0, blockSamples);
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

    updateOutputSpectrumSnapshot(buffer);

    if (numChannels <= 0 || numSamples <= 0)
    {
        outputMeterPeakLeft.store(0.0f, std::memory_order_relaxed);
        outputMeterPeakRight.store(0.0f, std::memory_order_relaxed);
        outputMeterRmsLeft.store(0.0f, std::memory_order_relaxed);
        outputMeterRmsRight.store(0.0f, std::memory_order_relaxed);
        stereoFieldCorrelation.store(0.0f, std::memory_order_relaxed);
        stereoFieldWidth.store(0.0f, std::memory_order_relaxed);
        stereoFieldBalance.store(0.0f, std::memory_order_relaxed);
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
    auto crossSum = 0.0;
    auto midSquares = 0.0;
    auto sideSquares = 0.0;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto left = leftSamples[sampleIndex];
        const auto right = rightSamples[sampleIndex];
        lowEndStateLeft += lowAlpha * (left - lowEndStateLeft);
        lowEndStateRight += lowAlpha * (right - lowEndStateRight);

        const auto monoLow = (lowEndStateLeft + lowEndStateRight) * 0.5f;
        const auto sideLow = (lowEndStateLeft - lowEndStateRight) * 0.5f;
        monoLowSquares += static_cast<double>(monoLow) * static_cast<double>(monoLow);
        sideLowSquares += static_cast<double>(sideLow) * static_cast<double>(sideLow);

        const auto mid = (left + right) * 0.5f;
        const auto side = (left - right) * 0.5f;
        crossSum += static_cast<double>(left) * static_cast<double>(right);
        midSquares += static_cast<double>(mid) * static_cast<double>(mid);
        sideSquares += static_cast<double>(side) * static_cast<double>(side);
    }

    const auto leftSquares = static_cast<double>(rmsLeft) * static_cast<double>(rmsLeft) * static_cast<double>(numSamples);
    const auto rightSquares = static_cast<double>(rmsRight) * static_cast<double>(rmsRight) * static_cast<double>(numSamples);
    const auto correlationDenominator = std::sqrt(juce::jmax(0.0, leftSquares * rightSquares));
    const auto correlation = correlationDenominator > 0.0000001
        ? static_cast<float>(crossSum / correlationDenominator)
        : 0.0f;
    const auto midRms = static_cast<float>(std::sqrt(midSquares / static_cast<double>(numSamples)));
    const auto sideRmsFullBand = static_cast<float>(std::sqrt(sideSquares / static_cast<double>(numSamples)));
    const auto stereoWidth = sideRmsFullBand / juce::jmax(0.00001f, midRms + sideRmsFullBand);
    const auto balance = (rmsRight - rmsLeft) / juce::jmax(0.00001f, rmsRight + rmsLeft);
    const auto subRms = static_cast<float>(std::sqrt(monoLowSquares / static_cast<double>(numSamples)));
    const auto sideRms = static_cast<float>(std::sqrt(sideLowSquares / static_cast<double>(numSamples)));
    const auto stereoRisk = sideRms / juce::jmax(0.00001f, subRms + sideRms);
    stereoFieldCorrelation.store(juce::jlimit(-1.0f, 1.0f, correlation), std::memory_order_relaxed);
    stereoFieldWidth.store(juce::jlimit(0.0f, 1.0f, stereoWidth), std::memory_order_relaxed);
    stereoFieldBalance.store(juce::jlimit(-1.0f, 1.0f, balance), std::memory_order_relaxed);
    lowEndSubRms.store(juce::jlimit(0.0f, 2.0f, subRms), std::memory_order_relaxed);
    lowEndStereoRisk.store(juce::jlimit(0.0f, 1.0f, stereoRisk), std::memory_order_relaxed);
    lowEndOutputPeak.store(juce::jlimit(0.0f, 2.0f, juce::jmax(peakLeft, peakRight)), std::memory_order_relaxed);
}

void NateVSTAudioProcessor::updateOutputSpectrumSnapshot(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    const auto* leftSamples = buffer.getReadPointer(0);
    const auto* rightSamples = buffer.getReadPointer(numChannels > 1 ? 1 : 0);

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto mono = (leftSamples[sampleIndex] + rightSamples[sampleIndex]) * 0.5f;
        outputSpectrumSamples[outputSpectrumWriteCursor].store(juce::jlimit(-2.0f, 2.0f, mono),
                                                               std::memory_order_relaxed);
        outputSpectrumWriteCursor = (outputSpectrumWriteCursor + 1u) % static_cast<uint32_t>(outputSpectrumSamples.size());
    }

    outputSpectrumWriteIndex.store(outputSpectrumWriteCursor, std::memory_order_relaxed);
}

void NateVSTAudioProcessor::mixPresetPreviewPlayback(juce::AudioBuffer<float>& buffer) noexcept
{
    if (! presetPreviewPlaying.load(std::memory_order_acquire))
        return;

    const juce::SpinLock::ScopedTryLockType lock(presetPreviewLock);
    if (! lock.isLocked())
        return;

    if (! presetPreviewPlaying.load(std::memory_order_relaxed)
        || presetPreviewPlaybackBuffer.getNumSamples() <= 0
        || presetPreviewPlaybackBuffer.getNumChannels() <= 0)
    {
        return;
    }

    const auto remaining = presetPreviewPlaybackBuffer.getNumSamples() - presetPreviewPlaybackPosition;
    if (remaining <= 0)
    {
        presetPreviewPlaying.store(false, std::memory_order_release);
        presetPreviewPlaybackPosition = 0;
        return;
    }

    const auto samplesToCopy = juce::jmin(buffer.getNumSamples(), remaining);
    const auto previewChannels = presetPreviewPlaybackBuffer.getNumChannels();
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto sourceChannel = juce::jmin(channel, previewChannels - 1);
        buffer.addFrom(channel,
                       0,
                       presetPreviewPlaybackBuffer,
                       sourceChannel,
                       presetPreviewPlaybackPosition,
                       samplesToCopy);
    }

    presetPreviewPlaybackPosition += samplesToCopy;
    if (presetPreviewPlaybackPosition >= presetPreviewPlaybackBuffer.getNumSamples())
    {
        presetPreviewPlaying.store(false, std::memory_order_release);
        presetPreviewPlaybackPosition = 0;
    }
}

NateVSTAudioProcessor::PresetPreviewRenderResult NateVSTAudioProcessor::renderPresetPreviewBuffer(const juce::String& presetName) const
{
    PresetPreviewRenderResult result;
    const auto trimmedName = presetName.trim();
    if (trimmedName.isEmpty())
    {
        result.info.status = "Preset name missing";
        return result;
    }

    auto presetFile = presetFileForName(trimmedName);
    if (! presetFile.existsAsFile())
        presetFile = factoryPresetFileForName(trimmedName);

    if (! presetFile.existsAsFile())
    {
        result.info.status = "Preset file missing";
        return result;
    }

    auto xml = juce::XmlDocument::parse(presetFile);
    if (xml == nullptr)
    {
        result.info.status = "Preset XML unreadable";
        return result;
    }

    auto state = juce::ValueTree::fromXml(*xml);
    if (! state.isValid() || ! state.hasType(parameters.state.getType()))
    {
        result.info.status = "Preset state invalid";
        return result;
    }

    const auto library = getPresetLibrary();
    const PresetInfo* presetInfo = nullptr;
    for (const auto& preset : library)
    {
        if (preset.name == trimmedName)
        {
            presetInfo = &preset;
            break;
        }
    }

    const auto descriptor = presetPreviewDescriptor(presetInfo, trimmedName);
    const auto role = presetPreviewRoleForDescriptor(descriptor);
    const auto rootNote = juce::roundToInt(readPresetParameterValue(state, Parameters::ID::sequencerRoot, 60.0f)
                                           + (readPresetParameterValue(state, Parameters::ID::sequencerOctave, 0.0f) * 12.0f));

    std::vector<PresetPreviewMidiEvent> midiEvents;
    buildPresetPreviewMidi(midiEvents, role, foldPreviewNoteToRange(rootNote, 36, 72));

    const auto totalSamples = static_cast<int>(std::round(presetPreviewSampleRate * presetPreviewDurationSeconds));
    result.buffer.setSize(presetPreviewChannelCount, totalSamples);
    result.buffer.clear();

    NateVSTAudioProcessor previewProcessor;
    previewProcessor.restorePluginState(state);
    previewProcessor.prepareToPlay(presetPreviewSampleRate, presetPreviewBlockSize);

    juce::AudioBuffer<float> blockBuffer(presetPreviewChannelCount, presetPreviewBlockSize);
    auto eventIndex = size_t { 0 };
    for (auto renderedSamples = 0; renderedSamples < totalSamples; renderedSamples += presetPreviewBlockSize)
    {
        const auto currentBlockSize = juce::jmin(presetPreviewBlockSize, totalSamples - renderedSamples);
        blockBuffer.setSize(presetPreviewChannelCount, currentBlockSize, false, false, true);
        blockBuffer.clear();

        juce::MidiBuffer midi;
        while (eventIndex < midiEvents.size() && midiEvents[eventIndex].sample < renderedSamples + currentBlockSize)
        {
            if (midiEvents[eventIndex].sample >= renderedSamples)
                midi.addEvent(midiEvents[eventIndex].message, midiEvents[eventIndex].sample - renderedSamples);

            ++eventIndex;
        }

        previewProcessor.processBlock(blockBuffer, midi);
        for (auto channel = 0; channel < presetPreviewChannelCount; ++channel)
            result.buffer.copyFrom(channel, renderedSamples, blockBuffer, channel, 0, currentBlockSize);
    }

    result.info = measurePresetPreviewBuffer(result.buffer, presetPreviewSampleRate);
    result.info.file = presetPreviewFileForName(trimmedName);
    result.info.status = "Rendered " + presetPreviewRoleName(role) + " preview | "
        + previewLevelText(result.info.peak, result.info.rms);
    result.rendered = result.info.peak > 0.001f && result.info.rms > 0.00005f;
    result.info.available = result.rendered;
    result.info.stale = false;
    return result;
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
    state.setProperty(sequencerSceneChainPlaybackProperty,
                      isSequencerSceneChainPlaybackEnabled(),
                      nullptr);
    state.setProperty(sequencerSceneChainClipBarsProperty,
                      getSequencerSceneChainClipBars(),
                      nullptr);

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
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_ratchet", step.ratchet, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_condition", step.condition, nullptr);
        state.setProperty("seq_step_" + juce::String(stepIndex) + "_slide", step.slide, nullptr);
    }

    if (includePerformanceSnapshots)
        appendPerformanceSnapshotsToState(state);
    appendSequencerScenesToState(state);

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
    removeSequencerSceneChildren(stateForParameters);
    const auto hasSequencerChordMode = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordMode).isValid();
    const auto hasSequencerChordVoicing = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordVoicing).isValid();
    const auto hasSequencerChordStrum = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordStrum).isValid();
    const auto hasSequencerChordMemory = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerChordMemory).isValid();
    const auto hasSampleSliceStyle = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceStyle).isValid();
    const auto hasSampleEngineControls = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleEngineMode).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleGrainSize).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleGrainSpray).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSpectralFreeze).isValid();
    const auto hasSampleSliceEdits = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceCustom[0]).isValid();
    const auto hasSampleSliceRegions = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceStart[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceEnd[0]).isValid();
    const auto hasSampleSlicePanProbability = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSlicePan[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceProbability[0]).isValid();
    const auto hasSampleSliceChoke = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceChoke[0]).isValid();
    const auto hasSampleSliceNudgeFade = stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceNudge[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sampleSliceFade[0]).isValid();
    const auto hasFilterCharacter = stateForParameters.getChildWithProperty("id", Parameters::ID::filterCharacter).isValid();
    const auto hasFilterSlope = stateForParameters.getChildWithProperty("id", Parameters::ID::filterSlope).isValid();
    const auto hasOscWarp = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWarp).isValid();
    const auto hasOscWarpB = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWarpB).isValid();
    const auto hasOsc2Warp = stateForParameters.getChildWithProperty("id", Parameters::ID::osc2Warp).isValid();
    const auto hasOsc2WarpB = stateForParameters.getChildWithProperty("id", Parameters::ID::osc2WarpB).isValid();
    const auto hasOscWarpMode = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWarpMode).isValid();
    const auto hasOscWarpBMode = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWarpBMode).isValid();
    const auto hasOsc2WarpMode = stateForParameters.getChildWithProperty("id", Parameters::ID::osc2WarpMode).isValid();
    const auto hasOsc2WarpBMode = stateForParameters.getChildWithProperty("id", Parameters::ID::osc2WarpBMode).isValid();
    const auto hasNoiseSourceControls = stateForParameters.getChildWithProperty("id", Parameters::ID::noiseType).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::noiseDecay).isValid();
    const auto hasFxSendControls = stateForParameters.getChildWithProperty("id", Parameters::ID::fxSendDelay).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::fxSendReverb).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::fxSendTailKill).isValid();
    const auto hasFxDriveBassSafe = stateForParameters.getChildWithProperty("id", Parameters::ID::fxDistortionBassSafe).isValid();
    const auto hasFxGuardDynamics = stateForParameters.getChildWithProperty("id", Parameters::ID::fxGuardGlue).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::fxGuardPunch).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::fxGuardClipMix).isValid();
    const auto hasWavetablePositions = stateForParameters.getChildWithProperty("id", Parameters::ID::oscWavetablePosition).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::osc2WavetablePosition).isValid();
    const auto hasOsc1CustomWave = stateForParameters.getChildWithProperty("id", Parameters::ID::oscCustomWave.back()).isValid();
    const auto hasOsc2CustomWave = stateForParameters.getChildWithProperty("id", Parameters::ID::osc2CustomWave.back()).isValid();
    const auto hasSequencerLockControls = stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerLockDestination).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::sequencerLockDepth).isValid();
    const auto hasModMatrixEnabled = stateForParameters.getChildWithProperty("id", Parameters::ID::modMatrixEnabled[0]).isValid();
    const auto hasStepLfoControls = stateForParameters.getChildWithProperty("id", Parameters::ID::stepLfoSync).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::stepLfoValue.back()).isValid();
    const auto hasModMatrixRouteShape = stateForParameters.getChildWithProperty("id", Parameters::ID::modMatrixPolarity[0]).isValid()
        && stateForParameters.getChildWithProperty("id", Parameters::ID::modMatrixSlew[0]).isValid();

    if (shouldRestorePerformanceSnapshots)
        restorePerformanceSnapshotsFromState(state);
    sequencerSceneChainPlaybackEnabled.store(static_cast<bool>(stateForParameters.getProperty(sequencerSceneChainPlaybackProperty, false)) ? 1 : 0,
                                             std::memory_order_release);
    sequencerSceneChainClipBars.store(sanitiseSequencerSceneChainClipBars(static_cast<int>(stateForParameters.getProperty(sequencerSceneChainClipBarsProperty, 0))),
                                      std::memory_order_release);
    restoreSequencerScenesFromState(state);

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
    if (! hasSampleEngineControls)
    {
        setParameterPlainValue(Parameters::ID::sampleEngineMode, 0.0f);
        setParameterPlainValue(Parameters::ID::sampleGrainSize, 0.08f);
        setParameterPlainValue(Parameters::ID::sampleGrainSpray, 0.0f);
        setParameterPlainValue(Parameters::ID::sampleSpectralFreeze, 0.0f);
    }
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
            setParameterPlainValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceFade[index], 0.0f);
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
    if (! hasSampleSliceNudgeFade)
    {
        for (size_t index = 0; index < Parameters::ID::sampleSliceNudge.size(); ++index)
        {
            setParameterPlainValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
            setParameterPlainValue(Parameters::ID::sampleSliceFade[index], 0.0f);
        }
    }

    if (! hasFilterCharacter)
        setParameterPlainValue(Parameters::ID::filterCharacter, 0.0f);
    if (! hasFilterSlope)
        setParameterPlainValue(Parameters::ID::filterSlope, 0.0f);
    if (! hasOscWarp)
        setParameterPlainValue(Parameters::ID::oscWarp, 0.0f);
    if (! hasOscWarpB)
        setParameterPlainValue(Parameters::ID::oscWarpB, 0.0f);
    if (! hasOsc2Warp)
        setParameterPlainValue(Parameters::ID::osc2Warp, 0.0f);
    if (! hasOsc2WarpB)
        setParameterPlainValue(Parameters::ID::osc2WarpB, 0.0f);
    if (! hasOscWarpMode)
        setParameterPlainValue(Parameters::ID::oscWarpMode, 0.0f);
    if (! hasOscWarpBMode)
        setParameterPlainValue(Parameters::ID::oscWarpBMode, 0.0f);
    if (! hasOsc2WarpMode)
    {
        auto sharedWarpMode = 0.0f;
        if (auto* parameter = parameters.getRawParameterValue(Parameters::ID::oscWarpMode))
            sharedWarpMode = parameter->load();

        setParameterPlainValue(Parameters::ID::osc2WarpMode, sharedWarpMode);
    }
    if (! hasOsc2WarpBMode)
        setParameterPlainValue(Parameters::ID::osc2WarpBMode, 0.0f);
    if (! hasNoiseSourceControls)
    {
        setParameterPlainValue(Parameters::ID::noiseType, 0.0f);
        setParameterPlainValue(Parameters::ID::noiseDecay, 0.18f);
    }
    if (! hasFxSendControls)
    {
        setParameterPlainValue(Parameters::ID::fxSendDelay, 0.0f);
        setParameterPlainValue(Parameters::ID::fxSendReverb, 0.0f);
        setParameterPlainValue(Parameters::ID::fxSendTailKill, 0.0f);
    }
    if (! hasFxDriveBassSafe)
        setParameterPlainValue(Parameters::ID::fxDistortionBassSafe, 0.0f);
    if (! hasFxGuardDynamics)
    {
        setParameterPlainValue(Parameters::ID::fxGuardGlue, 0.0f);
        setParameterPlainValue(Parameters::ID::fxGuardPunch, 0.0f);
        setParameterPlainValue(Parameters::ID::fxGuardClipMix, 1.0f);
    }
    if (! hasWavetablePositions)
    {
        setParameterPlainValue(Parameters::ID::oscWavetablePosition, 0.0f);
        setParameterPlainValue(Parameters::ID::osc2WavetablePosition, 0.35f);
    }
    if (! hasOsc1CustomWave || ! hasOsc2CustomWave)
    {
        constexpr std::array<float, 16> customWaveDefaults {
            0.5f,
            0.691342f,
            0.853553f,
            0.961940f,
            1.0f,
            0.961940f,
            0.853553f,
            0.691342f,
            0.5f,
            0.308658f,
            0.146447f,
            0.038060f,
            0.0f,
            0.038060f,
            0.146447f,
            0.308658f
        };

        for (size_t index = 0; index < customWaveDefaults.size(); ++index)
        {
            if (! hasOsc1CustomWave)
                setParameterPlainValue(Parameters::ID::oscCustomWave[index], customWaveDefaults[index]);

            if (! hasOsc2CustomWave)
                setParameterPlainValue(Parameters::ID::osc2CustomWave[index], customWaveDefaults[index]);
        }
    }
    if (! hasSequencerLockControls)
    {
        setParameterPlainValue(Parameters::ID::sequencerLockDestination, 0.0f);
        setParameterPlainValue(Parameters::ID::sequencerLockDepth, 0.35f);
    }
    if (! hasModMatrixEnabled)
        for (const auto* parameterID : Parameters::ID::modMatrixEnabled)
            setParameterPlainValue(parameterID, 1.0f);

    if (! hasStepLfoControls)
    {
        constexpr std::array<float, 8> defaultStepLfoValues {
            1.0f, -0.15f, 0.62f, -0.45f, 0.84f, 0.18f, -0.72f, 0.36f
        };

        setParameterPlainValue(Parameters::ID::stepLfoSync, 1.0f);
        setParameterPlainValue(Parameters::ID::stepLfoSyncRate, 3.0f);
        setParameterPlainValue(Parameters::ID::stepLfoRate, 2.0f);
        setParameterPlainValue(Parameters::ID::stepLfoDepth, 0.55f);
        setParameterPlainValue(Parameters::ID::stepLfoSlew, 0.0f);
        for (size_t index = 0; index < Parameters::ID::stepLfoValue.size(); ++index)
            setParameterPlainValue(Parameters::ID::stepLfoValue[index], defaultStepLfoValues[index]);
    }

    if (! hasModMatrixRouteShape)
    {
        for (size_t index = 0; index < Parameters::ID::modMatrixPolarity.size(); ++index)
        {
            setParameterPlainValue(Parameters::ID::modMatrixPolarity[index], 0.0f);
            setParameterPlainValue(Parameters::ID::modMatrixCurve[index], 0.0f);
            setParameterPlainValue(Parameters::ID::modMatrixRangeMin[index], -1.0f);
            setParameterPlainValue(Parameters::ID::modMatrixRangeMax[index], 1.0f);
            setParameterPlainValue(Parameters::ID::modMatrixSlew[index], 0.0f);
        }
    }

    restoreSampleFileReference(stateForParameters.getProperty("sample_file").toString());

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
        step.ratchet = static_cast<int>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_ratchet", 1));
        step.condition = static_cast<int>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_condition", 0));
        step.slide = static_cast<bool>(stateForParameters.getProperty("seq_step_" + juce::String(stepIndex) + "_slide", false));
        patternSequencer.setStep(stepIndex, step);
    }

    refreshSequencerSceneChainPlayback();
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

juce::File NateVSTAudioProcessor::presetPreviewFileForName(const juce::String& presetName) const
{
    auto legalName = juce::File::createLegalFileName(presetName.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    return getPresetPreviewDirectory().getChildFile(legalName).withFileExtension(".wav");
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
