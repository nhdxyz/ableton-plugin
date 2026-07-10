#include "WavetableDisplay.h"

#include <array>
#include <cmath>
#include <utility>

namespace UI
{
namespace
{
constexpr WavetableDisplay::CustomPointArray defaultCustomWavePoints {
    0.5f,
    0.691342f,
    0.853553f,
    0.961940f,
    1.0f,
    0.961940f,
    0.853553f,
    0.691342f,
    0.5f,
    0.308658f,
    0.146447f,
    0.038060f,
    0.0f,
    0.038060f,
    0.146447f,
    0.308658f
};

float sineHarmonic(float phase, float harmonic, float phaseOffset = 0.0f)
{
    return std::sin(juce::MathConstants<float>::twoPi * ((phase * harmonic) + phaseOffset));
}

WavetableDisplay::CustomPointArray sanitiseCustomPoints(WavetableDisplay::CustomPointArray points)
{
    for (auto& point : points)
        point = juce::jlimit(0.0f, 1.0f, point);

    return points;
}

WavetableDisplay::CustomFrameSet sanitiseCustomFrames(WavetableDisplay::CustomFrameSet frames)
{
    for (auto& frame : frames)
        frame = sanitiseCustomPoints(frame);

    return frames;
}

bool pointsDiffer(const WavetableDisplay::CustomPointArray& lhs, const WavetableDisplay::CustomPointArray& rhs)
{
    for (size_t index = 0; index < lhs.size(); ++index)
        if (std::abs(lhs[index] - rhs[index]) >= 0.001f)
            return true;

    return false;
}

bool framesDiffer(const WavetableDisplay::CustomFrameSet& lhs, const WavetableDisplay::CustomFrameSet& rhs)
{
    for (size_t index = 0; index < lhs.size(); ++index)
        if (pointsDiffer(lhs[index], rhs[index]))
            return true;

    return false;
}

struct WaveAnalysis
{
    float rms = 0.0f;
    float dc = 0.0f;
    float peak = 0.0f;
    std::array<float, 16> partials {};
};

WaveAnalysis analysePoints(const WavetableDisplay::CustomPointArray& points)
{
    WaveAnalysis analysis;
    auto sum = 0.0f;
    auto squareSum = 0.0f;

    for (const auto point : points)
    {
        const auto sample = (juce::jlimit(0.0f, 1.0f, point) * 2.0f) - 1.0f;
        sum += sample;
        squareSum += sample * sample;
        analysis.peak = juce::jmax(analysis.peak, std::abs(sample));
    }

    analysis.dc = sum / static_cast<float>(points.size());
    analysis.rms = std::sqrt(squareSum / static_cast<float>(points.size()));

    for (size_t partial = 0; partial < analysis.partials.size(); ++partial)
    {
        auto real = 0.0f;
        auto imaginary = 0.0f;
        const auto harmonic = static_cast<float>(partial + 1);
        for (size_t index = 0; index < points.size(); ++index)
        {
            const auto phase = static_cast<float>(index) / static_cast<float>(points.size());
            const auto sample = (juce::jlimit(0.0f, 1.0f, points[index]) * 2.0f) - 1.0f;
            const auto angle = juce::MathConstants<float>::twoPi * harmonic * phase;
            real += sample * std::cos(angle);
            imaginary -= sample * std::sin(angle);
        }

        analysis.partials[partial] = juce::jlimit(0.0f,
                                                  1.0f,
                                                  std::sqrt((real * real) + (imaginary * imaginary))
                                                      / static_cast<float>(points.size()) * 2.0f);
    }

    return analysis;
}

float drawFrame(int frameIndex, float phase)
{
    switch (frameIndex)
    {
        case 0:
            return (sineHarmonic(phase, 1.0f)
                    + (sineHarmonic(phase, 2.0f) * 0.46f)
                    + (sineHarmonic(phase, 3.0f) * 0.28f)
                    + (sineHarmonic(phase, 6.0f) * 0.16f)) * 0.58f;

        case 1:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 12; ++harmonic)
                sample += sineHarmonic(phase,
                                       static_cast<float>(harmonic),
                                       0.0f) * ((harmonic % 2 == 0 ? -0.72f : 1.0f) / static_cast<float>(harmonic));
            return sample * 0.62f;
        }

        case 2:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 13; harmonic += 2)
                sample += sineHarmonic(phase, static_cast<float>(harmonic)) / static_cast<float>(harmonic);
            return sample * 0.82f;
        }

        case 3:
            return ((sineHarmonic(phase, 1.0f) * 0.82f)
                    + (sineHarmonic(phase, 2.0f, 0.08f) * 0.48f)
                    + (sineHarmonic(phase, 5.0f, 0.19f) * 0.33f)
                    + (sineHarmonic(phase, 8.0f, 0.31f) * 0.18f)) * 0.72f;

        default:
        {
            const auto edge = (phase * 2.0f) - 1.0f;
            const auto fold = std::sin(edge * juce::MathConstants<float>::pi * 1.85f);
            return (edge * 0.55f) + (fold * 0.45f);
        }
    }
}
}

WavetableDisplay::WavetableDisplay()
{
    setInterceptsMouseClicks(true, true);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void WavetableDisplay::setState(float newOsc1Position,
                                float newOsc2Position,
                                bool newOsc1Active,
                                bool newOsc2Active,
                                float newOsc1ModAmount,
                                float newOsc2ModAmount,
                                int newModRouteCount,
                                juce::String newModSourceSummary,
                                float newWarp,
                                CustomPointArray newOsc1CustomPoints,
                                CustomPointArray newOsc2CustomPoints,
                                bool newOsc1CustomActive,
                                bool newOsc2CustomActive,
                                CustomFrameSet newOsc1CustomFrames,
                                CustomFrameSet newOsc2CustomFrames)
{
    newOsc1Position = juce::jlimit(0.0f, 1.0f, newOsc1Position);
    newOsc2Position = juce::jlimit(0.0f, 1.0f, newOsc2Position);
    newOsc1ModAmount = juce::jlimit(-1.0f, 1.0f, newOsc1ModAmount);
    newOsc2ModAmount = juce::jlimit(-1.0f, 1.0f, newOsc2ModAmount);
    newModRouteCount = juce::jmax(0, newModRouteCount);
    newModSourceSummary = newModSourceSummary.trim();
    newWarp = juce::jlimit(0.0f, 1.0f, newWarp);
    newOsc1CustomPoints = sanitiseCustomPoints(newOsc1CustomPoints);
    newOsc2CustomPoints = sanitiseCustomPoints(newOsc2CustomPoints);
    newOsc1CustomFrames = sanitiseCustomFrames(newOsc1CustomFrames);
    newOsc2CustomFrames = sanitiseCustomFrames(newOsc2CustomFrames);

    if (std::abs(osc1Position - newOsc1Position) < 0.001f
        && std::abs(osc2Position - newOsc2Position) < 0.001f
        && std::abs(warp - newWarp) < 0.001f
        && std::abs(osc1ModAmount - newOsc1ModAmount) < 0.001f
        && std::abs(osc2ModAmount - newOsc2ModAmount) < 0.001f
        && modRouteCount == newModRouteCount
        && modSourceSummary == newModSourceSummary
        && osc1Active == newOsc1Active
        && osc2Active == newOsc2Active
        && osc1CustomActive == newOsc1CustomActive
        && osc2CustomActive == newOsc2CustomActive
        && ! pointsDiffer(osc1CustomPoints, newOsc1CustomPoints)
        && ! pointsDiffer(osc2CustomPoints, newOsc2CustomPoints)
        && ! framesDiffer(osc1CustomFrames, newOsc1CustomFrames)
        && ! framesDiffer(osc2CustomFrames, newOsc2CustomFrames))
    {
        return;
    }

    osc1Position = newOsc1Position;
    osc2Position = newOsc2Position;
    warp = newWarp;
    osc1ModAmount = newOsc1ModAmount;
    osc2ModAmount = newOsc2ModAmount;
    modRouteCount = newModRouteCount;
    modSourceSummary = std::move(newModSourceSummary);
    osc1Active = newOsc1Active;
    osc2Active = newOsc2Active;
    osc1CustomActive = newOsc1CustomActive;
    osc2CustomActive = newOsc2CustomActive;
    osc1CustomPoints = newOsc1CustomPoints;
    osc2CustomPoints = newOsc2CustomPoints;
    osc1CustomFrames = newOsc1CustomFrames;
    osc2CustomFrames = newOsc2CustomFrames;
    repaint();
}

juce::String WavetableDisplay::getTooltip()
{
    return juce::String(viewMode == ViewMode::perspective ? "3D wavetable editor" : "2D precision wavetable editor")
        + ": top rail selects/scans morph frames; Option-click copies, Command-click pastes, Shift-click stores morph. Shift/right-drag targets Osc 2, Option-drag adjusts warp.";
}

WavetableDisplay::LayoutMetrics WavetableDisplay::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    metrics.frameStrip = frameStripBounds().toNearestInt();
    metrics.plot = editorPlotBounds().toNearestInt();
    metrics.partialBars = partialBarsBounds().toNearestInt();
    metrics.spectrum = spectrumBounds().toNearestInt();
    metrics.frameStripVisible = ! metrics.frameStrip.isEmpty();
    metrics.partialBarsVisible = ! metrics.partialBars.isEmpty();
    const CustomFrameSet* previewFrames = nullptr;
    if (osc1CustomActive)
        previewFrames = &osc1CustomFrames;
    else if (osc2CustomActive)
        previewFrames = &osc2CustomFrames;

    if (metrics.frameStripVisible && previewFrames != nullptr)
    {
        metrics.customFramePreviewCards = static_cast<int>(previewFrames->size());
        for (size_t index = 1; index < previewFrames->size(); ++index)
        {
            if (pointsDiffer((*previewFrames)[0], (*previewFrames)[index]))
            {
                metrics.customFramePreviewsVary = true;
                break;
            }
        }

        if (osc1CustomActive)
            ++metrics.customFrameActionBadgeCards;
        if (osc2CustomActive)
            ++metrics.customFrameActionBadgeCards;
    }

    metrics.readable = metrics.frameStrip.getWidth() >= 180
        && metrics.frameStrip.getHeight() >= 14
        && metrics.plot.getWidth() >= 180
        && metrics.plot.getHeight() >= 72
        && metrics.partialBars.getWidth() >= 120
        && metrics.partialBars.getHeight() >= 18
        && metrics.spectrum.getWidth() >= 72
        && metrics.spectrum.getHeight() >= 18;
    return metrics;
}

void WavetableDisplay::setCustomDrawMode(CustomDrawMode newMode)
{
    if (customDrawMode == newMode)
        return;

    customDrawMode = newMode;
    lastDrawCustomPoint = -1;
    repaint();
}

void WavetableDisplay::setViewMode(ViewMode newMode)
{
    if (viewMode == newMode)
        return;

    viewMode = newMode;
    repaint();
}

void WavetableDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto visualArea = bounds.reduced(8.0f, 8.0f);
    const auto showFrameStrip = visualArea.getHeight() >= 118.0f && visualArea.getWidth() >= 180.0f;
    auto frameStrip = juce::Rectangle<float>();
    if (showFrameStrip)
    {
        frameStrip = visualArea.removeFromTop(20.0f);
        visualArea.removeFromTop(4.0f);
    }

    const auto showAnalysis = visualArea.getHeight() >= 112.0f;
    auto analysisArea = juce::Rectangle<float>();
    if (showAnalysis)
    {
        analysisArea = visualArea.removeFromBottom(28.0f);
        visualArea.removeFromBottom(4.0f);
    }
    const auto plot = visualArea;
    const auto inactive = ! osc1Active && ! osc2Active && ! osc1CustomActive && ! osc2CustomActive;

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(inactive ? juce::Colour(0xff27343a) : juce::Colour(0xff40535a));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(inactive ? 0.35f : 0.86f));
    g.drawFittedText(viewMode == ViewMode::perspective ? "3D WT SURFACE" : "2D WT PRECISION",
                     bounds.withTrimmedLeft(8.0f).withTrimmedTop(5.0f).withHeight(12.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.70f);
    g.setColour(juce::Colour(0xff9fb0b6).withAlpha(inactive ? 0.28f : 0.78f));
    g.drawFittedText("DRAW  PARTIALS  MORPH",
                     bounds.withTrimmedRight(8.0f).withTrimmedTop(5.0f).withHeight(12.0f).toNearestInt(),
                     juce::Justification::centredRight,
                     1,
                     0.70f);

    if (showFrameStrip)
    {
        g.setColour(juce::Colour(0xff0d1316).withAlpha(0.96f));
        g.fillRoundedRectangle(frameStrip, 4.0f);
        g.setColour(juce::Colour(0xff2a3840));
        g.drawRoundedRectangle(frameStrip, 4.0f, 1.0f);

        g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9fb0b6).withAlpha(0.76f));
        g.drawFittedText("FRAMES",
                         frameStrip.withWidth(42.0f).toNearestInt(),
                         juce::Justification::centred,
                         1,
                         0.62f);

        auto rail = frameRailBounds();
        if (rail.isEmpty())
            rail = frameStrip.withTrimmedLeft(43.0f).withTrimmedRight(5.0f).reduced(1.0f, 2.0f);
        const auto cellWidth = rail.getWidth() / static_cast<float>(customFrameCount);
        const CustomFrameSet* railCustomFrames = nullptr;
        if (editingOscillator == 2 && osc2CustomActive)
            railCustomFrames = &osc2CustomFrames;
        else if (osc1CustomActive)
            railCustomFrames = &osc1CustomFrames;
        else if (osc2CustomActive)
            railCustomFrames = &osc2CustomFrames;

        for (size_t frame = 0; frame < customFrameCount; ++frame)
        {
            auto cell = juce::Rectangle<float>(rail.getX() + (static_cast<float>(frame) * cellWidth),
                                               rail.getY(),
                                               cellWidth,
                                               rail.getHeight()).reduced(1.3f, 0.8f);
            const auto position = static_cast<float>(frame) / static_cast<float>(customFrameCount - 1);
            const auto isNearO1 = osc1Active && std::abs(position - osc1Position) <= (0.52f / static_cast<float>(customFrameCount));
            const auto isNearO2 = osc2Active && std::abs(position - osc2Position) <= (0.52f / static_cast<float>(customFrameCount));
            const auto* railCustomPoints = railCustomFrames != nullptr ? &(*railCustomFrames)[frame] : nullptr;
            g.setColour((isNearO1 ? juce::Colour(0xff183b34) : juce::Colour(0xff111a1e)).withAlpha(isNearO2 ? 0.98f : 0.88f));
            g.fillRoundedRectangle(cell, 2.5f);
            g.setColour((isNearO2 ? juce::Colour(0xffc4a2ff) : juce::Colour(0xff33444c)).withAlpha(isNearO1 ? 0.82f : 0.58f));
            g.drawRoundedRectangle(cell, 2.5f, 0.8f);
            drawMiniWave(g,
                         cell.reduced(3.0f, 2.0f),
                         position,
                         railCustomPoints,
                         isNearO2 && ! isNearO1 ? juce::Colour(0xffc4a2ff) : juce::Colour(0xff8ee6c9),
                         inactive ? 0.16f : (isNearO1 || isNearO2 ? 0.92f : 0.42f));

            g.setColour((isNearO1 || isNearO2 ? juce::Colour(0xffedf7f4) : juce::Colour(0xff9fb0b6))
                            .withAlpha(inactive ? 0.20f : 0.72f));
            g.setFont(juce::FontOptions(6.4f, juce::Font::bold));
            g.drawFittedText(juce::String(static_cast<int>(frame + 1)),
                             cell.reduced(2.0f, 1.0f).removeFromLeft(9.0f).toNearestInt(),
                             juce::Justification::centred,
                             1,
                             0.62f);

            if ((isNearO1 || isNearO2) && cell.getWidth() >= 32.0f && cell.getHeight() >= 12.0f)
            {
                const auto badges = actionBadgeBoundsForRailCell(cell);
                const std::array<juce::String, 3> labels { "C", "P", "S" };

                for (size_t badgeIndex = 0; badgeIndex < labels.size(); ++badgeIndex)
                {
                    const auto badge = badges[badgeIndex];
                    g.setColour(juce::Colour(0xff0d1316).withAlpha(0.76f));
                    g.fillRoundedRectangle(badge, 1.6f);
                    g.setColour((isNearO2 && ! isNearO1 ? juce::Colour(0xffc4a2ff) : juce::Colour(0xff8ee6c9)).withAlpha(0.70f));
                    g.drawRoundedRectangle(badge, 1.6f, 0.6f);
                    g.setFont(juce::FontOptions(5.5f, juce::Font::bold));
                    g.setColour(juce::Colour(0xffedf7f4).withAlpha(0.86f));
                    g.drawFittedText(labels[badgeIndex], badge.toNearestInt(), juce::Justification::centred, 1, 0.56f);
                }
            }
        }

        const auto drawPositionPin = [&] (float position, juce::Colour colour, const char* label, float yOffset)
        {
            const auto x = rail.getX() + (rail.getWidth() * juce::jlimit(0.0f, 1.0f, position));
            g.setColour(colour.withAlpha(0.16f));
            g.drawLine(x, rail.getY(), x, rail.getBottom(), 2.6f);
            g.setColour(colour);
            g.fillEllipse(x - 3.5f, rail.getY() + yOffset, 7.0f, 7.0f);
            g.setColour(juce::Colour(0xff101619));
            g.setFont(juce::FontOptions(5.8f, juce::Font::bold));
            g.drawFittedText(label,
                             juce::Rectangle<float>(x - 4.0f, rail.getY() + yOffset - 0.5f, 8.0f, 8.0f).toNearestInt(),
                             juce::Justification::centred,
                             1,
                             0.62f);
        };

        if (osc1Active)
            drawPositionPin(osc1Position, juce::Colour(0xff8ee6c9), "1", 1.0f);
        if (osc2Active)
            drawPositionPin(osc2Position, juce::Colour(0xffc4a2ff), "2", 8.0f);
    }

    g.setColour(juce::Colour(0x223d4a50));
    for (auto line = 1; line < 4; ++line)
    {
        const auto y = plot.getY() + (plot.getHeight() * static_cast<float>(line) / 4.0f);
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
    }

    g.setColour(juce::Colour(0x335b6e75));
    g.drawLine(plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), 1.0f);

    const auto drawSurface = [&] (bool customActive,
                                  const CustomFrameSet& customFrames,
                                  juce::Colour colour)
    {
        if (plot.getWidth() < 150.0f || plot.getHeight() < 74.0f)
            return;

        const auto surfaceBounds = plot.reduced(12.0f, 13.0f);
        const auto sliceCount = viewMode == ViewMode::perspective ? 9 : 1;
        for (auto slice = 0; slice < sliceCount; ++slice)
        {
            const auto depth = viewMode == ViewMode::perspective
                ? static_cast<float>(slice) / static_cast<float>(sliceCount - 1)
                : (editingOscillator == 2 ? osc2Position : osc1Position);
            const auto xSkew = viewMode == ViewMode::perspective ? juce::jmap(depth, -12.0f, 14.0f) : 0.0f;
            const auto ySkew = viewMode == ViewMode::perspective ? juce::jmap(depth, 17.0f, -15.0f) : 0.0f;
            const auto alpha = viewMode == ViewMode::perspective ? juce::jmap(depth, 0.06f, 0.30f) : 0.34f;
            juce::Path path;
            constexpr auto surfacePoints = 72;

            for (auto index = 0; index < surfacePoints; ++index)
            {
                const auto phase = static_cast<float>(index) / static_cast<float>(surfacePoints - 1);
                auto sample = 0.0f;

                if (customActive)
                {
                    sample = sampleCustomFrameSet(customFrames, phase, depth);
                    sample *= 0.72f + (depth * 0.18f);
                }
                else
                {
                    sample = sampleFrame(phase, juce::jlimit(0.0f, 1.0f, depth)) * 0.80f;
                }

                const auto x = surfaceBounds.getX() + (surfaceBounds.getWidth() * phase) + xSkew;
                const auto y = surfaceBounds.getCentreY() - (sample * surfaceBounds.getHeight() * 0.30f) + ySkew;

                if (index == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }

            g.setColour(colour.withAlpha(alpha));
            g.strokePath(path, juce::PathStrokeType(slice == sliceCount - 1 ? 1.45f : 0.85f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        g.setColour(colour.withAlpha(0.10f));
        for (auto rib = 0; rib < (viewMode == ViewMode::perspective ? 5 : 0); ++rib)
        {
            const auto phase = static_cast<float>(rib) / 4.0f;
            const auto x = surfaceBounds.getX() + (surfaceBounds.getWidth() * phase);
            g.drawLine(x - 12.0f, surfaceBounds.getCentreY() + 17.0f,
                       x + 14.0f, surfaceBounds.getCentreY() - 15.0f,
                       0.7f);
        }
    };

    if (osc2Active)
        drawSurface(osc2CustomActive, osc2CustomFrames, juce::Colour(0xffc4a2ff));

    if (osc1Active)
        drawSurface(osc1CustomActive, osc1CustomFrames, juce::Colour(0xff8ee6c9));

    if (! inactive)
    {
        const auto warpY = juce::jmap(warp, 0.0f, 1.0f, plot.getBottom(), plot.getY());
        g.setColour(juce::Colour(0xffffc36b).withAlpha(0.24f));
        g.drawHorizontalLine(static_cast<int>(std::round(warpY)), plot.getX(), plot.getRight());
    }

    const auto drawModRange = [&] (float position, float amount, juce::Colour colour)
    {
        if (std::abs(amount) < 0.01f)
            return;

        const auto lowPosition = juce::jlimit(0.0f, 1.0f, position + (juce::jmin(0.0f, amount) * 0.36f));
        const auto highPosition = juce::jlimit(0.0f, 1.0f, position + (juce::jmax(0.0f, amount) * 0.36f));
        const auto lowX = plot.getX() + (plot.getWidth() * lowPosition);
        const auto highX = plot.getX() + (plot.getWidth() * highPosition);
        const auto leftX = juce::jmin(lowX, highX);
        const auto width = juce::jmax(2.0f, std::abs(highX - lowX));

        g.setColour(colour.withAlpha(0.12f));
        g.fillRoundedRectangle(juce::Rectangle<float>(leftX, plot.getY() + 1.0f, width, plot.getHeight() - 2.0f), 3.0f);
        g.setColour(colour.withAlpha(0.48f));
        g.strokePath(makePath(plot, lowPosition), juce::PathStrokeType(0.9f));
        g.strokePath(makePath(plot, highPosition), juce::PathStrokeType(0.9f));
    };

    if (osc2Active)
        drawModRange(osc2Position, osc2ModAmount, juce::Colour(0xffc4a2ff));

    if (osc1Active)
        drawModRange(osc1Position, osc1ModAmount, juce::Colour(0xff8ee6c9));

    const auto drawCustomPath = [&] (const CustomPointArray& points,
                                     juce::Colour colour,
                                     float thickness,
                                     bool showHandles)
    {
        g.setColour(colour);
        g.strokePath(makeCustomPath(plot, points), juce::PathStrokeType(thickness));

        if (! showHandles)
            return;

        for (size_t index = 0; index < points.size(); ++index)
        {
            const auto phase = static_cast<float>(index) / static_cast<float>(points.size());
            const auto x = plot.getX() + (plot.getWidth() * phase);
            const auto y = plot.getBottom() - (plot.getHeight() * points[index]);
            const auto handleSize = static_cast<int>(index) == editingCustomPoint ? 9.2f : 7.0f;
            const auto handle = juce::Rectangle<float>(x - (handleSize * 0.5f),
                                                       y - (handleSize * 0.5f),
                                                       handleSize,
                                                       handleSize);
            g.setColour(colour.withAlpha(0.18f));
            g.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
            g.setColour(juce::Colour(0xff101619));
            g.fillEllipse(handle.expanded(1.4f));
            g.setColour(static_cast<int>(index) == editingCustomPoint ? juce::Colour(0xffedf7f4) : colour.withAlpha(0.70f));
            g.drawEllipse(handle.expanded(3.0f), 1.0f);
            g.setColour(colour);
            g.fillEllipse(handle);
        }
    };

    const auto drawPath = [&] (float position, juce::Colour colour, float thickness)
    {
        g.setColour(colour);
        g.strokePath(makePath(plot, position), juce::PathStrokeType(thickness));
        const auto markerX = plot.getX() + (plot.getWidth() * juce::jlimit(0.0f, 1.0f, position));
        g.drawLine(markerX, plot.getY(), markerX, plot.getBottom(), 1.0f);
    };

    if (osc2Active)
    {
        if (osc2CustomActive)
            drawCustomPath(osc2CustomPoints, juce::Colour(0x99c4a2ff), 1.2f, editingOscillator == 2);
        else
            drawPath(osc2Position, juce::Colour(0x99c4a2ff), 1.2f);
    }

    if (osc1Active)
    {
        if (osc1CustomActive)
            drawCustomPath(osc1CustomPoints, juce::Colour(0xff8ee6c9), 1.6f, editingOscillator != 2);
        else
            drawPath(osc1Position, juce::Colour(0xff8ee6c9), 1.6f);
    }

    if (showAnalysis && ! inactive)
    {
        CustomPointArray analysisPoints {};
        const auto targetCustom = (editingOscillator == 2 && osc2CustomActive) || (! osc1CustomActive && osc2CustomActive);
        if (targetCustom)
        {
            analysisPoints = osc2CustomPoints;
        }
        else if (osc1CustomActive)
        {
            analysisPoints = osc1CustomPoints;
        }
        else
        {
            const auto position = osc1Active ? osc1Position : osc2Position;
            for (size_t index = 0; index < analysisPoints.size(); ++index)
            {
                const auto phase = static_cast<float>(index) / static_cast<float>(analysisPoints.size());
                analysisPoints[index] = juce::jlimit(0.0f, 1.0f, 0.5f + (sampleFrame(phase, position) * 0.5f));
            }
        }

        const auto analysis = analysePoints(analysisPoints);
        auto barsArea = analysisArea.removeFromLeft(analysisArea.getWidth() * 0.58f).reduced(1.0f, 2.0f);
        const auto barWidth = barsArea.getWidth() / static_cast<float>(analysis.partials.size());
        g.setColour(juce::Colour(0xff11191d).withAlpha(0.92f));
        g.fillRoundedRectangle(barsArea.expanded(1.0f), 3.0f);
        g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9fb0b6).withAlpha(0.74f));
        g.drawFittedText("PARTIALS",
                         barsArea.withTrimmedLeft(3.0f).withHeight(8.0f).toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.62f);
        for (size_t index = 0; index < analysis.partials.size(); ++index)
        {
            auto cell = juce::Rectangle<float>(barsArea.getX() + (static_cast<float>(index) * barWidth),
                                               barsArea.getY(),
                                               barWidth,
                                               barsArea.getHeight()).reduced(1.5f, 1.0f);
            const auto value = analysis.partials[index];
            auto bar = cell.withY(cell.getBottom() - (cell.getHeight() * value))
                           .withHeight(cell.getHeight() * value);
            g.setColour((index == 0 ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff7bb7ff)).withAlpha(0.28f));
            g.fillRoundedRectangle(cell, 2.0f);
            g.setColour(index == 0 ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff7bb7ff));
            g.fillRoundedRectangle(bar, 2.0f);
            if ((index % 4) == 0 && barsArea.getHeight() >= 18.0f)
            {
                g.setColour(juce::Colour(0xff101619).withAlpha(0.75f));
                g.setFont(juce::FontOptions(6.5f, juce::Font::bold));
                g.drawFittedText(juce::String(static_cast<int>(index + 1)),
                                 cell.withHeight(8.0f).toNearestInt(),
                                 juce::Justification::centred,
                                 1,
                                 0.65f);
            }
        }

        auto spectrumArea = analysisArea.reduced(1.0f, 2.0f);
        g.setColour(juce::Colour(0xff11191d).withAlpha(0.92f));
        g.fillRoundedRectangle(spectrumArea.expanded(1.0f), 3.0f);
        g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9fb0b6).withAlpha(0.74f));
        g.drawFittedText("SPECTRAL TILT",
                         spectrumArea.withTrimmedLeft(3.0f).withHeight(8.0f).toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.62f);

        auto heatArea = spectrumArea.withTrimmedTop(8.0f).reduced(2.0f, 1.0f);
        const auto heatWidth = heatArea.getWidth() / static_cast<float>(analysis.partials.size());
        for (size_t index = 0; index < analysis.partials.size(); ++index)
        {
            const auto value = analysis.partials[index];
            const auto normalizedIndex = static_cast<float>(index) / static_cast<float>(analysis.partials.size() - 1);
            const auto hue = 0.47f + (normalizedIndex * 0.14f);
            const auto cell = juce::Rectangle<float>(heatArea.getX() + (static_cast<float>(index) * heatWidth),
                                                     heatArea.getY(),
                                                     heatWidth,
                                                     heatArea.getHeight()).reduced(0.5f, 0.5f);
            g.setColour(juce::Colour::fromHSV(hue, 0.48f, 0.28f + (value * 0.70f), 0.32f + (value * 0.60f)));
            g.fillRoundedRectangle(cell, 1.5f);
        }

        g.setColour(juce::Colour(0xff9fb0b6));
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        const auto stats = "RMS " + juce::String(juce::roundToInt(analysis.rms * 100.0f)) + "%"
            + "  DC " + (analysis.dc >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(analysis.dc * 100.0f)) + "%"
            + "  PK " + juce::String(juce::roundToInt(analysis.peak * 100.0f)) + "%";
        g.drawFittedText(stats, spectrumArea.toNearestInt().reduced(4, 2), juce::Justification::centredBottom, 1, 0.72f);
    }

    if (inactive)
    {
        g.setColour(juce::Colour(0xff617078));
        g.setFont(juce::FontOptions(10.0f));
        g.drawFittedText("WT IDLE", getLocalBounds().reduced(8), juce::Justification::centred, 1);
    }
    else
    {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        const auto labelBounds = getLocalBounds().removeFromTop(16).reduced(8, 1);
        g.setColour(juce::Colour(0xff8ee6c9));
        if (osc1Active)
            g.drawText(osc1CustomActive ? "O1 EDIT"
                                        : "O1 " + juce::String(juce::roundToInt(osc1Position * 100.0f)) + "%",
                       labelBounds,
                       juce::Justification::centredLeft);

        if (osc2Active)
        {
            g.setColour(juce::Colour(0xffc4a2ff));
            g.drawText(osc2CustomActive ? "O2 EDIT"
                                        : "O2 " + juce::String(juce::roundToInt(osc2Position * 100.0f)) + "%",
                       labelBounds,
                       juce::Justification::centredRight);
        }

        if (modRouteCount > 0)
        {
            auto modBadge = getLocalBounds().withSizeKeepingCentre(102, 14).withY(2);
            g.setColour(juce::Colour(0xff11191d).withAlpha(0.92f));
            g.fillRoundedRectangle(modBadge.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.72f));
            g.drawRoundedRectangle(modBadge.toFloat(), 3.0f, 1.0f);
            g.setColour(juce::Colour(0xffd7e3e6));
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            const auto mainAmount = std::abs(osc1ModAmount) >= std::abs(osc2ModAmount) ? osc1ModAmount : osc2ModAmount;
            g.drawFittedText("WT MOD " + modulationText(mainAmount),
                             modBadge.reduced(4, 0),
                             juce::Justification::centred,
                             1,
                             0.72f);

            auto detail = getLocalBounds().reduced(8, 2).removeFromBottom(11);
            const auto sourceText = modSourceSummary.isNotEmpty() ? modSourceSummary : "Routes";
            g.setColour(juce::Colour(0xff8a989e));
            g.setFont(juce::FontOptions(8.0f));
            g.drawFittedText("O1 " + modulationText(osc1ModAmount)
                                 + "  O2 " + modulationText(osc2ModAmount)
                                 + "  " + sourceText,
                             detail,
                             juce::Justification::centredLeft,
                             1,
                             0.72f);
        }
        else
        {
            auto detail = getLocalBounds().reduced(8, 2).removeFromBottom(11);
            g.setColour(juce::Colour(0xff8a989e));
            g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
            g.drawFittedText("WRP " + juce::String(juce::roundToInt(warp * 100.0f)) + "%",
                             detail,
                             juce::Justification::centredRight,
                             1,
                             0.72f);
        }
    }
}

void WavetableDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (const auto partialIndex = partialForEvent(event); partialIndex >= 0)
    {
        editingOscillator = oscillatorForEvent(event);
        editingPartial = partialIndex;
        editingCustomPoint = -1;
        editGestureActive = true;
        lastDrawCustomPoint = -1;
        if (onEditStart)
            onEditStart();
        editPartial(event, partialIndex);
        return;
    }

    if (frameRailBounds().contains(event.position))
    {
        beginFrameRailEdit(event);
        return;
    }

    beginEdit(event);
    applyMousePosition(event);
}

void WavetableDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (frameActionGestureActive)
        return;

    if (editingFrameRail)
    {
        applyFrameRailPosition(event);
        return;
    }

    if (editingPartial >= 0)
    {
        editPartial(event, editingPartial);
        return;
    }

    if (! editGestureActive)
        beginEdit(event);

    applyMousePosition(event);
}

void WavetableDisplay::mouseUp(const juce::MouseEvent&)
{
    editingCustomPoint = -1;
    editingPartial = -1;
    editingFrameRail = false;
    frameActionGestureActive = false;
    lastDrawCustomPoint = -1;
    editGestureActive = false;
}

void WavetableDisplay::mouseDoubleClick(const juce::MouseEvent& event)
{
    beginEdit(event);
    if (editingCustomPoint >= 0)
    {
        const auto target = oscillatorForEvent(event);
        setCustomPointValue(target,
                            static_cast<size_t>(editingCustomPoint),
                            defaultCustomWavePoints[static_cast<size_t>(editingCustomPoint)]);

        editingCustomPoint = -1;
        editGestureActive = false;
        repaint();
        return;
    }

    const auto target = oscillatorForEvent(event);
    if (target == 2 && onOsc2PositionChange)
        onOsc2PositionChange(0.5f);
    else if (onOsc1PositionChange)
        onOsc1PositionChange(0.5f);

    editGestureActive = false;
}

void WavetableDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const auto delta = (wheel.deltaY >= 0.0f ? 0.025f : -0.025f)
        * (event.mods.isShiftDown() || event.mods.isCommandDown() ? 0.35f : 1.0f);
    nudgePosition(event, delta);
}

void WavetableDisplay::beginFrameRailEdit(const juce::MouseEvent& event)
{
    editingOscillator = oscillatorForEvent(event);
    editingCustomPoint = -1;
    editingPartial = -1;
    editingFrameRail = true;
    editGestureActive = true;
    lastDrawCustomPoint = -1;

    const auto plainBadgeClick = ! event.mods.isAltDown()
        && ! event.mods.isCommandDown()
        && ! event.mods.isShiftDown()
        && ! event.mods.isCtrlDown();
    const auto actionHit = plainBadgeClick ? frameActionHitForEvent(event) : FrameActionHit {};
    if (actionHit.valid && onFrameAction != nullptr)
    {
        editingOscillator = actionHit.oscillator;
        onFrameAction(actionHit.oscillator, actionHit.frameIndex, actionHit.action);
        editingFrameRail = false;
        editGestureActive = false;
        frameActionGestureActive = true;
        repaint();
        return;
    }

    const auto frameIndex = frameForRailEvent(event);
    if (frameIndex >= 0 && handleFrameActionGesture(static_cast<size_t>(frameIndex), event))
    {
        editingFrameRail = false;
        editGestureActive = false;
        frameActionGestureActive = true;
        return;
    }

    if (onEditStart)
        onEditStart();

    if (frameIndex >= 0)
        applyFrameRailFrame(editingOscillator, static_cast<size_t>(frameIndex));
    else
        applyFrameRailPosition(event);
}

WavetableDisplay::FrameActionHit WavetableDisplay::frameActionHitForEvent(const juce::MouseEvent& event) const
{
    FrameActionHit hit;
    const auto rail = frameRailBounds();
    if (rail.isEmpty() || ! rail.contains(event.position))
        return hit;

    const auto cellWidth = rail.getWidth() / static_cast<float>(customFrameCount);
    for (size_t frameIndex = 0; frameIndex < customFrameCount; ++frameIndex)
    {
        const auto position = static_cast<float>(frameIndex) / static_cast<float>(customFrameCount - 1);
        const auto nearO1 = osc1Active && std::abs(position - osc1Position) <= (0.52f / static_cast<float>(customFrameCount));
        const auto nearO2 = osc2Active && std::abs(position - osc2Position) <= (0.52f / static_cast<float>(customFrameCount));
        if (! nearO1 && ! nearO2)
            continue;

        auto cell = juce::Rectangle<float>(rail.getX() + (static_cast<float>(frameIndex) * cellWidth),
                                           rail.getY(),
                                           cellWidth,
                                           rail.getHeight()).reduced(1.3f, 0.8f);
        if (! cell.contains(event.position))
            continue;

        const auto badges = actionBadgeBoundsForRailCell(cell);
        for (size_t badgeIndex = 0; badgeIndex < badges.size(); ++badgeIndex)
        {
            if (! badges[badgeIndex].contains(event.position))
                continue;

            hit.valid = true;
            hit.oscillator = nearO2 && ! nearO1 ? 2 : 1;
            hit.frameIndex = frameIndex;
            hit.action = actionForBadgeIndex(badgeIndex);
            return hit;
        }
    }

    return hit;
}

bool WavetableDisplay::handleFrameActionGesture(size_t frameIndex, const juce::MouseEvent& event)
{
    if (onFrameAction == nullptr)
        return false;

    auto action = FrameAction::copy;
    if (event.mods.isCommandDown())
        action = FrameAction::paste;
    else if (event.mods.isAltDown())
        action = FrameAction::copy;
    else if (event.mods.isShiftDown())
        action = FrameAction::storeMorph;
    else
        return false;

    onFrameAction(editingOscillator,
                  juce::jlimit<size_t>(0, customFrameCount - 1, frameIndex),
                  action);
    repaint();
    return true;
}

WavetableDisplay::FrameAction WavetableDisplay::actionForBadgeIndex(size_t badgeIndex) noexcept
{
    switch (badgeIndex)
    {
        case 1: return FrameAction::paste;
        case 2: return FrameAction::storeMorph;
        case 0:
        default: return FrameAction::copy;
    }
}

void WavetableDisplay::applyFrameRailFrame(int oscillator, size_t frameIndex)
{
    const auto safeFrame = juce::jlimit<size_t>(0, customFrameCount - 1, frameIndex);
    const auto position = static_cast<float>(safeFrame) / static_cast<float>(customFrameCount - 1);

    if (oscillator == 2)
    {
        osc2Position = position;
        if (onOsc2PositionChange)
            onOsc2PositionChange(position);
    }
    else
    {
        osc1Position = position;
        if (onOsc1PositionChange)
            onOsc1PositionChange(position);
    }

    repaint();
}

void WavetableDisplay::applyFrameRailPosition(const juce::MouseEvent& event)
{
    const auto rail = frameRailBounds();
    if (rail.isEmpty())
        return;

    const auto position = juce::jlimit(0.0f,
                                      1.0f,
                                      (event.position.x - rail.getX()) / juce::jmax(1.0f, rail.getWidth()));
    if (editingOscillator == 2)
    {
        osc2Position = position;
        if (onOsc2PositionChange)
            onOsc2PositionChange(position);
    }
    else
    {
        osc1Position = position;
        if (onOsc1PositionChange)
            onOsc1PositionChange(position);
    }

    repaint();
}

void WavetableDisplay::beginEdit(const juce::MouseEvent& event)
{
    editingOscillator = oscillatorForEvent(event);
    editingCustomPoint = customPointForEvent(event);
    if (! editGestureActive)
    {
        editGestureActive = true;
        lastDrawCustomPoint = -1;
        if (onEditStart)
            onEditStart();
    }
}

void WavetableDisplay::applyMousePosition(const juce::MouseEvent& event)
{
    const auto plot = editorPlotBounds();
    if (plot.isEmpty())
        return;

    if (! event.mods.isAltDown()
        && ((editingOscillator == 2 && osc2CustomActive) || (editingOscillator != 2 && osc1CustomActive)))
    {
        editingCustomPoint = customPointForEvent(event);
        editCustomPoint(event, editingCustomPoint);
        return;
    }

    if (event.mods.isAltDown())
    {
        const auto nextWarp = juce::jlimit(0.0f, 1.0f, 1.0f - ((event.position.y - plot.getY()) / juce::jmax(1.0f, plot.getHeight())));
        warp = nextWarp;
        if (onWarpChange)
            onWarpChange(nextWarp);
        repaint();
        return;
    }

    const auto position = juce::jlimit(0.0f, 1.0f, (event.position.x - plot.getX()) / juce::jmax(1.0f, plot.getWidth()));
    if (editingOscillator == 2)
    {
        osc2Position = position;
        if (onOsc2PositionChange)
            onOsc2PositionChange(position);
    }
    else
    {
        osc1Position = position;
        if (onOsc1PositionChange)
            onOsc1PositionChange(position);
    }

    repaint();
}

void WavetableDisplay::nudgePosition(const juce::MouseEvent& event, float delta)
{
    beginEdit(event);
    if (event.mods.isAltDown())
    {
        const auto nextWarp = juce::jlimit(0.0f, 1.0f, warp + delta);
        warp = nextWarp;
        if (onWarpChange)
            onWarpChange(nextWarp);
        repaint();
        return;
    }

    if ((editingOscillator == 2 && osc2CustomActive) || (editingOscillator != 2 && osc1CustomActive))
    {
        const auto plot = editorPlotBounds();
        const auto position = juce::jlimit(0.0f, 1.0f,
                                           ((event.position.x - plot.getX()) / juce::jmax(1.0f, plot.getWidth())) + delta);
        editingCustomPoint = juce::jlimit(0,
                                          static_cast<int>(defaultCustomWavePoints.size()) - 1,
                                          static_cast<int>(std::round(position * static_cast<float>(defaultCustomWavePoints.size() - 1))));
        editingCustomPoint = editingCustomPoint % static_cast<int>(defaultCustomWavePoints.size());
        editCustomPoint(event, editingCustomPoint);
        editGestureActive = false;
        return;
    }

    if (editingOscillator == 2)
    {
        const auto nextPosition = juce::jlimit(0.0f, 1.0f, osc2Position + delta);
        osc2Position = nextPosition;
        if (onOsc2PositionChange)
            onOsc2PositionChange(nextPosition);
    }
    else
    {
        const auto nextPosition = juce::jlimit(0.0f, 1.0f, osc1Position + delta);
        osc1Position = nextPosition;
        if (onOsc1PositionChange)
            onOsc1PositionChange(nextPosition);
    }

    editGestureActive = false;
    repaint();
}

int WavetableDisplay::oscillatorForEvent(const juce::MouseEvent& event) const noexcept
{
    if (! osc1Active && osc2Active)
        return 2;

    if (event.mods.isRightButtonDown() || event.mods.isShiftDown() || event.mods.isCommandDown())
        return 2;

    return 1;
}

int WavetableDisplay::customPointForEvent(const juce::MouseEvent& event) const noexcept
{
    const auto target = oscillatorForEvent(event);
    if ((target == 2 && ! osc2CustomActive) || (target != 2 && ! osc1CustomActive))
        return -1;

    const auto plot = editorPlotBounds();
    if (plot.isEmpty())
        return -1;

    const auto normalisedX = juce::jlimit(0.0f, 0.999f,
                                          (event.position.x - plot.getX()) / juce::jmax(1.0f, plot.getWidth()));
    return juce::jlimit(0,
                        static_cast<int>(defaultCustomWavePoints.size()) - 1,
                        static_cast<int>(std::floor(normalisedX * static_cast<float>(defaultCustomWavePoints.size()))));
}

void WavetableDisplay::editCustomPoint(const juce::MouseEvent& event, int pointIndex)
{
    pointIndex = juce::jlimit(0, static_cast<int>(defaultCustomWavePoints.size()) - 1, pointIndex);

    const auto plot = editorPlotBounds();
    const auto value = juce::jlimit(0.0f, 1.0f,
                                    1.0f - ((event.position.y - plot.getY()) / juce::jmax(1.0f, plot.getHeight())));
    const auto point = static_cast<size_t>(pointIndex);
    const auto target = editingOscillator == 2 ? 2 : 1;

    if (customDrawMode == CustomDrawMode::line && lastDrawCustomPoint >= 0 && lastDrawCustomPoint != pointIndex)
    {
        const auto start = juce::jmin(lastDrawCustomPoint, pointIndex);
        const auto end = juce::jmax(lastDrawCustomPoint, pointIndex);
        const auto span = juce::jmax(1, end - start);
        for (auto index = start; index <= end; ++index)
        {
            const auto normalised = static_cast<float>(index - start) / static_cast<float>(span);
            const auto drawValue = lastDrawCustomPoint < pointIndex
                ? lastDrawCustomValue + ((value - lastDrawCustomValue) * normalised)
                : value + ((lastDrawCustomValue - value) * normalised);
            setCustomPointValue(target, static_cast<size_t>(index), drawValue);
        }
    }
    else if (customDrawMode == CustomDrawMode::smooth)
    {
        setCustomPointValue(target, point, value);
        if (point > 0)
            setCustomPointValue(target, point - 1, (value * 0.35f) + ((editingOscillator == 2 ? osc2CustomPoints[point - 1] : osc1CustomPoints[point - 1]) * 0.65f));
        if (point + 1 < defaultCustomWavePoints.size())
            setCustomPointValue(target, point + 1, (value * 0.35f) + ((editingOscillator == 2 ? osc2CustomPoints[point + 1] : osc1CustomPoints[point + 1]) * 0.65f));
    }
    else if (customDrawMode == CustomDrawMode::step)
    {
        setCustomPointValue(target, point, value);
        if (point + 1 < defaultCustomWavePoints.size())
            setCustomPointValue(target, point + 1, value);
    }
    else if (customDrawMode == CustomDrawMode::erase)
    {
        setCustomPointValue(target, point, 0.5f);
        if (point > 0)
            setCustomPointValue(target, point - 1, (editingOscillator == 2 ? osc2CustomPoints[point - 1] : osc1CustomPoints[point - 1]) * 0.65f + 0.175f);
        if (point + 1 < defaultCustomWavePoints.size())
            setCustomPointValue(target, point + 1, (editingOscillator == 2 ? osc2CustomPoints[point + 1] : osc1CustomPoints[point + 1]) * 0.65f + 0.175f);
    }
    else
    {
        setCustomPointValue(target, point, value);
    }

    lastDrawCustomPoint = pointIndex;
    lastDrawCustomValue = value;
    repaint();
}

void WavetableDisplay::setCustomPointValue(int oscillator, size_t pointIndex, float value)
{
    if (pointIndex >= defaultCustomWavePoints.size())
        return;

    value = juce::jlimit(0.0f, 1.0f, value);
    if (oscillator == 2)
    {
        osc2CustomPoints[pointIndex] = value;
        osc2CustomFrames[activeCustomFrameIndex(2)][pointIndex] = value;
        if (onCustomPointChange)
            onCustomPointChange(2, pointIndex, value);
    }
    else
    {
        osc1CustomPoints[pointIndex] = value;
        osc1CustomFrames[activeCustomFrameIndex(1)][pointIndex] = value;
        if (onCustomPointChange)
            onCustomPointChange(1, pointIndex, value);
    }
}

void WavetableDisplay::editPartial(const juce::MouseEvent& event, int partialIndex)
{
    partialIndex = juce::jlimit(0, 15, partialIndex);
    const auto target = editingOscillator == 2 ? 2 : 1;
    if ((target == 2 && ! osc2CustomActive) || (target != 2 && ! osc1CustomActive))
        return;

    const auto bars = partialBarsBounds();
    if (bars.isEmpty())
        return;

    const auto desired = juce::jlimit(0.0f,
                                     1.0f,
                                     1.0f - ((event.position.y - bars.getY()) / juce::jmax(1.0f, bars.getHeight())));
    auto points = target == 2 ? osc2CustomPoints : osc1CustomPoints;
    const auto analysis = analysePoints(points);
    const auto current = analysis.partials[static_cast<size_t>(partialIndex)];
    const auto harmonic = static_cast<float>(partialIndex + 1);
    const auto delta = (desired - current) * 0.90f;
    auto peak = 0.0001f;
    std::array<float, customPointCount> bipolar {};

    for (size_t index = 0; index < points.size(); ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(points.size());
        bipolar[index] = ((points[index] * 2.0f) - 1.0f)
            + (std::sin(juce::MathConstants<float>::twoPi * phase * harmonic) * delta);
        peak = juce::jmax(peak, std::abs(bipolar[index]));
    }

    const auto normalise = peak > 1.0f ? peak : 1.0f;
    for (size_t index = 0; index < points.size(); ++index)
        setCustomPointValue(target, index, 0.5f + ((bipolar[index] / normalise) * 0.5f));

    repaint();
}

juce::Rectangle<float> WavetableDisplay::editorPlotBounds() const
{
    auto visualArea = getLocalBounds().toFloat().reduced(9.0f, 8.0f);
    if (visualArea.getHeight() >= 118.0f && visualArea.getWidth() >= 180.0f)
    {
        visualArea.removeFromTop(20.0f);
        visualArea.removeFromTop(4.0f);
    }

    if (visualArea.getHeight() >= 112.0f)
    {
        visualArea.removeFromBottom(28.0f);
        visualArea.removeFromBottom(4.0f);
    }

    return visualArea;
}

juce::Rectangle<float> WavetableDisplay::frameStripBounds() const
{
    auto visualArea = getLocalBounds().toFloat().reduced(9.0f, 8.0f);
    if (visualArea.getHeight() < 118.0f || visualArea.getWidth() < 180.0f)
        return {};

    return visualArea.removeFromTop(20.0f);
}

juce::Rectangle<float> WavetableDisplay::frameRailBounds() const
{
    const auto frameStrip = frameStripBounds();
    if (frameStrip.isEmpty())
        return {};

    return frameStrip.withTrimmedLeft(43.0f).withTrimmedRight(5.0f).reduced(1.0f, 2.0f);
}

juce::Rectangle<float> WavetableDisplay::partialBarsBounds() const
{
    auto visualArea = getLocalBounds().toFloat().reduced(9.0f, 8.0f);
    if (visualArea.getHeight() >= 118.0f && visualArea.getWidth() >= 180.0f)
    {
        visualArea.removeFromTop(20.0f);
        visualArea.removeFromTop(4.0f);
    }

    if (visualArea.getHeight() < 112.0f)
        return {};

    auto analysisArea = visualArea.removeFromBottom(28.0f);
    analysisArea.removeFromRight(analysisArea.getWidth() * 0.42f);
    return analysisArea.reduced(1.0f, 2.0f);
}

juce::Rectangle<float> WavetableDisplay::spectrumBounds() const
{
    auto visualArea = getLocalBounds().toFloat().reduced(9.0f, 8.0f);
    if (visualArea.getHeight() >= 118.0f && visualArea.getWidth() >= 180.0f)
    {
        visualArea.removeFromTop(20.0f);
        visualArea.removeFromTop(4.0f);
    }

    if (visualArea.getHeight() < 112.0f)
        return {};

    auto analysisArea = visualArea.removeFromBottom(28.0f);
    analysisArea.removeFromLeft(analysisArea.getWidth() * 0.58f);
    return analysisArea.reduced(1.0f, 2.0f);
}

int WavetableDisplay::partialForEvent(const juce::MouseEvent& event) const noexcept
{
    const auto target = oscillatorForEvent(event);
    if ((target == 2 && ! osc2CustomActive) || (target != 2 && ! osc1CustomActive))
        return -1;

    const auto bars = partialBarsBounds();
    if (bars.isEmpty() || ! bars.contains(event.position))
        return -1;

    const auto normalised = juce::jlimit(0.0f, 0.999f,
                                        (event.position.x - bars.getX()) / juce::jmax(1.0f, bars.getWidth()));
    return juce::jlimit(0, 15, static_cast<int>(std::floor(normalised * 16.0f)));
}

int WavetableDisplay::frameForRailEvent(const juce::MouseEvent& event) const noexcept
{
    const auto rail = frameRailBounds();
    if (rail.isEmpty() || ! rail.contains(event.position))
        return -1;

    const auto normalised = juce::jlimit(0.0f, 0.999f,
                                        (event.position.x - rail.getX()) / juce::jmax(1.0f, rail.getWidth()));
    return juce::jlimit(0,
                        static_cast<int>(customFrameCount - 1),
                        static_cast<int>(std::floor(normalised * static_cast<float>(customFrameCount))));
}

std::array<juce::Rectangle<float>, 3>
WavetableDisplay::actionBadgeBoundsForRailCell(juce::Rectangle<float> cell) const
{
    std::array<juce::Rectangle<float>, 3> badges {};
    if (cell.getWidth() < 32.0f || cell.getHeight() < 12.0f)
        return badges;

    auto badgeRow = cell.reduced(2.0f, 2.0f).removeFromBottom(9.0f).withTrimmedLeft(11.0f);
    const auto gap = 1.0f;
    const auto badgeWidth = juce::jmax(4.0f,
                                       (badgeRow.getWidth() - (gap * 2.0f)) / static_cast<float>(badges.size()));

    for (size_t index = 0; index < badges.size(); ++index)
    {
        badges[index] = badgeRow.removeFromLeft(badgeWidth);
        if (index + 1 < badges.size())
            badgeRow.removeFromLeft(gap);
    }

    return badges;
}

size_t WavetableDisplay::activeCustomFrameIndex(int oscillator) const noexcept
{
    const auto position = oscillator == 2 ? osc2Position : osc1Position;
    return static_cast<size_t>(
        juce::jlimit(0,
                     static_cast<int>(customFrameCount - 1),
                     juce::roundToInt(juce::jlimit(0.0f, 1.0f, position)
                                      * static_cast<float>(customFrameCount - 1))));
}

void WavetableDisplay::drawMiniWave(juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    float position,
                                    const CustomPointArray* customPoints,
                                    juce::Colour colour,
                                    float alpha) const
{
    if (bounds.getWidth() <= 4.0f || bounds.getHeight() <= 4.0f)
        return;

    juce::Path path;
    constexpr auto points = 28;
    for (auto index = 0; index < points; ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(points - 1);
        auto sample = 0.0f;
        if (customPoints != nullptr)
        {
            sample = sampleCustomPoints(*customPoints, phase);
        }
        else
        {
            sample = sampleFrame(phase, position);
        }

        const auto x = bounds.getX() + (bounds.getWidth() * phase);
        const auto y = bounds.getCentreY() - (sample * bounds.getHeight() * 0.38f);
        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.setColour(colour.withAlpha(alpha));
    g.strokePath(path, juce::PathStrokeType(0.85f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

float WavetableDisplay::sampleCustomPoints(const CustomPointArray& points, float phase)
{
    const auto pointPosition = juce::jlimit(0.0f, 0.999f, phase) * static_cast<float>(points.size());
    const auto lowerIndex = static_cast<size_t>(std::floor(pointPosition)) % points.size();
    const auto upperIndex = (lowerIndex + 1) % points.size();
    const auto mix = pointPosition - std::floor(pointPosition);
    return ((points[lowerIndex] + ((points[upperIndex] - points[lowerIndex]) * mix)) * 2.0f) - 1.0f;
}

float WavetableDisplay::sampleCustomFrameSet(const CustomFrameSet& frames, float phase, float position)
{
    const auto framePosition = juce::jlimit(0.0f, 1.0f, position) * static_cast<float>(frames.size() - 1);
    const auto lowerFrame = static_cast<size_t>(juce::jlimit(0,
                                                            static_cast<int>(frames.size() - 1),
                                                            static_cast<int>(std::floor(framePosition))));
    const auto upperFrame = static_cast<size_t>(juce::jlimit(0,
                                                            static_cast<int>(frames.size() - 1),
                                                            static_cast<int>(lowerFrame + 1)));
    const auto mix = juce::jlimit(0.0f, 1.0f, framePosition - static_cast<float>(lowerFrame));
    const auto smoothMix = mix * mix * (3.0f - (2.0f * mix));
    const auto lowerSample = sampleCustomPoints(frames[lowerFrame], phase);
    const auto upperSample = sampleCustomPoints(frames[upperFrame], phase);
    return lowerSample + ((upperSample - lowerSample) * smoothMix);
}

float WavetableDisplay::sampleFrame(float phase, float position)
{
    constexpr auto lastFrame = 4.0f;
    const auto framePosition = juce::jlimit(0.0f, 1.0f, position) * lastFrame;
    const auto lowerFrame = juce::jlimit(0, static_cast<int>(lastFrame), static_cast<int>(std::floor(framePosition)));
    const auto upperFrame = juce::jlimit(0, static_cast<int>(lastFrame), lowerFrame + 1);
    const auto mix = juce::jlimit(0.0f, 1.0f, framePosition - static_cast<float>(lowerFrame));
    const auto smoothMix = mix * mix * (3.0f - (2.0f * mix));
    const auto lowerSample = drawFrame(lowerFrame, phase);
    const auto upperSample = drawFrame(upperFrame, phase);

    return lowerSample + ((upperSample - lowerSample) * smoothMix);
}

juce::Path WavetableDisplay::makeCustomPath(juce::Rectangle<float> bounds, const CustomPointArray& points)
{
    juce::Path path;

    for (size_t index = 0; index <= points.size(); ++index)
    {
        const auto pointIndex = index % points.size();
        const auto phase = static_cast<float>(index) / static_cast<float>(points.size());
        const auto x = bounds.getX() + (bounds.getWidth() * phase);
        const auto y = bounds.getBottom() - (bounds.getHeight() * juce::jlimit(0.0f, 1.0f, points[pointIndex]));

        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

juce::Path WavetableDisplay::makePath(juce::Rectangle<float> bounds, float position)
{
    juce::Path path;
    constexpr auto points = 96;

    for (auto index = 0; index < points; ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(points - 1);
        const auto sample = juce::jlimit(-1.0f, 1.0f, sampleFrame(phase, position));
        const auto x = bounds.getX() + (bounds.getWidth() * phase);
        const auto y = bounds.getCentreY() - (sample * bounds.getHeight() * 0.42f);

        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

juce::String WavetableDisplay::modulationText(float amount)
{
    amount = juce::jlimit(-1.0f, 1.0f, amount);
    return (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%";
}
}
