#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
struct PresetRenderStats
{
    float peak = 0.0f;
    float rms = 0.0f;
    bool finite = true;
};

bool copyFactoryPresetFiles(std::vector<juce::String>& repoPresetNames)
{
    NateVSTAudioProcessor processor;
    const juce::File sourceDirectory { NATE_VST_FACTORY_PRESETS_TEST_SOURCE };
    const auto factoryDirectory = processor.getFactoryPresetDirectory();
    if (! sourceDirectory.isDirectory() || ! factoryDirectory.createDirectory())
    {
        std::cerr << "Factory preset source or destination directory was unavailable\n";
        return false;
    }

    for (const auto& file : sourceDirectory.findChildFiles(juce::File::findFiles, true, "*.natevstpreset"))
    {
        repoPresetNames.push_back(file.getFileNameWithoutExtension());

        const auto destination = factoryDirectory.getChildFile(file.getFileName());
        if (destination.existsAsFile() && ! destination.deleteFile())
        {
            std::cerr << "Could not replace stale factory preset for render audit: "
                      << destination.getFullPathName() << '\n';
            return false;
        }

        if (! file.copyFileTo(destination))
        {
            std::cerr << "Could not copy factory preset for render audit: " << file.getFullPathName() << '\n';
            return false;
        }
    }

    return true;
}

const NateVSTAudioProcessor::PresetInfo* findPreset(const std::vector<NateVSTAudioProcessor::PresetInfo>& presets,
                                                    const juce::String& name)
{
    for (const auto& preset : presets)
        if (preset.name == name)
            return &preset;

    return nullptr;
}

int validationNoteForPreset(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto descriptor = (preset.name + " " + preset.category + " " + preset.pack + " "
                             + preset.tags + " " + preset.notes)
                                .toLowerCase();

    if (descriptor.contains("bass")
        || descriptor.contains("reese")
        || descriptor.contains("rumble")
        || descriptor.contains("sub")
        || descriptor.contains("acid")
        || descriptor.contains("wobble")
        || descriptor.contains("log drum"))
        return 48;

    if (descriptor.contains("pad") || descriptor.contains("wash") || descriptor.contains("atmos"))
        return 55;

    return 60;
}

bool isTransientRole(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto descriptor = (preset.name + " " + preset.category + " " + preset.pack + " "
                             + preset.tags + " " + preset.notes)
                                .toLowerCase();

    return descriptor.contains("tick")
        || descriptor.contains("noise")
        || descriptor.contains("hit")
        || descriptor.contains("perc")
        || descriptor.contains("pluck")
        || descriptor.contains("riser")
        || descriptor.contains("sweep");
}

PresetRenderStats renderPreset(const juce::String& presetName, int midiNote)
{
    static constexpr auto sampleRate = 44100.0;
    static constexpr auto blockSize = 512;
    static constexpr auto totalBlocks = 160;
    static constexpr auto noteOffBlock = 112;
    static constexpr auto channelCount = 2;

    NateVSTAudioProcessor processor;
    if (! processor.loadPreset(presetName))
    {
        std::cerr << "Factory preset could not be loaded by name for render audit: " << presetName << '\n';
        return { 0.0f, 0.0f, false };
    }

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(channelCount, blockSize);
    PresetRenderStats stats;
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto blockIndex = 0; blockIndex < totalBlocks; ++blockIndex)
    {
        buffer.clear();
        juce::MidiBuffer midi;

        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(108)), 0);
        if (blockIndex == noteOffBlock)
            midi.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0);

        processor.processBlock(buffer, midi);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
            {
                const auto value = samples[sampleIndex];
                if (! std::isfinite(value))
                    stats.finite = false;

                const auto absolute = std::abs(value);
                stats.peak = juce::jmax(stats.peak, absolute);
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;
            }
        }
    }

    stats.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    return stats;
}
}

int main()
{
    static constexpr auto minimumUsefulPeak = 0.01f;
    static constexpr auto minimumUsefulRms = 0.0008f;
    static constexpr auto minimumTransientRms = 0.00025f;
    static constexpr auto maximumSmokePeak = 1.25f;

    std::vector<juce::String> repoPresetNames;
    if (! copyFactoryPresetFiles(repoPresetNames))
        return 1;

    if (repoPresetNames.size() < 400)
    {
        std::cerr << "Expected at least 400 repo-managed factory presets, found " << repoPresetNames.size() << '\n';
        return 1;
    }

    NateVSTAudioProcessor libraryProcessor;
    const auto library = libraryProcessor.getPresetLibrary();
    auto quietestName = juce::String();
    auto quietestRms = std::numeric_limits<float>::max();
    auto hottestName = juce::String();
    auto hottestPeak = 0.0f;
    std::vector<juce::String> failures;

    for (const auto& presetName : repoPresetNames)
    {
        const auto* preset = findPreset(library, presetName);
        if (preset == nullptr || ! preset->isFactory)
        {
            failures.push_back("Repo-managed factory preset was not found in the scanned factory library: " + presetName);
            continue;
        }

        const auto stats = renderPreset(presetName, validationNoteForPreset(*preset));
        const auto requiredRms = isTransientRole(*preset) ? minimumTransientRms : minimumUsefulRms;
        if (stats.rms < quietestRms)
        {
            quietestRms = stats.rms;
            quietestName = presetName;
        }

        if (stats.peak > hottestPeak)
        {
            hottestPeak = stats.peak;
            hottestName = presetName;
        }

        if (! stats.finite)
        {
            failures.push_back("Factory preset rendered non-finite output: " + presetName);
            continue;
        }

        if (stats.peak < minimumUsefulPeak || stats.rms < requiredRms)
        {
            failures.push_back("Factory preset rendered too quietly: " + presetName
                               + " peak " + juce::String(stats.peak, 6)
                               + " rms " + juce::String(stats.rms, 6));
            continue;
        }

        if (stats.peak > maximumSmokePeak)
        {
            failures.push_back("Factory preset exceeded the smoke peak ceiling: " + presetName
                               + " peak " + juce::String(stats.peak, 6)
                               + " rms " + juce::String(stats.rms, 6));
        }
    }

    if (! failures.empty())
    {
        for (const auto& failure : failures)
            std::cerr << failure << '\n';
        return 1;
    }

    std::cout << "Factory preset render audit passed for " << repoPresetNames.size()
              << " presets. Quietest RMS: " << quietestName << " " << quietestRms
              << ", hottest peak: " << hottestName << " " << hottestPeak << ".\n";
    return 0;
}
