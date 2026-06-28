#include "../Source/PluginProcessor.h"

#include <juce_core/juce_core.h>

#include <iostream>

int main()
{
    NateVSTAudioProcessor processor;

    NateVSTAudioProcessor::PresetSaveOptions options;
    options.category = "UKG\\Bass";
    options.author = "Preset Save Audit";
    options.pack = "UKG Basslines";
    options.key = "C Min";
    options.bpm = 132;
    options.generated = true;
    options.generatedRecipe = "UKG Dred Bass";

    const auto presetName = "Preset Save Audit " + juce::String(static_cast<int>(juce::Time::getMillisecondCounter() % 1000000));
    auto legalName = juce::File::createLegalFileName(presetName);
    if (legalName.isEmpty())
        legalName = "Untitled";

    const auto expectedFile = processor.getPresetDirectory()
                                  .getChildFile("UKG")
                                  .getChildFile("Bass")
                                  .getChildFile(legalName)
                                  .withFileExtension(".natevstpreset");

    if (! processor.savePreset(presetName, options))
    {
        std::cerr << "savePreset returned false\n";
        return 1;
    }

    if (! expectedFile.existsAsFile())
    {
        std::cerr << "Expected categorized preset file was not created: "
                  << expectedFile.getFullPathName() << '\n';
        return 1;
    }

    auto cleanup = [&expectedFile]
    {
        expectedFile.deleteFile();
    };

    if (auto xml = juce::XmlDocument::parse(expectedFile))
    {
        const auto state = juce::ValueTree::fromXml(*xml);
        if (! state.isValid())
        {
            std::cerr << "Saved preset XML did not parse into a ValueTree\n";
            cleanup();
            return 1;
        }

        if (state.getProperty("preset_category").toString() != "UKG/Bass"
            || state.getProperty("preset_folder").toString() != "UKG/Bass"
            || state.getProperty("preset_pack").toString() != "UKG Basslines"
            || state.getProperty("preset_key").toString() != "C Min"
            || static_cast<int>(state.getProperty("preset_bpm")) != 132
            || state.getProperty("preset_source").toString() != "Generated"
            || state.getProperty("preset_generated_recipe").toString() != "UKG Dred Bass"
            || ! state.getProperty("preset_notes").toString().contains("Recipe: UKG Dred Bass")
            || ! state.getProperty("preset_notes").toString().contains("Category: UKG/Bass")
            || ! state.getProperty("preset_tags").toString().contains("Generated")
            || ! state.getProperty("preset_tags").toString().contains("Random Lab"))
        {
            std::cerr << "Saved preset metadata was not normalized correctly\n";
            cleanup();
            return 1;
        }
    }
    else
    {
        std::cerr << "Saved preset XML could not be parsed\n";
        cleanup();
        return 1;
    }

    auto foundInLibrary = false;
    for (const auto& preset : processor.getPresetLibrary())
    {
        if (preset.name != legalName)
            continue;

        foundInLibrary = preset.source == "Generated"
            && preset.tags.contains("Generated")
            && preset.tags.contains("Random Lab")
            && preset.tags.contains("UKG Dred Bass")
            && preset.notes.contains("Recipe: UKG Dred Bass");
        break;
    }

    if (! foundInLibrary)
    {
        std::cerr << "Generated preset metadata was not visible through the library scan\n";
        cleanup();
        return 1;
    }

    if (! processor.loadPreset(presetName))
    {
        std::cerr << "Saved categorized preset could not be loaded by name\n";
        cleanup();
        return 1;
    }

    cleanup();
    std::cout << "Preset save audit passed for categorized folder writes.\n";
    return 0;
}
