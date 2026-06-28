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

    std::cout << "Random candidate audit passed for capture, cue, recall, and promotion.\n";
    return 0;
}
