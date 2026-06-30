#include "../Source/PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool copyFactoryPresetFiles()
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
        const auto destination = factoryDirectory.getChildFile(file.getFileName());
        if (destination.existsAsFile() && ! destination.deleteFile())
        {
            std::cerr << "Could not replace stale factory preset for preview audit: "
                      << destination.getFullPathName() << '\n';
            return false;
        }

        if (! file.copyFileTo(destination))
        {
            std::cerr << "Could not copy factory preset for preview audit: " << file.getFullPathName() << '\n';
            return false;
        }
    }

    return true;
}

bool recentNamesContain(const NateVSTAudioProcessor& processor, const juce::String& name)
{
    return processor.getRecentPresetNames().contains(name);
}

bool previewFileIsReadable(const juce::File& file)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    return reader != nullptr && reader->lengthInSamples > 0 && reader->numChannels > 0;
}
}

int main()
{
    if (! copyFactoryPresetFiles())
        return 1;

    NateVSTAudioProcessor processor;
    const std::vector<juce::String> previewPresets {
        "Deep House Ninth Chord Kit",
        "Jackin Organ Bass Kit",
        "Tech House Rubber Hook Kit",
        "UKG Vocal Chop Starter",
        "Warehouse Riser Noise"
    };

    for (const auto& presetName : previewPresets)
    {
        const auto previewFile = processor.getPresetPreviewDirectory()
            .getChildFile(juce::File::createLegalFileName(presetName))
            .withFileExtension(".wav");
        if (previewFile.existsAsFile() && ! previewFile.deleteFile())
        {
            std::cerr << "Could not clear stale preview before audit: " << previewFile.getFullPathName() << '\n';
            return 1;
        }
    }

    juce::StringArray batchNames;
    std::vector<bool> recentBefore;
    processor.notePresetLoaded("__sentinel_recent__");
    for (const auto& presetName : previewPresets)
    {
        batchNames.add(presetName);
        recentBefore.push_back(recentNamesContain(processor, presetName));
    }

    const auto batchResult = processor.ensurePresetPreviews(batchNames, false);
    if (batchResult.requested != static_cast<int>(previewPresets.size())
        || batchResult.rendered != static_cast<int>(previewPresets.size())
        || batchResult.failed != 0)
    {
        std::cerr << "Preview batch did not render all missing previews: "
                  << batchResult.status << '\n';
        return 1;
    }

    const auto readyResult = processor.ensurePresetPreviews(batchNames, false);
    if (readyResult.rendered != 0
        || readyResult.alreadyReady != static_cast<int>(previewPresets.size())
        || readyResult.failed != 0)
    {
        std::cerr << "Ready preview batch did not skip cached previews: "
                  << readyResult.status << '\n';
        return 1;
    }

    const auto regeneratedResult = processor.ensurePresetPreviews(batchNames, true);
    if (regeneratedResult.rendered != static_cast<int>(previewPresets.size())
        || regeneratedResult.failed != 0)
    {
        std::cerr << "Forced preview regeneration failed: "
                  << regeneratedResult.status << '\n';
        return 1;
    }

    for (size_t index = 0; index < previewPresets.size(); ++index)
    {
        const auto& presetName = previewPresets[index];
        const auto info = processor.getPresetPreviewInfo(presetName);
        if (! info.available || info.stale || info.file == juce::File() || ! info.file.existsAsFile())
        {
            std::cerr << "Preview was not cached for " << presetName << ": " << info.status << '\n';
            return 1;
        }

        if (! previewFileIsReadable(info.file))
        {
            std::cerr << "Preview file was not readable for " << presetName << '\n';
            return 1;
        }

        if (info.peak < 0.001f || info.rms < 0.00005f || info.peak > 1.5f || ! std::isfinite(info.peak) || ! std::isfinite(info.rms))
        {
            std::cerr << "Preview levels were invalid for " << presetName
                      << " peak " << info.peak
                      << " rms " << info.rms << '\n';
            return 1;
        }

        if (recentNamesContain(processor, presetName) != recentBefore[index])
        {
            std::cerr << "Preview render modified recent preset state for " << presetName << '\n';
            return 1;
        }

        if (! processor.startPresetPreviewPlayback(presetName) || ! processor.isPresetPreviewPlaying())
        {
            std::cerr << "Preview playback did not start for " << presetName << '\n';
            return 1;
        }

        processor.stopPresetPreviewPlayback();
    }

    const auto library = processor.getPresetLibrary();
    auto foundReadyPreview = false;
    for (const auto& preset : library)
    {
        if (previewPresets.front() == preset.name)
        {
            foundReadyPreview = preset.previewAvailable
                && ! preset.previewStale
                && preset.previewPeak > 0.001f
                && preset.previewRms > 0.00005f
                && preset.previewDurationSeconds > 0.1;
            break;
        }
    }

    if (! foundReadyPreview)
    {
        std::cerr << "Preset library did not report ready preview level metadata\n";
        return 1;
    }

    std::cout << "Preset preview audit passed for " << previewPresets.size() << " rendered previews.\n";
    return 0;
}
