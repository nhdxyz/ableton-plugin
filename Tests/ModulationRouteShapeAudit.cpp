#include "../Source/Modulation/LfoShapes.h"
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

    auto freeLfoPhase = 0.9f;
    const auto freePhaseUpdate = Modulation::updateLfoPhase(freeLfoPhase,
                                                            false,
                                                            std::nullopt,
                                                            1.0,
                                                            2.0f,
                                                            4410,
                                                            44100.0);
    if (freePhaseUpdate.syncedToHost
        || ! freePhaseUpdate.wrappedCycle
        || ! expectNear("Free LFO phase before advance", freePhaseUpdate.phaseForShape, 0.9f)
        || ! expectNear("Free LFO phase advances and wraps", freeLfoPhase, 0.1f))
    {
        return 1;
    }

    auto syncedLfoPhase = 0.2f;
    const auto syncedPhaseUpdate = Modulation::updateLfoPhase(syncedLfoPhase,
                                                              true,
                                                              0.125,
                                                              4.0,
                                                              8.0f,
                                                              512,
                                                              44100.0);
    if (! syncedPhaseUpdate.syncedToHost
        || syncedPhaseUpdate.wrappedCycle
        || ! expectNear("Synced LFO phase follows PPQ", syncedPhaseUpdate.phaseForShape, 0.5f)
        || ! expectNear("Synced LFO phase does not free-advance", syncedLfoPhase, 0.5f))
    {
        return 1;
    }

    syncedLfoPhase = 0.9f;
    const auto wrappedSyncedPhaseUpdate = Modulation::updateLfoPhase(syncedLfoPhase,
                                                                     true,
                                                                     0.125,
                                                                     4.0,
                                                                     8.0f,
                                                                     512,
                                                                     44100.0);
    if (! wrappedSyncedPhaseUpdate.syncedToHost
        || ! wrappedSyncedPhaseUpdate.wrappedCycle
        || ! expectNear("Synced LFO detects host wrap", syncedLfoPhase, 0.5f))
    {
        return 1;
    }

    const std::array<float, 8> curveValues { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f };
    std::array<std::atomic<float>, 8> atomicCurveValues {};
    std::array<std::atomic<float>*, 8> atomicCurvePointers {};
    for (size_t index = 0; index < atomicCurvePointers.size(); ++index)
    {
        atomicCurveValues[index].store(curveValues[index]);
        atomicCurvePointers[index] = &atomicCurveValues[index];
    }

    if (! expectNear("LFO phase wraps positive", Modulation::LfoShapes::normalisePhase(1.25f), 0.25f)
        || ! expectNear("LFO phase wraps negative", Modulation::LfoShapes::normalisePhase(-0.25f), 0.75f)
        || ! expectNear("LFO sine shape", Modulation::LfoShapes::shapeValue(0, 0.25f), 1.0f)
        || ! expectNear("LFO triangle shape", Modulation::LfoShapes::shapeValue(1, 0.25f), 1.0f)
        || ! expectNear("LFO saw shape", Modulation::LfoShapes::shapeValue(2, 0.75f), 0.5f)
        || ! expectNear("LFO square shape", Modulation::LfoShapes::shapeValue(3, 0.75f), -1.0f)
        || ! expectNear("LFO held step shape", Modulation::LfoShapes::shapeValue(4, 0.75f, -0.37f), -0.37f)
        || ! expectNear("LFO preview step shape", Modulation::LfoShapes::previewStepValue(0.62f), 0.55f)
        || ! expectNear("LFO curve shape", Modulation::LfoShapes::shapeValueWithCurve(5, 0.125f, curveValues), -0.5f)
        || ! expectNear("LFO atomic curve shape", Modulation::LfoShapes::shapeValueWithCurve(5, 0.125f, atomicCurvePointers), -0.5f))
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

    rangeMin.store(0.35f);
    rangeMax.store(0.75f);
    smoothed = -0.8f;
    const auto rangeClampedSlewed = Modulation::processRouteValue(1.0f, &polarity, &curve, &rangeMin, &rangeMax, &slew, smoothed, 16, 44100.0);
    if (rangeClampedSlewed < 0.35f || rangeClampedSlewed > 0.75f)
    {
        std::cerr << "Slewed route escaped current range: " << rangeClampedSlewed << '\n';
        return 1;
    }

    rangeMin.store(-1.0f);
    rangeMax.store(1.0f);

    Modulation::RouteRuntimeState routeState;
    if (! Modulation::prepareRouteState(routeState, 1, 1, 0, 0, true))
    {
        std::cerr << "Route state did not activate\n";
        return 1;
    }

    routeState.smoothedValue = 0.66f;
    if (! Modulation::prepareRouteState(routeState, 1, 1, 0, 0, true)
        || ! expectNear("Route state preserves active identity", routeState.smoothedValue, 0.66f))
    {
        return 1;
    }

    if (! Modulation::prepareRouteState(routeState, 1, 2, 0, 0, true)
        || ! expectNear("Route state resets on retarget", routeState.smoothedValue, 0.0f))
    {
        return 1;
    }

    routeState.smoothedValue = 0.74f;
    if (! Modulation::prepareRouteState(routeState, 1, 2, 1, 0, true)
        || ! expectNear("Route state resets on polarity change", routeState.smoothedValue, 0.0f))
    {
        return 1;
    }

    routeState.smoothedValue = 0.42f;
    if (! Modulation::prepareRouteState(routeState, 1, 2, 1, 2, true)
        || ! expectNear("Route state resets on curve change", routeState.smoothedValue, 0.0f))
    {
        return 1;
    }

    routeState.smoothedValue = 0.5f;
    if (Modulation::prepareRouteState(routeState, 1, 2, 1, 2, false)
        || routeState.active
        || routeState.sourceIndex != 0
        || routeState.destinationIndex != 0
        || routeState.polarityIndex != 0
        || routeState.curveIndex != 0
        || ! expectNear("Route state resets when inactive", routeState.smoothedValue, 0.0f))
    {
        std::cerr << "Route state did not clear when inactive\n";
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
    if (! expectNear("Step LFO synced phase does not free-advance", stepPhase, 0.5f))
        return 1;

    std::cout << "Modulation route shape audit passed.\n";
    return 0;
}
