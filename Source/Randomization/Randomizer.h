#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <random>

namespace Randomization
{
enum class Recipe
{
    deepHouseBass = 0,
    rollingTechBass,
    acidLine,
    minimalBlip,
    darkStab,
    noiseFx,
    ukgTwoStepBass,
    ukgOrganStab,
    ukgChordStab,
    ukgBellPluck,
    ukgDredBass
};

class Randomizer
{
public:
    explicit Randomizer(Parameters::APVTS& parameters);

    void generate();
    void mutate();
    void wildMutate();
    void variation();

private:
    Parameters::APVTS& parameters;
    std::mt19937 rng;

    float randomFloat(float min, float max);
    int randomInt(int min, int max);
    float readFloat(const juce::String& parameterID) const;
    int readChoice(const juce::String& parameterID) const;
    void setParameter(const juce::String& parameterID, float plainValue);
    void setChoice(const juce::String& parameterID, int choiceIndex);

    void randomizeForRecipe(Recipe recipe, float amount, float chaos, bool subtle);
    void applyOutputSafety(float drive);
};
}
