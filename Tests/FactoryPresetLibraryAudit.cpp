#include "../Source/PluginProcessor.h"

#include <juce_core/juce_core.h>

#include <iostream>
#include <vector>

namespace
{
const NateVSTAudioProcessor::PresetInfo* findPreset(const std::vector<NateVSTAudioProcessor::PresetInfo>& presets,
                                                    const juce::String& name)
{
    for (const auto& preset : presets)
        if (preset.name == name)
            return &preset;

    return nullptr;
}
}

int main()
{
    NateVSTAudioProcessor processor;
    const juce::File sourceDirectory { NATE_VST_FACTORY_PRESETS_TEST_SOURCE };
    const auto factoryDirectory = processor.getFactoryPresetDirectory();
    if (! sourceDirectory.isDirectory() || ! factoryDirectory.createDirectory())
    {
        std::cerr << "Factory preset source or destination directory was unavailable\n";
        return 1;
    }

    for (const auto& file : sourceDirectory.findChildFiles(juce::File::findFiles, true, "*.natevstpreset"))
    {
        const auto destination = factoryDirectory.getChildFile(file.getFileName());
        if (! file.copyFileTo(destination))
        {
            std::cerr << "Could not copy factory preset for audit: " << file.getFullPathName() << '\n';
            return 1;
        }
    }

    const auto presets = processor.getPresetLibrary();

    auto factoryCount = 0;
    for (const auto& preset : presets)
        if (preset.isFactory)
            ++factoryCount;

    if (factoryCount < 24)
    {
        std::cerr << "Expected at least 24 factory presets, found " << factoryCount << '\n';
        return 1;
    }

    const std::vector<std::pair<juce::String, juce::String>> expectedPacks {
        { "Deep House Velvet Pad", "House Tools" },
        { "House Organ Skank", "House Tools" },
        { "Tech House Rolling Bass", "Tech House Tools" },
        { "Tech House Perc Stab", "Tech House Tools" },
        { "Minimal Dub Chord", "Minimal Tools" },
        { "Minimal Noise Tick", "Minimal Tools" },
        { "Warehouse Noise Sweep", "Techno Tools" },
        { "Techno Rumble Stab", "Techno Tools" },
    };

    for (const auto& [name, pack] : expectedPacks)
    {
        const auto* preset = findPreset(presets, name);
        if (preset == nullptr)
        {
            std::cerr << "Missing factory preset: " << name << '\n';
            return 1;
        }

        if (! preset->isFactory
            || preset->pack != pack
            || preset->notes.trim().isEmpty()
            || preset->macroValues.size() != 8
            || preset->tags.trim().isEmpty())
        {
            std::cerr << "Factory preset metadata was incomplete for " << name << '\n';
            return 1;
        }

        if (! processor.loadPreset(name))
        {
            std::cerr << "Factory preset could not be loaded by name: " << name << '\n';
            return 1;
        }
    }

    std::cout << "Factory preset library audit passed for expanded style pack.\n";
    return 0;
}
