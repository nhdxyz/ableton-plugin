#include "../Source/UI/WavetableDisplay.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <iostream>
#include <vector>

namespace
{
juce::MouseEvent makeMouseEvent(UI::WavetableDisplay& display,
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
                            &display,
                            &display,
                            now,
                            { downX, downY },
                            now,
                            1,
                            dragged);
}

UI::WavetableDisplay::CustomPointArray makeTestWave()
{
    UI::WavetableDisplay::CustomPointArray points {};
    for (size_t index = 0; index < points.size(); ++index)
        points[index] = static_cast<float>((index * 5) % points.size()) / static_cast<float>(points.size() - 1);

    return points;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::WavetableDisplay display;
    display.setBounds(0, 0, 420, 188);
    display.setState(0.28f,
                     0.72f,
                     true,
                     true,
                     0.35f,
                     -0.22f,
                     2,
                     "LFO 1, Tone",
                     0.42f,
                     makeTestWave(),
                     makeTestWave(),
                     true,
                     true);

    const auto metrics = display.getLayoutMetricsForAudit();
    if (! metrics.readable || ! metrics.frameStripVisible || ! metrics.partialBarsVisible)
    {
        std::cerr << "Wavetable display layout is not readable: frame="
                  << metrics.frameStrip.toString()
                  << " plot=" << metrics.plot.toString()
                  << " partials=" << metrics.partialBars.toString()
                  << " spectrum=" << metrics.spectrum.toString() << '\n';
        return 1;
    }

    if (metrics.frameStrip.getBottom() >= metrics.plot.getY()
        || metrics.plot.getBottom() >= metrics.partialBars.getY()
        || metrics.partialBars.getRight() > metrics.spectrum.getX())
    {
        std::cerr << "Wavetable display zones overlap or are misordered: frame="
                  << metrics.frameStrip.toString()
                  << " plot=" << metrics.plot.toString()
                  << " partials=" << metrics.partialBars.toString()
                  << " spectrum=" << metrics.spectrum.toString() << '\n';
        return 1;
    }

    juce::Image image(juce::Image::ARGB, display.getWidth(), display.getHeight(), true);
    juce::Graphics graphics(image);
    display.paint(graphics);

    auto visiblePixels = 0;
    for (auto y = 0; y < image.getHeight(); y += 6)
        for (auto x = 0; x < image.getWidth(); x += 6)
            if (image.getPixelAt(x, y).getAlpha() > 0
                && image.getPixelAt(x, y) != juce::Colours::transparentBlack)
                ++visiblePixels;

    if (visiblePixels < 220)
    {
        std::cerr << "Wavetable display render appears too sparse: " << visiblePixels << " sampled pixels\n";
        return 1;
    }

    std::vector<std::pair<int, size_t>> changedPoints;
    display.onCustomPointChange = [&changedPoints] (int oscillator, size_t point, float)
    {
        changedPoints.emplace_back(oscillator, point);
    };

    const auto plot = metrics.plot.toFloat();
    const auto drawX = plot.getX() + (plot.getWidth() * 0.52f);
    const auto drawY = plot.getY() + (plot.getHeight() * 0.22f);
    display.mouseDown(makeMouseEvent(display, drawX, drawY, drawX, drawY));
    display.mouseUp(makeMouseEvent(display, drawX, drawY, drawX, drawY));

    if (changedPoints.empty() || changedPoints.back().first != 1)
    {
        std::cerr << "Custom wave point editing did not emit an Osc 1 edit\n";
        return 1;
    }

    changedPoints.clear();
    const auto partial = metrics.partialBars.toFloat();
    const auto partialX = partial.getX() + (partial.getWidth() * 0.46f);
    const auto partialY = partial.getY() + (partial.getHeight() * 0.18f);
    display.mouseDown(makeMouseEvent(display, partialX, partialY, partialX, partialY));
    display.mouseUp(makeMouseEvent(display, partialX, partialY, partialX, partialY));

    if (changedPoints.size() < UI::WavetableDisplay::customPointCount / 2)
    {
        std::cerr << "Partial-bar editing did not rewrite enough custom points, changed "
                  << changedPoints.size() << '\n';
        return 1;
    }

    std::cout << "Wavetable display audit passed for frame rail, 3D surface zones, render coverage, point editing, and partial editing.\n";
    return 0;
}
