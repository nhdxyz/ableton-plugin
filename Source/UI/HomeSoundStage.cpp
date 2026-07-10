#include "HomeSoundStage.h"

#include <cmath>

namespace UI
{
namespace
{
juce::Colour oscillatorColour(const Theme& theme, size_t index)
{
    return index == 0 ? theme.accent : theme.accentSecondary;
}
}

HomeSoundStage::HomeSoundStage()
{
    setComponentID("HomeSoundStage");
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void HomeSoundStage::setTheme(const Theme& newTheme)
{
    if (theme.id == newTheme.id)
        return;

    theme = newTheme;
    repaint();
}

void HomeSoundStage::setState(State newState)
{
    for (auto& oscillator : newState.oscillators)
    {
        oscillator.level = clamp01(oscillator.level);
        oscillator.position = clamp01(oscillator.position);
        oscillator.warp = clamp01(oscillator.warp);
        for (auto& frame : oscillator.frames)
            for (auto& point : frame)
                point = clamp01(point);
    }

    for (auto& level : newState.sourceLevels)
        level = clamp01(level);
    for (auto& macro : newState.macros)
        macro = clamp01(macro);

    newState.cutoff = clamp01(newState.cutoff);
    newState.drive = clamp01(newState.drive);
    newState.outputPeak = clamp01(newState.outputPeak);
    newState.animationPhase -= std::floor(newState.animationPhase);
    state = std::move(newState);
    repaint();
}

HomeSoundStage::LayoutMetrics HomeSoundStage::getLayoutMetricsForAudit() const
{
    const auto area = getLocalBounds().toFloat().reduced(10.0f, 38.0f);
    return { area.getWidth(), area.getHeight(), area.getWidth() >= 280.0f && area.getHeight() >= 170.0f };
}

juce::String HomeSoundStage::getTooltip()
{
    const auto selected = state.osc2Selected ? 1 : 0;
    const auto& oscillator = state.oscillators[static_cast<size_t>(selected)];
    return oscillator.name + " " + oscillator.waveName
        + ": drag horizontally to scan the wavetable; double-click to open the full source editor";
}

void HomeSoundStage::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(theme.background.darker(0.16f));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto content = bounds.reduced(12.0f, 9.0f);
    auto header = content.removeFromTop(24.0f);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(theme.accent);
    g.drawFittedText("SOUND STAGE", header.removeFromLeft(104.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(theme.text);
    g.drawFittedText(state.presetName, header.toNearestInt(), juce::Justification::centredLeft, 1, 0.68f);

    auto safety = header.removeFromRight(58.0f).reduced(2.0f, 3.0f);
    const auto hot = state.safetyName == "HOT" || state.safetyName == "CLIP";
    const auto safetyColour = hot ? theme.danger : (state.safetyName == "LOW" ? theme.textDim : theme.accent);
    g.setColour(safetyColour.withAlpha(0.14f));
    g.fillRoundedRectangle(safety, 4.0f);
    g.setColour(safetyColour);
    g.drawRoundedRectangle(safety, 4.0f, 1.0f);
    g.drawFittedText(state.safetyName, safety.toNearestInt(), juce::Justification::centred, 1);

    auto footer = content.removeFromBottom(32.0f).reduced(0.0f, 5.0f);
    const std::array<const char*, 5> sourceNames { "O1", "O2", "SUB", "NOISE", "SAMPLE" };
    const auto gap = 5.0f;
    const auto sourceWidth = (footer.getWidth() - gap * 4.0f) / 5.0f;
    for (size_t index = 0; index < sourceNames.size(); ++index)
    {
        auto cell = footer.removeFromLeft(sourceWidth);
        footer.removeFromLeft(gap);
        auto label = cell.removeFromLeft(42.0f);
        auto rail = cell.reduced(0.0f, 7.0f);
        const auto colour = index < 2 ? oscillatorColour(theme, index)
                                     : (index == 4 ? theme.warning : theme.textMuted);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.setColour(theme.textDim);
        g.drawFittedText(sourceNames[index], label.toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);
        g.setColour(theme.outline);
        g.fillRoundedRectangle(rail, 2.0f);
        g.setColour(colour.withAlpha(0.76f));
        g.fillRoundedRectangle(rail.withWidth(rail.getWidth() * state.sourceLevels[index]), 2.0f);
    }

    const auto areas = oscillatorAreas();
    for (size_t oscillatorIndex = 0; oscillatorIndex < state.oscillators.size(); ++oscillatorIndex)
    {
        const auto& oscillator = state.oscillators[oscillatorIndex];
        auto area = areas[oscillatorIndex];
        const auto selected = state.osc2Selected == (oscillatorIndex == 1);
        const auto hovered = hoveredOscillator == static_cast<int>(oscillatorIndex);
        auto accent = oscillatorColour(theme, oscillatorIndex);
        if (! oscillator.active)
            accent = theme.textDim;

        g.setColour(selected ? accent.withAlpha(0.10f) : theme.panelAlt.withAlpha(0.46f));
        g.fillRoundedRectangle(area, 5.0f);
        g.setColour(selected || hovered ? accent.withAlpha(0.92f) : theme.outline.withAlpha(0.72f));
        g.drawRoundedRectangle(area, 5.0f, selected ? 1.5f : 1.0f);

        auto labelArea = area.reduced(8.0f, 5.0f).removeFromLeft(74.0f);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(accent);
        g.drawFittedText(oscillator.name, labelArea.removeFromTop(13.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        g.setColour(theme.textMuted);
        g.drawFittedText(oscillator.waveName, labelArea.removeFromTop(12.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);
        g.setColour(theme.textDim);
        g.drawFittedText(juce::String(juce::roundToInt(oscillator.position * 100.0f)) + "%",
                         labelArea.removeFromTop(12.0f).toNearestInt(), juce::Justification::centredLeft, 1);

        auto waveArea = area.reduced(84.0f, 8.0f);
        const auto visibleFrames = state.perspective ? frameCount : size_t { 1 };
        for (size_t drawIndex = 0; drawIndex < visibleFrames; ++drawIndex)
        {
            const auto frameIndex = state.perspective ? frameCount - 1 - drawIndex
                                                      : juce::jlimit<size_t>(0, frameCount - 1,
                                                          static_cast<size_t>(std::round(oscillator.position * 7.0f)));
            auto frameBounds = waveArea;
            if (state.perspective)
            {
                const auto depth = static_cast<float>(drawIndex) / static_cast<float>(frameCount - 1);
                frameBounds = frameBounds.reduced(depth * 14.0f, depth * 3.0f)
                                         .translated(depth * 13.0f, -depth * 2.0f);
            }

            const auto framePosition = static_cast<float>(frameIndex) / static_cast<float>(frameCount - 1);
            const auto positionDistance = std::abs(framePosition - oscillator.position);
            const auto current = positionDistance <= 0.075f;
            g.setColour(accent.withAlpha(current ? 0.96f : (0.12f + (0.18f * (1.0f - positionDistance)))));
            g.strokePath(wavePath(oscillator, frameIndex, frameBounds),
                         juce::PathStrokeType(current ? 2.0f : 1.0f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
        }

        const auto cursorX = waveArea.getX() + waveArea.getWidth() * oscillator.position;
        g.setColour(accent.withAlpha(0.22f));
        g.drawVerticalLine(juce::roundToInt(cursorX), waveArea.getY(), waveArea.getBottom());
        g.setColour(accent);
        g.fillEllipse(cursorX - 3.0f, waveArea.getBottom() - 5.0f, 6.0f, 6.0f);
    }

    if (state.animate)
    {
        const auto pulseX = content.getX() + content.getWidth() * state.animationPhase;
        auto pulse = juce::ColourGradient(theme.accent.withAlpha(0.0f), pulseX - 30.0f, content.getCentreY(),
                                          theme.accent.withAlpha(0.12f), pulseX, content.getCentreY(), false);
        pulse.addColour(1.0, theme.accent.withAlpha(0.0f));
        g.setGradientFill(pulse);
        g.fillRect(juce::Rectangle<float>(pulseX - 30.0f, content.getY(), 60.0f, content.getHeight()));
    }
}

void HomeSoundStage::mouseMove(const juce::MouseEvent& event)
{
    const auto next = oscillatorAt(event.position);
    if (next == hoveredOscillator)
        return;

    hoveredOscillator = next;
    repaint();
}

void HomeSoundStage::mouseExit(const juce::MouseEvent&)
{
    hoveredOscillator = -1;
    repaint();
}

void HomeSoundStage::mouseDown(const juce::MouseEvent& event)
{
    editingOscillator = oscillatorAt(event.position);
    if (editingOscillator < 0)
        return;

    const auto osc2 = editingOscillator == 1;
    state.osc2Selected = osc2;
    if (onOscillatorSelected)
        onOscillatorSelected(osc2);
    positionEditStarted = false;
}

void HomeSoundStage::mouseDrag(const juce::MouseEvent& event)
{
    if (editingOscillator >= 0 && ! positionEditStarted)
    {
        positionEditStarted = true;
        if (onPositionEditStart)
            onPositionEditStart(editingOscillator == 1);
    }
    updatePositionAt(event.position);
}

void HomeSoundStage::mouseUp(const juce::MouseEvent&)
{
    editingOscillator = -1;
    positionEditStarted = false;
}

void HomeSoundStage::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto oscillator = oscillatorAt(event.position);
    if (oscillator < 0)
        return;

    const auto osc2 = oscillator == 1;
    state.osc2Selected = osc2;
    if (onOscillatorSelected)
        onOscillatorSelected(osc2);
    if (onOpenEditor)
        onOpenEditor(osc2);
}

std::array<juce::Rectangle<float>, 2> HomeSoundStage::oscillatorAreas() const
{
    auto content = getLocalBounds().toFloat().reduced(13.0f, 10.0f);
    content.removeFromTop(28.0f);
    content.removeFromBottom(32.0f);
    const auto gap = 7.0f;
    const auto height = (content.getHeight() - gap) * 0.5f;
    std::array<juce::Rectangle<float>, 2> areas;
    areas[0] = content.removeFromTop(height);
    content.removeFromTop(gap);
    areas[1] = content;
    return areas;
}

int HomeSoundStage::oscillatorAt(juce::Point<float> point) const
{
    const auto areas = oscillatorAreas();
    for (size_t index = 0; index < areas.size(); ++index)
        if (areas[index].contains(point))
            return static_cast<int>(index);
    return -1;
}

void HomeSoundStage::updatePositionAt(juce::Point<float> point)
{
    if (editingOscillator < 0)
        return;

    auto waveArea = oscillatorAreas()[static_cast<size_t>(editingOscillator)].reduced(84.0f, 8.0f);
    const auto position = clamp01((point.x - waveArea.getX()) / juce::jmax(1.0f, waveArea.getWidth()));
    auto& oscillator = state.oscillators[static_cast<size_t>(editingOscillator)];
    if (std::abs(oscillator.position - position) < 0.001f)
        return;

    oscillator.position = position;
    if (onPositionChange)
        onPositionChange(editingOscillator == 1, position);
    repaint();
}

juce::Path HomeSoundStage::wavePath(const OscillatorState& oscillator,
                                    size_t frameIndex,
                                    juce::Rectangle<float> bounds) const
{
    juce::Path path;
    constexpr auto sampleCount = 72;
    const auto centreY = bounds.getCentreY();
    const auto amplitude = bounds.getHeight() * (0.28f + oscillator.level * 0.13f);
    for (auto index = 0; index < sampleCount; ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(sampleCount - 1);
        auto value = waveValue(oscillator, frameIndex, phase);
        value = std::tanh(value * (1.0f + oscillator.warp * 1.8f));
        const auto x = bounds.getX() + bounds.getWidth() * phase;
        const auto y = centreY - value * amplitude;
        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }
    return path;
}

float HomeSoundStage::waveValue(const OscillatorState& oscillator, size_t frameIndex, float phase) const
{
    const auto angle = phase * juce::MathConstants<float>::twoPi;
    switch (oscillator.waveType)
    {
        case 0: return std::sin(angle);
        case 1: return (phase * 2.0f) - 1.0f;
        case 2: return phase < 0.5f ? 1.0f : -1.0f;
        case 3: return 1.0f - (4.0f * std::abs(phase - 0.5f));
        case 4:
        {
            const auto harmonic = 1.0f + static_cast<float>(frameIndex) * 0.42f;
            return juce::jlimit(-1.0f, 1.0f,
                                std::sin(angle) * 0.72f
                                    + std::sin(angle * (harmonic + 1.0f)) * 0.28f);
        }
        case 5:
            return juce::jlimit(-1.0f, 1.0f,
                                std::sin(angle) * 0.72f + std::sin(angle * 2.0f) * 0.22f
                                    + std::sin(angle * 3.0f) * 0.12f);
        case 6:
            return juce::jlimit(-1.0f, 1.0f,
                                std::sin(angle) * 0.66f + std::sin(angle * 2.01f) * 0.20f
                                    + std::sin(angle * 4.03f) * 0.12f);
        default:
        {
            const auto pointPosition = phase * static_cast<float>(pointCount - 1);
            const auto lower = juce::jlimit<size_t>(0, pointCount - 1, static_cast<size_t>(pointPosition));
            const auto upper = juce::jmin(pointCount - 1, lower + 1);
            const auto fraction = pointPosition - static_cast<float>(lower);
            const auto normalised = oscillator.frames[frameIndex][lower]
                + (oscillator.frames[frameIndex][upper] - oscillator.frames[frameIndex][lower]) * fraction;
            return (normalised * 2.0f) - 1.0f;
        }
    }
}

float HomeSoundStage::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}
}
