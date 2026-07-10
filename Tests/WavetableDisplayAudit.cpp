#include "../Source/UI/WavetableDisplay.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
struct FrameActionEvent
{
    int oscillator = 1;
    size_t frameIndex = 0;
    UI::WavetableDisplay::FrameAction action = UI::WavetableDisplay::FrameAction::copy;
};

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

UI::WavetableDisplay::CustomPointArray makeTestWave(float phaseOffset = 0.0f)
{
    UI::WavetableDisplay::CustomPointArray points {};
    for (size_t index = 0; index < points.size(); ++index)
    {
        const auto phase = (static_cast<float>(index) / static_cast<float>(points.size())) + phaseOffset;
        const auto sample = (std::sin(juce::MathConstants<float>::twoPi * phase) * 0.34f)
            + (std::sin(juce::MathConstants<float>::twoPi * (phase * 2.0f + 0.13f)) * 0.18f)
            + (std::sin(juce::MathConstants<float>::twoPi * (phase * 5.0f + 0.31f)) * 0.10f);
        points[index] = juce::jlimit(0.0f, 1.0f, 0.5f + sample);
    }

    return points;
}

UI::WavetableDisplay::CustomFrameSet makeTestFrames(float phaseOffset)
{
    UI::WavetableDisplay::CustomFrameSet frames {};
    for (size_t frame = 0; frame < frames.size(); ++frame)
        frames[frame] = makeTestWave(phaseOffset + (static_cast<float>(frame) * 0.047f));

    return frames;
}

juce::Point<float> compactBadgeCenter(juce::Rectangle<float> rail, size_t frameIndex, size_t badgeIndex)
{
    const auto cellWidth = rail.getWidth() / static_cast<float>(UI::WavetableDisplay::customFrameCount);
    auto cell = juce::Rectangle<float>(rail.getX() + (static_cast<float>(frameIndex) * cellWidth),
                                       rail.getY(),
                                       cellWidth,
                                       rail.getHeight()).reduced(1.3f, 0.8f);
    auto badgeRow = cell.reduced(2.0f, 2.0f).removeFromBottom(9.0f).withTrimmedLeft(11.0f);
    const auto gap = 1.0f;
    const auto badgeWidth = juce::jmax(4.0f, (badgeRow.getWidth() - (gap * 2.0f)) / 3.0f);
    const auto safeBadge = juce::jlimit<size_t>(size_t { 0 }, size_t { 2 }, badgeIndex);
    return { badgeRow.getX() + ((badgeWidth + gap) * static_cast<float>(safeBadge)) + (badgeWidth * 0.5f),
             badgeRow.getCentreY() };
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const auto osc1Frames = makeTestFrames(0.0f);
    const auto osc2Frames = makeTestFrames(0.19f);

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
                     osc1Frames[2],
                     osc2Frames[5],
                     true,
                     true,
                     osc1Frames,
                     osc2Frames);

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

    if (metrics.customFramePreviewCards != static_cast<int>(UI::WavetableDisplay::customFrameCount)
        || ! metrics.customFramePreviewsVary
        || metrics.customFrameActionBadgeCards < 2)
    {
        std::cerr << "Wavetable custom frame previews are not exposing a varied 8-frame stack: cards="
                  << metrics.customFramePreviewCards
                  << " vary=" << metrics.customFramePreviewsVary
                  << " actionBadges=" << metrics.customFrameActionBadgeCards << '\n';
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

    display.setViewMode(UI::WavetableDisplay::ViewMode::precision);
    if (! display.getTooltip().containsIgnoreCase("2D precision"))
    {
        std::cerr << "Wavetable precision mode is not reflected in the editor tooltip\n";
        return 1;
    }

    juce::Image precisionImage(juce::Image::ARGB, display.getWidth(), display.getHeight(), true);
    juce::Graphics precisionGraphics(precisionImage);
    display.paint(precisionGraphics);
    auto precisionPixels = 0;
    for (auto y = 0; y < precisionImage.getHeight(); y += 6)
        for (auto x = 0; x < precisionImage.getWidth(); x += 6)
            if (precisionImage.getPixelAt(x, y).getAlpha() > 0)
                ++precisionPixels;

    if (precisionPixels < 220)
    {
        std::cerr << "Wavetable precision render appears too sparse: " << precisionPixels << " sampled pixels\n";
        return 1;
    }
    display.setViewMode(UI::WavetableDisplay::ViewMode::perspective);

    std::vector<std::pair<int, size_t>> changedPoints;
    display.onCustomPointChange = [&changedPoints] (int oscillator, size_t point, float)
    {
        changedPoints.emplace_back(oscillator, point);
    };
    std::vector<float> osc1Positions;
    display.onOsc1PositionChange = [&osc1Positions] (float position)
    {
        osc1Positions.push_back(position);
    };
    std::vector<FrameActionEvent> frameActions;
    display.onFrameAction = [&frameActions] (int oscillator, size_t frameIndex, UI::WavetableDisplay::FrameAction action)
    {
        frameActions.push_back({ oscillator, frameIndex, action });
    };

    const auto rail = metrics.frameStrip.toFloat().withTrimmedLeft(43.0f).withTrimmedRight(5.0f).reduced(1.0f, 2.0f);
    const auto cellWidth = rail.getWidth() / static_cast<float>(UI::WavetableDisplay::customFrameCount);
    const auto frameSixX = rail.getX() + (cellWidth * 5.5f);
    const auto exactFramePosition = 5.0f / static_cast<float>(UI::WavetableDisplay::customFrameCount - 1);
    const auto railY = rail.getCentreY();
    const auto railTopClickY = rail.getY() + juce::jmin(5.0f, rail.getHeight() * 0.25f);
    const auto directActionPositionsBefore = osc1Positions.size();
    const auto directActionPointChangesBefore = changedPoints.size();
    const auto osc1CopyBadge = compactBadgeCenter(rail, 2, 0);
    display.mouseDown(makeMouseEvent(display, osc1CopyBadge.x, osc1CopyBadge.y, osc1CopyBadge.x, osc1CopyBadge.y));
    display.mouseUp(makeMouseEvent(display, osc1CopyBadge.x, osc1CopyBadge.y, osc1CopyBadge.x, osc1CopyBadge.y));

    if (frameActions.empty()
        || frameActions.back().oscillator != 1
        || frameActions.back().frameIndex != 2
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::copy)
    {
        std::cerr << "Compact C badge did not emit an Osc 1 frame copy action\n";
        return 1;
    }

    const auto osc1PasteBadge = compactBadgeCenter(rail, 2, 1);
    display.mouseDown(makeMouseEvent(display, osc1PasteBadge.x, osc1PasteBadge.y, osc1PasteBadge.x, osc1PasteBadge.y));
    display.mouseUp(makeMouseEvent(display, osc1PasteBadge.x, osc1PasteBadge.y, osc1PasteBadge.x, osc1PasteBadge.y));

    if (frameActions.empty()
        || frameActions.back().oscillator != 1
        || frameActions.back().frameIndex != 2
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::paste)
    {
        std::cerr << "Compact P badge did not emit an Osc 1 frame paste action\n";
        return 1;
    }

    const auto osc2StoreBadge = compactBadgeCenter(rail, 5, 2);
    display.mouseDown(makeMouseEvent(display, osc2StoreBadge.x, osc2StoreBadge.y, osc2StoreBadge.x, osc2StoreBadge.y));
    display.mouseUp(makeMouseEvent(display, osc2StoreBadge.x, osc2StoreBadge.y, osc2StoreBadge.x, osc2StoreBadge.y));

    if (frameActions.empty()
        || frameActions.back().oscillator != 2
        || frameActions.back().frameIndex != 5
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::storeMorph)
    {
        std::cerr << "Compact S badge did not emit an Osc 2 store-morph action\n";
        return 1;
    }

    if (osc1Positions.size() != directActionPositionsBefore || changedPoints.size() != directActionPointChangesBefore)
    {
        std::cerr << "Compact direct badge clicks also emitted position or point edits\n";
        return 1;
    }

    frameActions.clear();
    display.mouseDown(makeMouseEvent(display, frameSixX, railTopClickY, frameSixX, railTopClickY));
    display.mouseUp(makeMouseEvent(display, frameSixX, railTopClickY, frameSixX, railTopClickY));

    if (! changedPoints.empty())
    {
        std::cerr << "Exact frame-card selection emitted custom point edits\n";
        return 1;
    }

    if (! frameActions.empty())
    {
        std::cerr << "Exact frame-card selection emitted a frame-card edit action\n";
        return 1;
    }

    if (osc1Positions.empty() || std::abs(osc1Positions.back() - exactFramePosition) > 0.005f)
    {
        std::cerr << "Frame-card selection did not snap to exact frame 6 position";
        if (! osc1Positions.empty())
            std::cerr << ": " << osc1Positions.back();
        std::cerr << '\n';
        return 1;
    }

    const auto frameActionPositionsBefore = osc1Positions.size();
    const auto frameActionPointChangesBefore = changedPoints.size();
    const auto frameThreeX = rail.getX() + (cellWidth * 2.5f);
    const auto altClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::altModifier);
    display.mouseDown(makeMouseEvent(display, frameThreeX, railY, frameThreeX, railY, altClick));
    display.mouseDrag(makeMouseEvent(display,
                                     frameThreeX + (cellWidth * 0.4f),
                                     railY + 12.0f,
                                     frameThreeX,
                                     railY,
                                     altClick,
                                     true));
    display.mouseUp(makeMouseEvent(display, frameThreeX, railY, frameThreeX, railY, altClick));

    if (frameActions.empty()
        || frameActions.back().oscillator != 1
        || frameActions.back().frameIndex != 2
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::copy)
    {
        std::cerr << "Option-click on compact frame 3 did not emit an Osc 1 copy action\n";
        return 1;
    }

    const auto frameSevenX = rail.getX() + (cellWidth * 6.5f);
    const auto commandClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::commandModifier);
    display.mouseDown(makeMouseEvent(display, frameSevenX, railY, frameSevenX, railY, commandClick));
    display.mouseUp(makeMouseEvent(display, frameSevenX, railY, frameSevenX, railY, commandClick));

    if (frameActions.empty()
        || frameActions.back().oscillator != 2
        || frameActions.back().frameIndex != 6
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::paste)
    {
        std::cerr << "Command-click on compact frame 7 did not emit an Osc 2 paste action\n";
        return 1;
    }

    const auto frameTwoX = rail.getX() + (cellWidth * 1.5f);
    const auto shiftClick = juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);
    display.mouseDown(makeMouseEvent(display, frameTwoX, railY, frameTwoX, railY, shiftClick));
    display.mouseUp(makeMouseEvent(display, frameTwoX, railY, frameTwoX, railY, shiftClick));

    if (frameActions.empty()
        || frameActions.back().oscillator != 2
        || frameActions.back().frameIndex != 1
        || frameActions.back().action != UI::WavetableDisplay::FrameAction::storeMorph)
    {
        std::cerr << "Shift-click on compact frame 2 did not emit an Osc 2 store-morph action\n";
        return 1;
    }

    if (osc1Positions.size() != frameActionPositionsBefore || changedPoints.size() != frameActionPointChangesBefore)
    {
        std::cerr << "Compact frame-card edit gestures also emitted position or point edits\n";
        return 1;
    }

    osc1Positions.clear();
    const auto railStartX = rail.getX() + (rail.getWidth() * 0.82f);
    const auto railEndX = rail.getX() + (rail.getWidth() * 0.18f);
    display.mouseDown(makeMouseEvent(display, railStartX, railY, railStartX, railY));
    display.mouseDrag(makeMouseEvent(display, railEndX, railY, railStartX, railY, juce::ModifierKeys::leftButtonModifier, true));
    display.mouseUp(makeMouseEvent(display, railEndX, railY, railStartX, railY));

    if (! changedPoints.empty())
    {
        std::cerr << "Frame rail scan emitted custom point edits instead of only WT position edits\n";
        return 1;
    }

    if (osc1Positions.size() < 2
        || std::abs(osc1Positions.front() - (6.0f / static_cast<float>(UI::WavetableDisplay::customFrameCount - 1))) > 0.005f
        || std::abs(osc1Positions.back() - 0.18f) > 0.04f)
    {
        std::cerr << "Frame rail scan did not emit expected Osc 1 WT positions";
        for (const auto position : osc1Positions)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

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

    std::cout << "Wavetable display audit passed for varied custom frame previews, exact frame selection, direct frame-card actions, frame rail scanning, 3D surface zones, render coverage, point editing, and partial editing.\n";
    return 0;
}
