#pragma once

#include <juce_core/juce_core.h>

#include <array>

namespace UI::LfoCurveLibrary
{
juce::StringArray presetChoices();
std::array<float, 8> presetValues(int presetId);
double cyclesPerBeatForRateIndex(int rateIndex);
float evaluateCurve(const std::array<float, 8>& values, float phase);
float shapeValue(int shapeIndex, float phase, const std::array<float, 8>& curveValues);
}
