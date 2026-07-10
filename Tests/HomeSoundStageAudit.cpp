#include "../Source/UI/HomeSoundStage.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <iostream>

namespace
{
juce::MouseEvent makeMouseEvent(UI::HomeSoundStage& stage,
                                float x,
                                float y,
                                float downX,
                                float downY,
                                bool dragged = false,
                                int clicks = 1)
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
                            &stage,
                            &stage,
                            now,
                            { downX, downY },
                            now,
                            clicks,
                            dragged);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::HomeSoundStage stage;
    stage.setBounds(0, 0, 460, 310);

    UI::HomeSoundStage::State state;
    state.presetName = "Audit Motion Stack";
    state.safetyName = "SAFE";
    state.perspective = true;
    state.animate = true;
    state.animationPhase = 0.42f;
    state.sourceLevels = { 0.86f, 0.62f, 0.40f, 0.18f, 0.52f };
    state.oscillators[0].name = "OSC 1";
    state.oscillators[0].waveName = "Custom";
    state.oscillators[0].waveType = 7;
    state.oscillators[0].level = 0.86f;
    state.oscillators[0].position = 0.28f;
    state.oscillators[0].active = true;
    state.oscillators[1].name = "OSC 2";
    state.oscillators[1].waveName = "Wavetable";
    state.oscillators[1].waveType = 4;
    state.oscillators[1].level = 0.62f;
    state.oscillators[1].position = 0.68f;
    state.oscillators[1].active = true;

    for (size_t oscillator = 0; oscillator < state.oscillators.size(); ++oscillator)
    {
        for (size_t frame = 0; frame < UI::HomeSoundStage::frameCount; ++frame)
        {
            for (size_t point = 0; point < UI::HomeSoundStage::pointCount; ++point)
            {
                const auto phase = static_cast<float>(point) / static_cast<float>(UI::HomeSoundStage::pointCount - 1);
                const auto frameShift = static_cast<float>(frame) * 0.11f;
                const auto value = std::sin(juce::MathConstants<float>::twoPi * (phase + frameShift));
                state.oscillators[oscillator].frames[frame][point] = 0.5f + value * (oscillator == 0 ? 0.42f : 0.30f);
            }
        }
    }

    stage.setState(state);
    const auto metrics = stage.getLayoutMetricsForAudit();
    if (! metrics.readable)
    {
        std::cerr << "HOME sound stage layout is not readable: "
                  << metrics.stageWidth << "x" << metrics.stageHeight << '\n';
        return 1;
    }

    juce::Image image(juce::Image::ARGB, stage.getWidth(), stage.getHeight(), true);
    juce::Graphics graphics(image);
    stage.paint(graphics);

    auto visiblePixels = 0;
    for (auto y = 0; y < image.getHeight(); y += 6)
        for (auto x = 0; x < image.getWidth(); x += 6)
            if (image.getPixelAt(x, y).getAlpha() > 0)
                ++visiblePixels;

    if (visiblePixels < 1200)
    {
        std::cerr << "HOME sound stage render appears sparse: " << visiblePixels << " sampled pixels\n";
        return 1;
    }

    auto selectedOscillator = -1;
    auto editStartCount = 0;
    auto positionChangeCount = 0;
    auto openedOscillator = -1;
    stage.onOscillatorSelected = [&selectedOscillator] (bool osc2) { selectedOscillator = osc2 ? 1 : 0; };
    stage.onPositionEditStart = [&editStartCount] (bool) { ++editStartCount; };
    stage.onPositionChange = [&positionChangeCount] (bool, float) { ++positionChangeCount; };
    stage.onOpenEditor = [&openedOscillator] (bool osc2) { openedOscillator = osc2 ? 1 : 0; };

    const auto downX = 250.0f;
    const auto osc2Y = 205.0f;
    stage.mouseDown(makeMouseEvent(stage, downX, osc2Y, downX, osc2Y));
    if (selectedOscillator != 1 || editStartCount != 0 || positionChangeCount != 0)
    {
        std::cerr << "Clicking OSC 2 should select it without changing wavetable position\n";
        return 1;
    }

    stage.mouseDrag(makeMouseEvent(stage, 390.0f, osc2Y, downX, osc2Y, true));
    stage.mouseUp(makeMouseEvent(stage, 390.0f, osc2Y, downX, osc2Y, true));
    if (editStartCount != 1 || positionChangeCount < 1)
    {
        std::cerr << "Dragging OSC 2 did not start and emit a wavetable-position edit\n";
        return 1;
    }

    stage.mouseDoubleClick(makeMouseEvent(stage, 250.0f, 80.0f, 250.0f, 80.0f, false, 2));
    if (openedOscillator != 0)
    {
        std::cerr << "Double-clicking OSC 1 did not request the detailed source editor\n";
        return 1;
    }

    std::cout << "HOME sound stage render and interaction audit passed.\n";
    return 0;
}
