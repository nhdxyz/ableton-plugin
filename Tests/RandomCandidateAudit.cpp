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
    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, 5432.0f))
    {
        std::cerr << "Could not edit cutoff before candidate recall\n";
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

    std::cout << "Random candidate audit passed for capture and recall.\n";
    return 0;
}
