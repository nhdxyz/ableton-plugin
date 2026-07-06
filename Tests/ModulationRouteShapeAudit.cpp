#include "../Source/Modulation/ModulationRouting.h"

#include <cmath>
#include <iostream>

namespace
{
bool near(float actual, float expected, float tolerance = 0.001f)
{
    return std::abs(actual - expected) <= tolerance;
}

bool expectNear(const char* label, float actual, float expected, float tolerance = 0.001f)
{
    if (near(actual, expected, tolerance))
        return true;

    std::cerr << label << " expected " << expected << " but got " << actual << '\n';
    return false;
}
}

int main()
{
    std::atomic<float> polarity { 0.0f };
    std::atomic<float> curve { 0.0f };
    std::atomic<float> rangeMin { -1.0f };
    std::atomic<float> rangeMax { 1.0f };
    std::atomic<float> slew { 0.0f };
    auto smoothed = 0.0f;

    if (! expectNear("Neutral route", Modulation::processRouteValue(0.42f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), 0.42f))
        return 1;

    const auto positivePpqPhase = Modulation::phaseFromPpq(1.125, 4.0);
    const auto negativePpqPhase = Modulation::phaseFromPpq(-0.125, 4.0);
    if (! positivePpqPhase.has_value()
        || ! negativePpqPhase.has_value()
        || ! expectNear("Positive PPQ phase wraps", *positivePpqPhase, 0.5f)
        || ! expectNear("Negative PPQ phase wraps", *negativePpqPhase, 0.5f))
    {
        return 1;
    }

    for (auto destination = 0; destination <= 21; ++destination)
    {
        const auto shouldBeSynth = (destination >= 1 && destination <= 6)
            || (destination >= 17 && destination <= 19);
        const auto shouldBeFx = (destination >= 7 && destination <= 11)
            || (destination >= 20 && destination <= 21);
        const auto shouldBeSample = destination >= 12 && destination <= 16;

        if (Modulation::isSynthDestination(destination) != shouldBeSynth
            || Modulation::isFxDestination(destination) != shouldBeFx
            || Modulation::isSampleDestination(destination) != shouldBeSample)
        {
            std::cerr << "Destination domain mapping failed for destination " << destination << '\n';
            return 1;
        }
    }

    polarity.store(1.0f);
    if (! expectNear("Unipolar low", Modulation::processRouteValue(-1.0f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), 0.0f)
        || ! expectNear("Unipolar high", Modulation::processRouteValue(1.0f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), 1.0f))
    {
        return 1;
    }

    polarity.store(3.0f);
    if (! expectNear("Inverted route", Modulation::processRouteValue(0.25f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), -0.25f))
        return 1;

    polarity.store(0.0f);
    curve.store(4.0f);
    if (! expectNear("Gated below threshold", Modulation::processRouteValue(0.2f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), 0.0f)
        || ! expectNear("Gated above threshold", Modulation::processRouteValue(-0.8f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), -1.0f))
    {
        return 1;
    }

    curve.store(0.0f);
    rangeMin.store(-0.25f);
    rangeMax.store(0.25f);
    if (! expectNear("Range clamp high", Modulation::processRouteValue(0.8f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), 0.25f)
        || ! expectNear("Range clamp low", Modulation::processRouteValue(-0.8f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0), -0.25f))
    {
        return 1;
    }

    rangeMin.store(-1.0f);
    rangeMax.store(1.0f);
    slew.store(1.0f);
    smoothed = 0.0f;
    const auto firstSlewed = Modulation::processRouteValue(1.0f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0);
    const auto laterSlewed = Modulation::processRouteValue(1.0f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 44100, 44100.0);
    if (! (firstSlewed > 0.0f && firstSlewed < 0.2f && laterSlewed > 0.85f))
    {
        std::cerr << "Slew did not ramp as expected: first=" << firstSlewed << " later=" << laterSlewed << '\n';
        return 1;
    }

    std::atomic<float> stepSync { 0.0f };
    std::atomic<float> stepSyncRate { 3.0f };
    std::atomic<float> stepRate { 2.0f };
    std::atomic<float> stepDepth { 0.5f };
    std::atomic<float> stepSlew { 0.0f };
    std::array<std::atomic<float>, 8> ownedStepValues {};
    const std::array<float, 8> stepValues { 1.0f, -1.0f, 0.5f, -0.5f, 0.25f, -0.25f, 0.0f, 0.75f };
    std::array<std::atomic<float>*, 8> stepValuePointers {};
    for (size_t index = 0; index < stepValuePointers.size(); ++index)
    {
        ownedStepValues[index].store(stepValues[index]);
        stepValuePointers[index] = &ownedStepValues[index];
    }

    auto stepPhase = 0.0f;
    auto stepSmoothed = 0.0f;
    const auto stepOutput = Modulation::processStepLfo(&stepSync,
                                                       &stepSyncRate,
                                                       &stepRate,
                                                       &stepDepth,
                                                       &stepSlew,
                                                       stepValuePointers,
                                                       stepPhase,
                                                       stepSmoothed,
                                                       16,
                                                       44100.0,
                                                       124.0);
    if (! expectNear("Step LFO first step", stepOutput, 0.5f))
        return 1;

    stepSync.store(1.0f);
    stepPhase = 0.0f;
    stepSmoothed = 0.0f;
    const auto syncedStepOutput = Modulation::processStepLfo(&stepSync,
                                                             &stepSyncRate,
                                                             &stepRate,
                                                             &stepDepth,
                                                             &stepSlew,
                                                             stepValuePointers,
                                                             stepPhase,
                                                             stepSmoothed,
                                                             16,
                                                             44100.0,
                                                             124.0,
                                                             0.125);
    if (! expectNear("Step LFO follows PPQ phase", syncedStepOutput, 0.125f))
        return 1;

    std::cout << "Modulation route shape audit passed.\n";
    return 0;
}
