#include "../Source/UI/SampleWaveformDisplay.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
juce::MouseEvent makeMouseEvent(UI::SampleWaveformDisplay& display,
                                float x,
                                float y,
                                float downX,
                                float downY,
                                bool dragged)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),
                            { x, y },
                            juce::ModifierKeys::leftButtonModifier,
                            1.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            &display,
                            &display,
                            now,
                            { downX, downY },
                            now,
                            1,
                            dragged);
}

Sampler::SamplePeakOverview makeOverview()
{
    Sampler::SamplePeakOverview overview;
    overview.fileName = "audit.wav";
    overview.totalSamples = 44100;
    overview.sourceSampleRate = 44100.0;
    overview.minimums.resize(128);
    overview.maximums.resize(128);
    for (size_t index = 0; index < overview.minimums.size(); ++index)
    {
        const auto value = static_cast<float>(std::sin(static_cast<double>(index) * 0.16) * 0.65);
        overview.minimums[index] = juce::jmin(0.0f, value);
        overview.maximums[index] = juce::jmax(0.0f, value);
    }

    return overview;
}

std::array<UI::SampleWaveformDisplay::SliceMarker, 8> makeMarkers()
{
    std::array<UI::SampleWaveformDisplay::SliceMarker, 8> markers;
    for (size_t index = 0; index < markers.size(); ++index)
    {
        markers[index].start = static_cast<float>(index) / static_cast<float>(markers.size());
        markers[index].end = static_cast<float>(index + 1) / static_cast<float>(markers.size());
        markers[index].custom = index == 3;
    }

    return markers;
}

float xForNormalised(float normalised)
{
    constexpr auto width = 800.0f;
    constexpr auto plotX = 10.0f;
    constexpr auto plotWidth = width - 20.0f;
    return plotX + (plotWidth * normalised);
}

bool near(float value, float expected, float tolerance = 0.012f)
{
    return std::abs(value - expected) <= tolerance;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::SampleWaveformDisplay display;
    display.setBounds(0, 0, 800, 220);
    display.setOverview(makeOverview());
    display.setRange(0.25f, 0.5f);
    display.setSliceMarkers(makeMarkers());

    std::vector<std::pair<size_t, float>> boundaryEvents;
    auto selectedSlice = -1;
    display.onSliceBoundaryChange = [&boundaryEvents] (size_t boundaryIndex, float position)
    {
        boundaryEvents.emplace_back(boundaryIndex, position);
    };
    display.onSliceSelected = [&selectedSlice] (size_t sliceIndex)
    {
        selectedSlice = static_cast<int>(sliceIndex);
    };

    constexpr auto laneY = 188.0f;
    const auto boundaryX = xForNormalised(0.375f);
    const auto targetX = xForNormalised(0.46f);
    display.mouseDown(makeMouseEvent(display, boundaryX, laneY, boundaryX, laneY, false));
    display.mouseDrag(makeMouseEvent(display, targetX, laneY, boundaryX, laneY, true));
    display.mouseUp(makeMouseEvent(display, targetX, laneY, boundaryX, laneY, true));

    if (boundaryEvents.empty())
    {
        std::cerr << "Slice boundary drag did not emit any boundary events\n";
        return 1;
    }

    const auto [boundaryIndex, boundaryPosition] = boundaryEvents.back();
    if (boundaryIndex != 3 || ! near(boundaryPosition, 0.46f))
    {
        std::cerr << "Unexpected boundary event: index=" << boundaryIndex
                  << " position=" << boundaryPosition << '\n';
        return 1;
    }

    if (selectedSlice >= 0)
    {
        std::cerr << "Boundary drag should not also fire slice selection, got S"
                  << (selectedSlice + 1) << '\n';
        return 1;
    }

    selectedSlice = -1;
    boundaryEvents.clear();
    const auto sliceCentreX = xForNormalised(0.44f);
    display.mouseDown(makeMouseEvent(display, sliceCentreX, laneY, sliceCentreX, laneY, false));
    display.mouseUp(makeMouseEvent(display, sliceCentreX, laneY, sliceCentreX, laneY, false));

    if (selectedSlice != 3)
    {
        std::cerr << "Slice lane click selected " << selectedSlice
                  << " instead of slice 4\n";
        return 1;
    }

    if (! boundaryEvents.empty())
    {
        std::cerr << "Slice lane centre click should not emit boundary edits\n";
        return 1;
    }

    std::cout << "Sample waveform display audit passed for direct slice-boundary dragging and slice-lane selection.\n";
    return 0;
}
