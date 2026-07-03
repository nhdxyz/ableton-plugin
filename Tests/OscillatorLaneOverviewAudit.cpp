#include "../Source/UI/OscillatorLaneOverview.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
juce::MouseEvent makeMouseEvent(UI::OscillatorLaneOverview& overview,
                                float x,
                                float y,
                                float downX,
                                float downY,
                                juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier,
                                bool dragged = false)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),
                            { x, y },
                            mods,
                            1.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            &overview,
                            &overview,
                            now,
                            { downX, downY },
                            now,
                            1,
                            dragged);
}

UI::OscillatorLaneOverview::State makeState()
{
    UI::OscillatorLaneOverview::State state;
    state.summary = "2/2 active | 1 custom";
    state.osc2Selected = false;
    state.lanes = {
        UI::OscillatorLaneOverview::Lane {
            "OSC 1",
            "Custom",
            "Fold",
            "Bend",
            0.86f,
            0.18f,
            0.42f,
            0.24f,
            true,
            true,
            true
        },
        UI::OscillatorLaneOverview::Lane {
            "OSC 2",
            "Wavetable",
            "Sync",
            "Fold",
            0.48f,
            0.64f,
            0.31f,
            0.18f,
            true,
            true,
            false
        }
    };
    return state;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::OscillatorLaneOverview overview;
    overview.setBounds(0, 0, 640, 118);
    overview.setState(makeState());

    const auto metrics = overview.getLayoutMetricsForAudit();
    if (! metrics.readable || metrics.visibleLanes != 2 || metrics.selectedLanes != 1)
    {
        std::cerr << "Oscillator lane overview layout is not readable: lanes="
                  << metrics.visibleLanes
                  << " selected=" << metrics.selectedLanes
                  << " minLaneWidth=" << metrics.minLaneWidth
                  << " minLaneHeight=" << metrics.minLaneHeight << '\n';
        return 1;
    }

    if (! overview.getTooltip().contains("OSC 1")
        || ! overview.getTooltip().contains("OSC 2")
        || ! overview.getTooltip().contains("Fold"))
    {
        std::cerr << "Oscillator lane overview tooltip did not summarise both lanes\n";
        return 1;
    }

    juce::Image image(juce::Image::ARGB, overview.getWidth(), overview.getHeight(), true);
    juce::Graphics graphics(image);
    overview.paint(graphics);

    auto visiblePixels = 0;
    for (auto y = 0; y < image.getHeight(); y += 4)
        for (auto x = 0; x < image.getWidth(); x += 4)
            if (image.getPixelAt(x, y).getAlpha() > 0
                && image.getPixelAt(x, y) != juce::Colours::transparentBlack)
                ++visiblePixels;

    if (visiblePixels < 1300)
    {
        std::cerr << "Oscillator lane overview render appears too sparse: "
                  << visiblePixels << " sampled pixels\n";
        return 1;
    }

    std::vector<bool> selectedLanes;
    std::vector<bool> editStarts;
    std::vector<std::pair<bool, float>> changes;
    std::vector<bool> openedLanes;
    overview.onLaneSelected = [&selectedLanes] (bool osc2) { selectedLanes.push_back(osc2); };
    overview.onPositionEditStart = [&editStarts] (bool osc2) { editStarts.push_back(osc2); };
    overview.onPositionChange = [&changes] (bool osc2, float position) { changes.emplace_back(osc2, position); };
    overview.onOpenLaneEditor = [&openedLanes] (bool osc2) { openedLanes.push_back(osc2); };

    const auto laneY = 66.0f;
    const auto osc1StartX = 8.0f;
    const auto osc1Width = 310.0f;
    const auto osc1X = osc1StartX + (osc1Width * 0.72f);
    overview.mouseDown(makeMouseEvent(overview, osc1X, laneY, osc1X, laneY));
    overview.mouseUp(makeMouseEvent(overview, osc1X, laneY, osc1X, laneY));

    if (selectedLanes.empty() || selectedLanes.back())
    {
        std::cerr << "Osc 1 lane click did not select Osc 1\n";
        return 1;
    }

    if (editStarts.empty() || editStarts.back())
    {
        std::cerr << "Osc 1 lane click did not start an Osc 1 position edit\n";
        return 1;
    }

    if (changes.empty() || changes.back().first || std::abs(changes.back().second - 0.72f) > 0.05f)
    {
        std::cerr << "Osc 1 lane click emitted wrong WT position";
        if (! changes.empty())
            std::cerr << ": osc2=" << changes.back().first << " position=" << changes.back().second;
        std::cerr << '\n';
        return 1;
    }

    const auto osc2StartX = 324.0f;
    const auto osc2Width = 310.0f;
    const auto osc2DownX = osc2StartX + (osc2Width * 0.16f);
    const auto osc2DragX = osc2StartX + (osc2Width * 0.84f);
    overview.mouseDown(makeMouseEvent(overview, osc2DownX, laneY, osc2DownX, laneY));
    overview.mouseDrag(makeMouseEvent(overview, osc2DragX, laneY, osc2DownX, laneY, juce::ModifierKeys::leftButtonModifier, true));
    overview.mouseUp(makeMouseEvent(overview, osc2DragX, laneY, osc2DownX, laneY));

    if (selectedLanes.empty() || ! selectedLanes.back())
    {
        std::cerr << "Osc 2 lane drag did not select Osc 2\n";
        return 1;
    }

    if (changes.empty() || ! changes.back().first || std::abs(changes.back().second - 0.84f) > 0.05f)
    {
        std::cerr << "Osc 2 lane drag emitted wrong WT position";
        if (! changes.empty())
            std::cerr << ": osc2=" << changes.back().first << " position=" << changes.back().second;
        std::cerr << '\n';
        return 1;
    }

    overview.mouseDoubleClick(makeMouseEvent(overview, osc2DragX, laneY, osc2DragX, laneY));
    if (openedLanes.empty() || ! openedLanes.back())
    {
        std::cerr << "Osc 2 lane double-click did not open the Osc 2 editor\n";
        return 1;
    }

    std::cout << "Oscillator lane overview audit passed for layout, render coverage, lane selection, WT drag, and editor open gestures.\n";
    return 0;
}
