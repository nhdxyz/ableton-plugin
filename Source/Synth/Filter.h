#pragma once

#include <juce_dsp/juce_dsp.h>

namespace Synth
{
class Filter
{
public:
    enum class Mode
    {
        lowpass = 0,
        bandpass,
        highpass
    };

    enum class Character
    {
        clean = 0,
        warm,
        acid,
        dirty
    };

    enum class Slope
    {
        twelveDb = 0,
        twentyFourDb
    };

    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void setMode(Mode mode);
    void setCharacter(Character newCharacter, float drive);
    void setSlope(Slope newSlope);
    void setCutoffAndResonance(float cutoffHz, float resonance);
    float process(float sample);

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::StateVariableTPTFilter<float> secondFilter;
    Character character = Character::clean;
    Slope slope = Slope::twelveDb;
    float characterDrive = 0.0f;

    float processFilterStack(float sample);
};
}
