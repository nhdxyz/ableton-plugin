#include "../Source/UI/WavetableFrameStrip.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
struct FrameActionEvent
{
    bool osc2 = false;
    size_t frameIndex = 0;
    UI::WavetableFrameStrip::FrameAction action = UI::WavetableFrameStrip::FrameAction::copy;
};

juce::MouseEvent makeMouseEvent(UI::WavetableFrameStrip& strip,
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
                            &strip,
                            &strip,
                            now,
                            { downX, downY },
                            now,
                            1,
                            dragged);
}

UI::WavetableFrameStrip::CustomPointArray makeFrame(float phaseOffset)
{
    UI::WavetableFrameStrip::CustomPointArray frame {};
    const auto lastIndex = static_cast<float>(frame.size() - 1);

    for (size_t point = 0; point < frame.size(); ++point)
    {
        const auto phase = (static_cast<float>(point) / lastIndex) + phaseOffset;
        frame[point] = 0.5f + (std::sin(phase * juce::MathConstants<float>::twoPi) * 0.48f);
    }

    return frame;
}

UI::WavetableFrameStrip::State makeState()
{
    UI::WavetableFrameStrip::State state;
    state.summary = "O1 custom | O2 custom";
    state.osc1.label = "OSC 1";
    state.osc1.detail = "Custom | 86%";
    state.osc1.position = 0.18f;
    state.osc1.active = true;
    state.osc2.label = "OSC 2";
    state.osc2.detail = "Custom | 42%";
    state.osc2.position = 0.64f;
    state.osc2.active = true;

    for (size_t frame = 0; frame < UI::WavetableFrameStrip::frameCount; ++frame)
    {
        state.osc1.frames[frame] = makeFrame(static_cast<float>(frame) * 0.017f);
        state.osc2.frames[frame] = makeFrame(0.25f + (static_cast<float>(frame) * 0.031f));
    }

    return state;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::WavetableFrameStrip strip;
    strip.setBounds(0, 0, 640, 190);
    strip.setState(makeState());

    const auto metrics = strip.getLayoutMetricsForAudit();
    if (! metrics.readable || metrics.visibleFrameCards != 16 || metrics.selectedFrameCards != 2)
    {
        std::cerr << "Wavetable frame strip layout is not readable: cards="
                  << metrics.visibleFrameCards
                  << " selected=" << metrics.selectedFrameCards
                  << " minFrameWidth=" << metrics.minFrameWidth
                  << " minLaneHeight=" << metrics.minLaneHeight << '\n';
        return 1;
    }

    juce::Image image(juce::Image::ARGB, strip.getWidth(), strip.getHeight(), true);
    juce::Graphics graphics(image);
    strip.paint(graphics);

    auto visiblePixels = 0;
    for (auto y = 0; y < image.getHeight(); y += 5)
        for (auto x = 0; x < image.getWidth(); x += 5)
            if (image.getPixelAt(x, y).getAlpha() > 0
                && image.getPixelAt(x, y) != juce::Colours::transparentBlack)
                ++visiblePixels;

    if (visiblePixels < 900)
    {
        std::cerr << "Wavetable frame strip render appears too sparse: "
                  << visiblePixels << " sampled pixels\n";
        return 1;
    }

    std::vector<bool> editStarts;
    std::vector<std::pair<bool, float>> changes;
    std::vector<FrameActionEvent> frameActions;
    strip.onPositionEditStart = [&editStarts] (bool osc2) { editStarts.push_back(osc2); };
    strip.onPositionChange = [&changes] (bool osc2, float position) { changes.emplace_back(osc2, position); };
    strip.onFrameAction = [&frameActions] (bool osc2, size_t frameIndex, UI::WavetableFrameStrip::FrameAction action)
    {
        frameActions.push_back({ osc2, frameIndex, action });
    };

    const auto lane1Y = 58.0f;
    const auto lane2Y = 136.0f;
    const auto railStartX = 78.0f;
    const auto railEndX = 624.0f;
    const auto railWidth = railEndX - railStartX;
    const auto expectedOsc1Frame = 5.0f / static_cast<float>(UI::WavetableFrameStrip::frameCount - 1);
    const auto osc1X = railStartX + ((railEndX - railStartX) * 0.72f);
    strip.mouseDown(makeMouseEvent(strip, osc1X, lane1Y, osc1X, lane1Y));
    strip.mouseUp(makeMouseEvent(strip, osc1X, lane1Y, osc1X, lane1Y));

    if (editStarts.empty() || editStarts.back())
    {
        std::cerr << "Osc 1 frame-strip scan did not start an Osc 1 edit\n";
        return 1;
    }

    if (changes.empty() || changes.back().first || std::abs(changes.back().second - expectedOsc1Frame) > 0.005f)
    {
        std::cerr << "Osc 1 frame-card click did not snap to the exact frame position";
        if (! changes.empty())
            std::cerr << ": osc2=" << changes.back().first << " position=" << changes.back().second;
        std::cerr << '\n';
        return 1;
    }

    if (! frameActions.empty())
    {
        std::cerr << "Plain frame selection emitted a frame-card edit action\n";
        return 1;
    }

    const auto frameActionChangesBefore = changes.size();
    const auto frameActionEditStartsBefore = editStarts.size();
    const auto osc1Frame3X = railStartX + (railWidth * (2.5f / static_cast<float>(UI::WavetableFrameStrip::frameCount)));
    const auto altClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::altModifier);
    strip.mouseDown(makeMouseEvent(strip, osc1Frame3X, lane1Y, osc1Frame3X, lane1Y, altClick));
    strip.mouseUp(makeMouseEvent(strip, osc1Frame3X, lane1Y, osc1Frame3X, lane1Y, altClick));

    if (frameActions.empty()
        || frameActions.back().osc2
        || frameActions.back().frameIndex != 2
        || frameActions.back().action != UI::WavetableFrameStrip::FrameAction::copy)
    {
        std::cerr << "Option-click on Osc 1 frame 3 did not emit a copy action\n";
        return 1;
    }

    const auto osc2Frame7X = railStartX + (railWidth * (6.5f / static_cast<float>(UI::WavetableFrameStrip::frameCount)));
    const auto commandClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::commandModifier);
    strip.mouseDown(makeMouseEvent(strip, osc2Frame7X, lane2Y, osc2Frame7X, lane2Y, commandClick));
    strip.mouseUp(makeMouseEvent(strip, osc2Frame7X, lane2Y, osc2Frame7X, lane2Y, commandClick));

    if (frameActions.empty()
        || ! frameActions.back().osc2
        || frameActions.back().frameIndex != 6
        || frameActions.back().action != UI::WavetableFrameStrip::FrameAction::paste)
    {
        std::cerr << "Command-click on Osc 2 frame 7 did not emit a paste action\n";
        return 1;
    }

    const auto osc2Frame2X = railStartX + (railWidth * (1.5f / static_cast<float>(UI::WavetableFrameStrip::frameCount)));
    const auto shiftClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);
    strip.mouseDown(makeMouseEvent(strip, osc2Frame2X, lane2Y, osc2Frame2X, lane2Y, shiftClick));
    strip.mouseUp(makeMouseEvent(strip, osc2Frame2X, lane2Y, osc2Frame2X, lane2Y, shiftClick));

    if (frameActions.empty()
        || ! frameActions.back().osc2
        || frameActions.back().frameIndex != 1
        || frameActions.back().action != UI::WavetableFrameStrip::FrameAction::storeMorph)
    {
        std::cerr << "Shift-click on Osc 2 frame 2 did not emit a store-morph action\n";
        return 1;
    }

    if (changes.size() != frameActionChangesBefore || editStarts.size() != frameActionEditStartsBefore)
    {
        std::cerr << "Frame-card edit gestures also emitted position scan events\n";
        return 1;
    }

    const auto osc2StartX = railStartX + ((railEndX - railStartX) * 0.20f);
    const auto osc2EndX = railStartX + ((railEndX - railStartX) * 0.86f);
    strip.mouseDown(makeMouseEvent(strip, osc2StartX, lane2Y, osc2StartX, lane2Y));
    strip.mouseDrag(makeMouseEvent(strip, osc2EndX, lane2Y - 22.0f, osc2StartX, lane2Y, juce::ModifierKeys::leftButtonModifier, true));
    strip.mouseUp(makeMouseEvent(strip, osc2EndX, lane2Y - 22.0f, osc2StartX, lane2Y));

    if (editStarts.empty() || ! editStarts.back())
    {
        std::cerr << "Osc 2 frame-strip scan did not start an Osc 2 edit\n";
        return 1;
    }

    if (changes.empty() || ! changes.back().first || std::abs(changes.back().second - 0.86f) > 0.04f)
    {
        std::cerr << "Osc 2 frame-strip drag emitted wrong position";
        if (! changes.empty())
            std::cerr << ": osc2=" << changes.back().first << " position=" << changes.back().second;
        std::cerr << '\n';
        return 1;
    }

    std::cout << "Wavetable frame strip audit passed for layout, render coverage, exact frame selection, direct frame-card actions, and Osc 1/Osc 2 scan gestures.\n";
    return 0;
}
