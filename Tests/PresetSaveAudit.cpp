#include "../Source/PluginProcessor.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <iostream>

namespace
{
bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}
}

int main()
{
    NateVSTAudioProcessor processor;

    if (! setPlainParameter(processor, Parameters::ID::macroTone, 0.24f)
        || ! setPlainParameter(processor, Parameters::ID::macroDirt, 0.12f)
        || ! setPlainParameter(processor, Parameters::ID::macroMotion, 0.62f)
        || ! setPlainParameter(processor, Parameters::ID::macroSpace, 0.48f)
        || ! setPlainParameter(processor, Parameters::ID::macroWeight, 0.70f)
        || ! setPlainParameter(processor, Parameters::ID::macroBounce, 0.36f)
        || ! setPlainParameter(processor, Parameters::ID::macroWarp, 0.82f)
        || ! setPlainParameter(processor, Parameters::ID::macroThrow, 0.28f)
        || ! setPlainParameter(processor, Parameters::ID::oscWarpB, 0.29f)
        || ! setPlainParameter(processor, Parameters::ID::oscWarpBMode, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Warp, 0.37f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WarpB, 0.21f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WarpMode, 3.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WarpBMode, 1.0f))
    {
        std::cerr << "Could not seed macro preview parameters\n";
        return 1;
    }

    NateVSTAudioProcessor::PresetSaveOptions options;
    options.category = "UKG\\Bass";
    options.author = "Preset Save Audit";
    options.pack = "UKG Basslines";
    options.key = "C Min";
    options.bpm = 132;
    options.generated = true;
    options.generatedRecipe = "UKG Dred Bass";
    options.notes = "Custom audit notes\nMacro intent: dark UKG sub";

    const auto presetName = "Preset Save Audit " + juce::String(static_cast<int>(juce::Time::getMillisecondCounter() % 1000000));
    auto legalName = juce::File::createLegalFileName(presetName);
    if (legalName.isEmpty())
        legalName = "Untitled";

    const auto expectedFile = processor.getPresetDirectory()
                                  .getChildFile("UKG")
                                  .getChildFile("Bass")
                                  .getChildFile(legalName)
                                  .withFileExtension(".natevstpreset");
    const auto siblingFile = processor.getPresetDirectory()
                                  .getChildFile("House")
                                  .getChildFile("Chords")
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

    auto cleanup = [&expectedFile, &siblingFile]
    {
        expectedFile.deleteFile();
        siblingFile.deleteFile();
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

        const auto osc2WarpState = state.getChildWithProperty("id", Parameters::ID::osc2Warp);
        if (! osc2WarpState.isValid()
            || std::abs(static_cast<float>(osc2WarpState.getProperty("value", -1.0f)) - 0.37f) > 0.002f)
        {
            std::cerr << "Saved preset did not preserve Osc 2 Warp\n";
            cleanup();
            return 1;
        }

        const auto oscWarpBState = state.getChildWithProperty("id", Parameters::ID::oscWarpB);
        const auto oscWarpBModeState = state.getChildWithProperty("id", Parameters::ID::oscWarpBMode);
        const auto osc2WarpBState = state.getChildWithProperty("id", Parameters::ID::osc2WarpB);
        const auto osc2WarpBModeState = state.getChildWithProperty("id", Parameters::ID::osc2WarpBMode);
        if (! oscWarpBState.isValid()
            || ! oscWarpBModeState.isValid()
            || ! osc2WarpBState.isValid()
            || ! osc2WarpBModeState.isValid()
            || std::abs(static_cast<float>(oscWarpBState.getProperty("value", -1.0f)) - 0.29f) > 0.002f
            || std::abs(static_cast<float>(oscWarpBModeState.getProperty("value", -1.0f)) - 2.0f) > 0.002f
            || std::abs(static_cast<float>(osc2WarpBState.getProperty("value", -1.0f)) - 0.21f) > 0.002f
            || std::abs(static_cast<float>(osc2WarpBModeState.getProperty("value", -1.0f)) - 1.0f) > 0.002f)
        {
            std::cerr << "Saved preset did not preserve dual warp B lanes\n";
            cleanup();
            return 1;
        }

        const auto osc2WarpModeState = state.getChildWithProperty("id", Parameters::ID::osc2WarpMode);
        if (! osc2WarpModeState.isValid()
            || std::abs(static_cast<float>(osc2WarpModeState.getProperty("value", -1.0f)) - 3.0f) > 0.002f)
        {
            std::cerr << "Saved preset did not preserve Osc 2 Warp Mode\n";
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
            || ! state.getProperty("preset_notes").toString().contains("Custom audit notes")
            || ! state.getProperty("preset_notes").toString().contains("Macro intent: dark UKG sub")
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
            && preset.notes.contains("Custom audit notes")
            && preset.notes.contains("dark UKG sub")
            && preset.macroSummary.contains("Warp 82")
            && preset.macroSummary.contains("Weight 70")
            && preset.macroValues.size() == 8
            && std::abs(preset.macroValues[0] - 0.24f) < 0.002f
            && std::abs(preset.macroValues[2] - 0.62f) < 0.002f
            && std::abs(preset.macroValues[6] - 0.82f) < 0.002f;
        break;
    }

    if (! foundInLibrary)
    {
        std::cerr << "Generated preset metadata was not visible through the library scan\n";
        cleanup();
        return 1;
    }

    if (! processor.userPresetExists(presetName, options.category))
    {
        std::cerr << "Saved preset existence check failed for first category\n";
        cleanup();
        return 1;
    }

    auto siblingOptions = options;
    siblingOptions.category = "House/Chords";
    siblingOptions.generated = false;
    siblingOptions.notes = "Sibling preset should not delete the UKG/Bass version";

    if (! processor.savePreset(presetName, siblingOptions))
    {
        std::cerr << "Saving same-name sibling preset returned false\n";
        cleanup();
        return 1;
    }

    if (! expectedFile.existsAsFile() || ! siblingFile.existsAsFile())
    {
        std::cerr << "Same-name presets were not preserved in separate category folders\n";
        cleanup();
        return 1;
    }

    if (! processor.userPresetExists(presetName, options.category)
        || ! processor.userPresetExists(presetName, siblingOptions.category))
    {
        std::cerr << "Preset existence checks failed for same-name category siblings\n";
        cleanup();
        return 1;
    }

    if (! processor.loadPreset(presetName))
    {
        std::cerr << "Saved categorized preset could not be loaded by name\n";
        cleanup();
        return 1;
    }

    if (auto* osc2Warp = processor.getValueTreeState().getRawParameterValue(Parameters::ID::osc2Warp);
        osc2Warp == nullptr || std::abs(osc2Warp->load() - 0.37f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 2 Warp\n";
        cleanup();
        return 1;
    }

    if (auto* oscWarpB = processor.getValueTreeState().getRawParameterValue(Parameters::ID::oscWarpB);
        oscWarpB == nullptr || std::abs(oscWarpB->load() - 0.29f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 1 Warp B\n";
        cleanup();
        return 1;
    }

    if (auto* osc2WarpB = processor.getValueTreeState().getRawParameterValue(Parameters::ID::osc2WarpB);
        osc2WarpB == nullptr || std::abs(osc2WarpB->load() - 0.21f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 2 Warp B\n";
        cleanup();
        return 1;
    }

    if (auto* oscWarpBMode = processor.getValueTreeState().getRawParameterValue(Parameters::ID::oscWarpBMode);
        oscWarpBMode == nullptr || std::abs(oscWarpBMode->load() - 2.0f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 1 Warp B Mode\n";
        cleanup();
        return 1;
    }

    if (auto* osc2WarpMode = processor.getValueTreeState().getRawParameterValue(Parameters::ID::osc2WarpMode);
        osc2WarpMode == nullptr || std::abs(osc2WarpMode->load() - 3.0f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 2 Warp Mode\n";
        cleanup();
        return 1;
    }

    if (auto* osc2WarpBMode = processor.getValueTreeState().getRawParameterValue(Parameters::ID::osc2WarpBMode);
        osc2WarpBMode == nullptr || std::abs(osc2WarpBMode->load() - 1.0f) > 0.002f)
    {
        std::cerr << "Loaded preset did not restore Osc 2 Warp B Mode\n";
        cleanup();
        return 1;
    }

    cleanup();
    std::cout << "Preset save audit passed for categorized folder writes.\n";
    return 0;
}
