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
        if (destination.existsAsFile() && ! destination.deleteFile())
        {
            std::cerr << "Could not replace stale factory preset for audit: " << destination.getFullPathName() << '\n';
            return 1;
        }

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

    if (factoryCount < 400)
    {
        std::cerr << "Expected at least 400 factory presets, found " << factoryCount << '\n';
        return 1;
    }

    const std::vector<std::pair<juce::String, juce::String>> expectedPacks {
        { "Deep House Seventh Stab", "House Chords" },
        { "Deep House Mellow Keys", "House Chords" },
        { "Deep House Velvet Pad", "House Tools" },
        { "House Organ Skank", "House Tools" },
        { "Tech House Acid Roller", "Tech House Tools" },
        { "Tech House Top Pluck", "Tech House Tools" },
        { "Tech House Rolling Bass", "Tech House Tools" },
        { "Tech House Perc Stab", "Tech House Tools" },
        { "Minimal Hypno Sequence", "Minimal Tools" },
        { "Minimal Glass Pluck", "Minimal Tools" },
        { "Minimal Dub Chord", "Minimal Tools" },
        { "Minimal Noise Tick", "Minimal Tools" },
        { "Techno Metallic Ping", "Techno Tools" },
        { "Warehouse Riser Noise", "Techno Tools" },
        { "Warehouse Noise Sweep", "Techno Tools" },
        { "Techno Rumble Stab", "Techno Tools" },
        { "Afro House Perc Bell", "House Tools" },
        { "Afro House Marimba Skank", "House Tools" },
        { "Progressive House Rise Pad", "House Tools" },
        { "Lo-Fi House Dust Keys", "House Chords" },
        { "Detroit House Chord Stab", "House Chords" },
        { "Melodic Techno Pluck Lead", "Techno Tools" },
        { "Melodic Techno Dark Pulse", "Techno Tools" },
        { "Acid Minimal Squiggle", "Minimal Tools" },
        { "Piano House Riff Keys", "House Chords" },
        { "Disco House Filter Stab", "House Tools" },
        { "Latin House Perc Organ", "House Tools" },
        { "Organic House Kalimba Pluck", "House Tools" },
        { "Dub Techno Chord Wash", "Dub Stabs" },
        { "Warehouse Techno Rumble Hit", "Techno Tools" },
        { "Breakbeat Garage Reese", "UKG Basslines" },
        { "Electro House Sync Lead", "House Tools" },
        { "Speed Garage Organ Bass", "UKG Basslines" },
        { "Bass House Wobble Hook", "House Tools" },
        { "Hardgroove Techno Perc Stab", "Techno Tools" },
        { "Amapiano Log Drum Pluck", "House Tools" },
        { "Future Garage Atmos Pad", "Garage Chops" },
        { "Jackin House Organ Bass", "House Tools" },
        { "Minimal Micro Perc Sequence", "Minimal Tools" },
        { "Peak Time Techno FM Lead", "Techno Tools" },
        { "Deep Tech Organ Roller", "Tech House Tools" },
        { "French House Filter Chord", "House Chords" },
        { "Soulful House M1 Keys", "House Chords" },
        { "Garage House Swing Lead", "House Tools" },
        { "Microhouse Dust Pluck", "Minimal Tools" },
        { "Raw Hypnotic Techno Stab", "Techno Tools" },
        { "Tribal Tech House Perc Bass", "Tech House Tools" },
        { "Breaks House Reese Pluck", "House Tools" },
        { "Chicago House Acid Bass", "House Tools" },
        { "Classic House Piano Chord", "House Chords" },
        { "Funky House Clav Stab", "House Tools" },
        { "Melodic House Arp Pluck", "House Tools" },
        { "Romanian Minimal Pluck", "Minimal Tools" },
        { "Berlin Dub Techno Stab", "Dub Stabs" },
        { "Hardgroove Rave Chord", "Techno Tools" },
        { "Electro Breaks Sync Bass", "House Tools" },
        { "Acid House 303 Line", "House Tools" },
        { "Deep Tech Chord Tool", "Tech House Tools" },
        { "Nu Disco Filter Lead", "House Tools" },
        { "Afro Melodic House Pluck", "House Tools" },
        { "Progressive House Pluck Stack", "House Tools" },
        { "Peak Time Techno Rumble Bass", "Techno Tools" },
        { "Detroit Techno Minor Chord", "Techno Tools" },
        { "Minimal FM Bubble", "Minimal Tools" },
        { "Indie Dance Rubber Bass", "House Tools" },
        { "Italo Disco Arp Lead", "House Tools" },
        { "Balearic Sunset Pad", "House Tools" },
        { "Afro Tech Perc Bass", "Tech House Tools" },
        { "Hard House Rave Stab", "House Tools" },
        { "Melodic Techno Reese Lead", "Techno Tools" },
        { "Deep Minimal Sub Groove", "Minimal Tools" },
        { "Lo-Fi House Vinyl Chord", "House Chords" },
        { "Deep House Ninth Chord Kit", "House Chords" },
        { "Piano House Lift Riff", "House Chords" },
        { "Jackin Organ Bass Kit", "House Tools" },
        { "Disco Filter Sweep Stab", "House Tools" },
        { "Tech House Rubber Hook Kit", "Tech House Tools" },
        { "Dub House Chord Throw", "Dub Stabs" },
        { "Dirty House Tool Stab", "House Tools" },
        { "Garage House Chord Kit", "Garage Chops" },
        { "UKG Swing Organ Bass", "UKG Basslines" },
        { "UKG Dark Reese Skip", "UKG Basslines" },
        { "Piano House Filter Pump", "House Chords" },
        { "Tech House Low Roller", "Tech House Tools" },
        { "Hypnotic Warehouse Chord", "Techno Tools" },
        { "Glassy Micro Chord", "Minimal Tools" },
        { "Afro House Sunrise Marimba Pluck", "Afro House Rituals" },
        { "Amapiano Log Drum Knock", "Amapiano Log Lab" },
        { "Melodic Techno Afterhours Pluck", "Melodic Techno Motion" },
        { "Organic House Cedar Pluck", "Organic Progressive" },
        { "DnB Liquid Reese", "Drum & Bass Tools" },
        { "Bass House Nasal Wobble", "Bass House Weapons" },
        { "Hard Techno Rave Siren Stab", "Hard Techno Rave" },
        { "Indie Dance Rubber Disco Bass", "Indie Dance Dark Disco" },
        { "Trance Supersaw Pluck", "Trance Progressive" },
        { "Ambient Dust Halo Pad", "Ambient Electronica" },
        { "Future Garage Rain Chord Stab", "Future Garage Dub" },
        { "Latin Tech Tribal Perc Bass", "Latin Tech Tribal" },
        { "House Velvet Organ Bass", "House Expansion" },
        { "Tech House Rubber Pressure Bass", "Tech House Expansion" },
        { "UKG 2 Step Sub Skip", "UKG Expansion" },
        { "Techno Warehouse Rumble Bass", "Techno Expansion" },
        { "Minimal Micro Click Pluck", "Minimal Expansion" },
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

    std::cout << "Factory preset library audit passed for 400-preset expanded style pack.\n";
    return 0;
}
