#include "HomeOverviewDisplay.h"

#include <cmath>

namespace UI
{
float HomeOverviewDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour HomeOverviewDisplay::accentForIndex(size_t index)
{
    static constexpr std::array<juce::uint32, 8> colours {
        0xff8ee6c9,
        0xffffc36b,
        0xff7bb7ff,
        0xffff8f78,
        0xffd7e37b,
        0xff7fd0e6,
        0xfff0a86e,
        0xffc4a7ff
    };

    return juce::Colour(colours[index % colours.size()]);
}

void HomeOverviewDisplay::setState(const State& newState)
{
    state = newState;
    for (auto& source : state.sources)
        source = clamp01(source);
    for (auto& macro : state.macros)
        macro = clamp01(macro);

    state.cutoff = clamp01(state.cutoff);
    state.drive = clamp01(state.drive);
    state.pumpReduction = clamp01(state.pumpReduction);
    state.guardReduction = clamp01(state.guardReduction);
    state.outputPeak = clamp01(state.outputPeak);
    state.delaySend = clamp01(state.delaySend);
    state.reverbSend = clamp01(state.reverbSend);

    repaint();
}

void HomeOverviewDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto corner = 6.0f;

    g.setColour(juce::Colour(0xff0b1012));
    g.fillRoundedRectangle(bounds, corner);
    g.setColour(juce::Colour(0xff2d383e));
    g.drawRoundedRectangle(bounds, corner, 1.0f);

    auto content = bounds.reduced(14.0f, 11.0f);
    auto header = content.removeFromTop(24.0f);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText("OVERVIEW", header.removeFromLeft(86.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour(juce::Colour(0xffdce7e4));
    g.drawFittedText(state.sourceName,
                     header.removeFromLeft(132.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1);

    auto statusArea = header.removeFromRight(84.0f);
    const auto safetyHot = state.outputPeak >= 0.9f || state.safetyName.containsIgnoreCase("HOT") || state.safetyName.containsIgnoreCase("CLIP");
    const auto statusFill = safetyHot ? juce::Colour(0xff5a2d2a) : juce::Colour(0xff1f4039);
    const auto statusText = safetyHot ? juce::Colour(0xffffb0a0) : juce::Colour(0xff8ee6c9);
    g.setColour(statusFill);
    g.fillRoundedRectangle(statusArea.reduced(0.0f, 2.0f), 4.0f);
    g.setColour(statusText);
    g.drawFittedText(state.safetyName, statusArea.toNearestInt(), juce::Justification::centred, 1);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff829096));
    g.drawFittedText(state.recipeName, header.toNearestInt(), juce::Justification::centredRight, 1);

    content.removeFromTop(6.0f);
    auto left = content.removeFromLeft(86.0f);
    content.removeFromLeft(12.0f);
    auto right = content.removeFromRight(100.0f);
    content.removeFromRight(12.0f);
    auto scope = content;

    const std::array<juce::String, 4> sourceLabels { "O1", "O2", "SUB", "NOISE" };
    auto sourceRow = left;
    const auto sourceGap = 5.0f;
    const auto sourceBarWidth = (sourceRow.getWidth() - (sourceGap * 3.0f)) / 4.0f;
    for (size_t index = 0; index < state.sources.size(); ++index)
    {
        auto bar = sourceRow.removeFromLeft(sourceBarWidth);
        sourceRow.removeFromLeft(sourceGap);
        const auto level = clamp01(state.sources[index]);
        const auto fillHeight = juce::jmax(3.0f, bar.getHeight() * level);
        auto rail = bar.withTrimmedTop(15.0f);

        g.setColour(juce::Colour(0xff182025));
        g.fillRoundedRectangle(rail, 3.0f);
        g.setColour(accentForIndex(index).withAlpha(0.85f));
        g.fillRoundedRectangle(rail.withTop(rail.getBottom() - fillHeight), 3.0f);
        g.setColour(juce::Colour(0xff9aa8aa));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawFittedText(sourceLabels[index], bar.removeFromTop(13.0f).toNearestInt(), juce::Justification::centred, 1);
    }

    g.setColour(juce::Colour(0xff141c20));
    g.fillRoundedRectangle(scope, 5.0f);
    g.setColour(juce::Colour(0xff28343a));
    g.drawRoundedRectangle(scope, 5.0f, 1.0f);

    auto scopeInner = scope.reduced(12.0f, 10.0f);
    auto scopeHeader = scopeInner.removeFromTop(14.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff829096));
    g.drawFittedText("MOTION / TONE", scopeHeader.toNearestInt(), juce::Justification::centredLeft, 1);

    auto waveArea = scopeInner.withTrimmedTop(4.0f);
    juce::Path wave;
    const auto motion = state.macros[2];
    const auto space = state.macros[3];
    const auto dirt = state.macros[1];
    const auto amp = juce::jmap(0.18f + motion + dirt, 0.18f, 2.18f, 0.10f, 0.44f);
    const auto cycles = 1.5f + (state.cutoff * 3.5f) + (space * 1.4f);
    const auto centreY = waveArea.getCentreY();

    for (auto point = 0; point < 96; ++point)
    {
        const auto normalisedX = static_cast<float>(point) / 95.0f;
        const auto x = waveArea.getX() + (waveArea.getWidth() * normalisedX);
        const auto harmonic = std::sin(normalisedX * juce::MathConstants<float>::twoPi * cycles)
            + (std::sin(normalisedX * juce::MathConstants<float>::twoPi * (cycles * 2.0f + 0.35f)) * dirt * 0.55f);
        const auto y = centreY - (harmonic * waveArea.getHeight() * amp);
        if (point == 0)
            wave.startNewSubPath(x, y);
        else
            wave.lineTo(x, y);
    }

    const auto peakFillHeight = waveArea.getHeight() * clamp01(state.outputPeak);
    auto peakFill = waveArea.withTop(waveArea.getBottom() - peakFillHeight);
    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(0.18f));
    g.fillRoundedRectangle(peakFill, 4.0f);
    g.setColour(juce::Colour(0xff8ee6c9));
    g.strokePath(wave, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto macroStrip = scope.removeFromBottom(28.0f).reduced(12.0f, 6.0f);
    const auto macroGap = 4.0f;
    const auto macroWidth = (macroStrip.getWidth() - (macroGap * 7.0f)) / 8.0f;
    for (size_t index = 0; index < state.macros.size(); ++index)
    {
        auto macro = macroStrip.removeFromLeft(macroWidth);
        macroStrip.removeFromLeft(macroGap);
        g.setColour(juce::Colour(0xff202a2f));
        g.fillRoundedRectangle(macro, 2.0f);
        g.setColour(accentForIndex(index));
        g.fillRoundedRectangle(macro.withWidth(macro.getWidth() * clamp01(state.macros[index])), 2.0f);
    }

    const std::array<juce::String, 5> rightLabels { "CUT", "DRIVE", "PUMP", "SEND", "OUT" };
    const std::array<float, 5> rightValues {
        state.cutoff,
        state.drive,
        state.pumpReduction,
        juce::jmax(state.delaySend, state.reverbSend),
        state.outputPeak
    };

    auto meterArea = right;
    const auto meterHeight = meterArea.getHeight() / 5.0f;
    for (size_t index = 0; index < rightLabels.size(); ++index)
    {
        auto row = meterArea.removeFromTop(meterHeight).reduced(0.0f, 3.0f);
        auto labelArea = row.removeFromLeft(40.0f);
        auto rail = row.reduced(0.0f, 5.0f);

        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff8a989e));
        g.drawFittedText(rightLabels[index], labelArea.toNearestInt(), juce::Justification::centredLeft, 1);
        g.setColour(juce::Colour(0xff1c252a));
        g.fillRoundedRectangle(rail, 3.0f);
        g.setColour(accentForIndex(index + 2));
        g.fillRoundedRectangle(rail.withWidth(rail.getWidth() * clamp01(rightValues[index])), 3.0f);
    }

    if (state.guardReduction > 0.005f)
    {
        auto guard = bounds.reduced(12.0f).removeFromBottom(5.0f);
        g.setColour(juce::Colour(0xffffc36b).withAlpha(0.32f));
        g.fillRoundedRectangle(guard.withWidth(guard.getWidth() * state.guardReduction), 2.0f);
    }
}
}
