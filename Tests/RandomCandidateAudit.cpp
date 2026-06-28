#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
float readPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float fallback)
{
    if (auto* value = processor.getValueTreeState().getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

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

    if (! setPlainParameter(processor, Parameters::ID::randomRecipe, 6.0f)
        || ! setPlainParameter(processor, Parameters::ID::randomAmount, 0.72f)
        || ! setPlainParameter(processor, Parameters::ID::randomChaos, 0.28f))
    {
        std::cerr << "Could not seed randomizer parameters\n";
        return 1;
    }

    processor.generateRandomPatch();

    if (! processor.hasRandomCandidate(0))
    {
        std::cerr << "First random candidate was not captured\n";
        return 1;
    }

    const auto firstSummary = processor.getRandomCandidateSummary(0);
    if (! firstSummary.containsIgnoreCase("Generate") || ! firstSummary.containsIgnoreCase("UKG"))
    {
        std::cerr << "First candidate summary is not descriptive: " << firstSummary << '\n';
        return 1;
    }

    const auto generatedCutoff = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    const auto editedCutoff = juce::jmax(20.0f, generatedCutoff * 0.35f);
    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, editedCutoff))
    {
        std::cerr << "Could not edit cutoff before candidate recall\n";
        return 1;
    }

    const auto compareSummary = processor.getRandomCandidateCompareSummary(0);
    if (! compareSummary.containsIgnoreCase("brighter"))
    {
        std::cerr << "Candidate compare summary did not report brightness: "
                  << compareSummary << '\n';
        return 1;
    }

    const auto changedSections = processor.getRandomCandidateChangedSectionsSummary(0);
    const auto changedSectionCount = processor.getRandomCandidateChangedSectionCount(0);
    const auto diffSummary = processor.getRandomCandidateDiffSummary(0);
    if (changedSectionCount < 1
        || ! changedSections.containsIgnoreCase("Filter")
        || ! diffSummary.containsIgnoreCase("Cutoff")
        || ! diffSummary.contains("->"))
    {
        std::cerr << "Candidate diff summary was not descriptive. Sections: "
                  << changedSections << " count " << changedSectionCount
                  << " diff " << diffSummary << '\n';
        return 1;
    }

    if (! processor.beginRandomCandidateAudition(0))
    {
        std::cerr << "Candidate audition did not start\n";
        return 1;
    }

    const auto auditionCutoff = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    if (std::abs(auditionCutoff - generatedCutoff) > 0.01f)
    {
        std::cerr << "Candidate audition did not apply candidate cutoff. Expected "
                  << generatedCutoff << " got " << auditionCutoff << '\n';
        return 1;
    }

    if (! processor.endRandomCandidateAudition())
    {
        std::cerr << "Candidate audition did not restore\n";
        return 1;
    }

    const auto restoredCutoff = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    if (std::abs(restoredCutoff - editedCutoff) > 0.01f)
    {
        std::cerr << "Candidate audition did not restore edited cutoff. Expected "
                  << editedCutoff << " got " << restoredCutoff << '\n';
        return 1;
    }

    NateVSTAudioProcessor::PresetSaveOptions candidateSaveOptions;
    candidateSaveOptions.category = "UKG\\Bass";
    candidateSaveOptions.author = "Random Candidate Audit";
    candidateSaveOptions.pack = "UKG Basslines";
    candidateSaveOptions.key = "C Min";
    candidateSaveOptions.bpm = 132;
    candidateSaveOptions.generated = true;
    candidateSaveOptions.generatedRecipe = "Candidate Save Audit";

    const auto candidatePresetName = "Candidate Save Audit "
        + juce::String(static_cast<int>(juce::Time::getMillisecondCounter() % 1000000));
    auto candidateLegalName = juce::File::createLegalFileName(candidatePresetName);
    if (candidateLegalName.isEmpty())
        candidateLegalName = "Untitled";

    const auto candidatePresetFile = processor.getPresetDirectory()
                                         .getChildFile("UKG")
                                         .getChildFile("Bass")
                                         .getChildFile(candidateLegalName)
                                         .withFileExtension(".natevstpreset");

    if (! processor.saveRandomCandidatePreset(0, candidatePresetName, candidateSaveOptions))
    {
        std::cerr << "Candidate preset save returned false\n";
        return 1;
    }

    const auto cutoffAfterCandidateSave = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    if (std::abs(cutoffAfterCandidateSave - editedCutoff) > 0.01f)
    {
        std::cerr << "Candidate preset save changed current cutoff. Expected "
                  << editedCutoff << " got " << cutoffAfterCandidateSave << '\n';
        candidatePresetFile.deleteFile();
        return 1;
    }

    if (! candidatePresetFile.existsAsFile())
    {
        std::cerr << "Candidate preset file was not created: "
                  << candidatePresetFile.getFullPathName() << '\n';
        return 1;
    }

    if (! processor.setPresetFavorite(candidateLegalName, true)
        || ! processor.isPresetFavorite(candidateLegalName)
        || ! processor.setPresetRating(candidateLegalName, 5)
        || processor.getPresetRating(candidateLegalName) != 5)
    {
        std::cerr << "Candidate preset favorite/rating handoff failed\n";
        candidatePresetFile.deleteFile();
        processor.setPresetFavorite(candidateLegalName, false);
        processor.setPresetRating(candidateLegalName, 0);
        return 1;
    }

    if (auto xml = juce::XmlDocument::parse(candidatePresetFile))
    {
        const auto state = juce::ValueTree::fromXml(*xml);
        const auto cutoffState = state.getChildWithProperty("id", Parameters::ID::filterCutoff);
        const auto savedCutoff = static_cast<float>(cutoffState.getProperty("value", -1.0f));

        if (! state.isValid()
            || state.getProperty("preset_category").toString() != "UKG/Bass"
            || state.getProperty("preset_folder").toString() != "UKG/Bass"
            || state.getProperty("preset_source").toString() != "Generated"
            || state.getProperty("preset_generated_recipe").toString() != "Candidate Save Audit"
            || ! state.getProperty("preset_tags").toString().contains("Generated")
            || ! state.getProperty("preset_tags").toString().contains("Random Lab")
            || std::abs(savedCutoff - generatedCutoff) > 0.01f)
        {
            std::cerr << "Saved candidate preset did not preserve generated metadata or candidate cutoff\n";
            candidatePresetFile.deleteFile();
            processor.setPresetFavorite(candidateLegalName, false);
            processor.setPresetRating(candidateLegalName, 0);
            return 1;
        }
    }
    else
    {
        std::cerr << "Candidate preset XML could not be parsed\n";
        candidatePresetFile.deleteFile();
        processor.setPresetFavorite(candidateLegalName, false);
        processor.setPresetRating(candidateLegalName, 0);
        return 1;
    }

    if (! processor.setPresetFavorite(candidateLegalName, false)
        || processor.isPresetFavorite(candidateLegalName)
        || ! processor.setPresetRating(candidateLegalName, 0)
        || processor.getPresetRating(candidateLegalName) != 0)
    {
        std::cerr << "Candidate preset favorite/rating clear failed\n";
        candidatePresetFile.deleteFile();
        return 1;
    }

    candidatePresetFile.deleteFile();

    if (! processor.recallRandomCandidate(0))
    {
        std::cerr << "Candidate recall failed\n";
        return 1;
    }

    const auto recalledCutoff = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    if (std::abs(recalledCutoff - generatedCutoff) > 0.01f)
    {
        std::cerr << "Candidate recall did not restore cutoff. Expected "
                  << generatedCutoff << " got " << recalledCutoff << '\n';
        return 1;
    }

    if (! processor.promoteRandomCandidateToPerformanceSnapshot(0, 0)
        || ! processor.hasPerformanceSnapshot(0))
    {
        std::cerr << "Candidate promotion to snapshot A failed\n";
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, juce::jmax(20.0f, generatedCutoff * 0.45f))
        || ! processor.recallPerformanceSnapshot(0))
    {
        std::cerr << "Promoted candidate snapshot could not be recalled\n";
        return 1;
    }

    const auto promotedCutoff = readPlainParameter(processor, Parameters::ID::filterCutoff, -1.0f);
    if (std::abs(promotedCutoff - generatedCutoff) > 0.01f)
    {
        std::cerr << "Promoted candidate snapshot did not restore cutoff. Expected "
                  << generatedCutoff << " got " << promotedCutoff << '\n';
        return 1;
    }

    processor.mutateRandomPatch(1);
    processor.createRandomVariation(3);
    processor.wildMutateRandomPatch(5);

    for (auto slot = 0; slot < 4; ++slot)
    {
        if (! processor.hasRandomCandidate(slot))
        {
            std::cerr << "Random candidate slot " << slot << " was not filled\n";
            return 1;
        }
    }

    if (processor.recallRandomCandidate(7))
    {
        std::cerr << "Out-of-range candidate recall unexpectedly succeeded\n";
        return 1;
    }

    std::cout << "Random candidate audit passed for capture, diff, cue, save, recall, and promotion.\n";
    return 0;
}
