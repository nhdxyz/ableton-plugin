#include "RandomMorphPad.h"

#include <cmath>
#include <utility>

namespace UI
{
namespace
{
constexpr std::array<const char*, 7> sectionNames { "SRC", "ENV", "FLT", "SMP", "FX", "SEQ", "MAC" };
}

RandomMorphPad::RandomMorphPad()
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);
}

void RandomMorphPad::setState(State newState)
{
    newState.x = clamp01(newState.x);
    newState.y = clamp01(newState.y);
    newState.amount = clamp01(newState.amount);
    newState.chaos = clamp01(newState.chaos);
    newState.brightness = juce::jlimit(-1.0f, 1.0f, newState.brightness);
    newState.drive = juce::jlimit(-1.0f, 1.0f, newState.drive);
    newState.motion = juce::jlimit(-1.0f, 1.0f, newState.motion);
    for (auto& value : newState.sectionIntensities)
        value = clamp01(value);

    state = std::move(newState);
    repaint();
}

juce::String RandomMorphPad::getTooltip()
{
    return "Random Lab sound map. Drag to steer tone, motion, chaos, drive, and section strengths; release to create a variation.";
}

void RandomMorphPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    auto content = bounds.reduced(10.0f, 8.0f);
    const auto accent = juce::Colour(0xff8ee6c9);
    const auto motionColour = juce::Colour(0xffc4a7ff);
    const auto driveColour = juce::Colour(0xffff9b78);
    const auto brightColour = juce::Colour(0xffffd36e);

    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = content.removeFromTop(18.0f);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(accent);
    g.drawFittedText("SOUND MAP", header.removeFromLeft(header.getWidth() * 0.45f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xff9dafb2));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawFittedText(state.recipe + " | " + state.scope, header.toNearestInt(), juce::Justification::centredRight, 1, 0.58f);

    auto meterArea = content.removeFromRight(juce::jlimit(112.0f, 152.0f, content.getWidth() * 0.24f));
    content.removeFromRight(8.0f);
    auto pad = content.withTrimmedBottom(32.0f);

    g.setColour(juce::Colour(0xff131b1f));
    g.fillRoundedRectangle(pad, 5.0f);
    g.setColour(juce::Colour(0xff334147));
    g.drawRoundedRectangle(pad, 5.0f, 1.0f);

    juce::ColourGradient field(driveColour.withAlpha(0.24f), pad.getX(), pad.getBottom(),
                               brightColour.withAlpha(0.22f), pad.getRight(), pad.getY(),
                               false);
    field.addColour(0.50, motionColour.withAlpha(0.20f));
    g.setGradientFill(field);
    g.fillRoundedRectangle(pad.reduced(1.0f), 4.0f);

    g.setColour(juce::Colour(0x66222d32));
    for (auto index = 1; index < 4; ++index)
    {
        const auto x = juce::jmap(static_cast<float>(index), 0.0f, 4.0f, pad.getX(), pad.getRight());
        const auto y = juce::jmap(static_cast<float>(index), 0.0f, 4.0f, pad.getY(), pad.getBottom());
        g.drawVerticalLine(static_cast<int>(x), pad.getY(), pad.getBottom());
        g.drawHorizontalLine(static_cast<int>(y), pad.getX(), pad.getRight());
    }

    const auto centre = pad.getCentre();
    g.setColour(juce::Colour(0x668ee6c9));
    g.drawLine(pad.getX(), centre.y, pad.getRight(), centre.y, 1.2f);
    g.drawLine(centre.x, pad.getY(), centre.x, pad.getBottom(), 1.2f);
    g.setColour(juce::Colour(0x559dafb2));
    g.drawEllipse(juce::Rectangle<float> { pad.getWidth() * 0.46f, pad.getHeight() * 0.46f }.withCentre(centre), 1.0f);

    const auto handleRange = pad.reduced(9.0f);
    const auto handle = juce::Point<float> {
        juce::jmap(state.x, 0.0f, 1.0f, handleRange.getX(), handleRange.getRight()),
        juce::jmap(state.y, 0.0f, 1.0f, handleRange.getBottom(), handleRange.getY())
    };

    g.setColour(accent.withAlpha(0.35f));
    g.drawLine(pad.getX(), handle.y, pad.getRight(), handle.y, 1.5f);
    g.setColour(motionColour.withAlpha(0.35f));
    g.drawLine(handle.x, pad.getY(), handle.x, pad.getBottom(), 1.5f);

    juce::ColourGradient glow(accent.withAlpha(0.38f), handle.x, handle.y,
                              motionColour.withAlpha(0.16f), handle.x + 32.0f, handle.y - 24.0f,
                              true);
    g.setGradientFill(glow);
    g.fillEllipse(juce::Rectangle<float> { 36.0f, 36.0f }.withCentre(handle));
    g.setColour(juce::Colour(0xff0b1012));
    g.fillEllipse(juce::Rectangle<float> { 14.0f, 14.0f }.withCentre(handle));
    g.setColour(accent);
    g.drawEllipse(juce::Rectangle<float> { 14.0f, 14.0f }.withCentre(handle), 1.7f);

    g.setFont(juce::FontOptions(8.3f, juce::Font::bold));
    g.setColour(driveColour);
    g.drawFittedText("DRIVE", pad.reduced(8.0f, 6.0f).removeFromBottom(11.0f).toNearestInt(), juce::Justification::bottomLeft, 1, 0.58f);
    g.setColour(brightColour);
    g.drawFittedText("BRIGHT", pad.reduced(8.0f, 6.0f).removeFromTop(11.0f).toNearestInt(), juce::Justification::topRight, 1, 0.58f);
    g.setColour(motionColour);
    g.drawFittedText("MOTION", pad.reduced(8.0f, 6.0f).removeFromTop(11.0f).toNearestInt(), juce::Justification::topLeft, 1, 0.58f);
    g.setColour(juce::Colour(0xff9dafb2));
    g.drawFittedText("CONTROL", pad.reduced(8.0f, 6.0f).removeFromBottom(11.0f).toNearestInt(), juce::Justification::bottomRight, 1, 0.58f);

    auto footer = content.removeFromBottom(24.0f).withTrimmedTop(6.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText("MORPH VECTOR", footer.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);
    g.setColour(accent);
    const auto coordText = juce::String(juce::roundToInt(state.x * 100.0f)) + " / "
                         + juce::String(juce::roundToInt(state.y * 100.0f));
    g.drawFittedText(coordText, footer.toNearestInt(), juce::Justification::centredRight, 1, 0.58f);

    auto drawMeter = [&g] (juce::Rectangle<float> area,
                           const juce::String& label,
                           float value,
                           juce::Colour colour,
                           bool bipolar)
    {
        value = bipolar ? juce::jlimit(-1.0f, 1.0f, value)
                        : juce::jlimit(0.0f, 1.0f, value);

        g.setFont(juce::FontOptions(8.2f, juce::Font::bold));
        g.setColour(juce::Colour(0xff849196));
        g.drawFittedText(label, area.removeFromTop(10.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

        auto track = area.removeFromTop(6.0f);
        g.setColour(juce::Colour(0xff223036));
        g.fillRoundedRectangle(track, 3.0f);
        if (bipolar)
        {
            const auto centreX = track.getCentreX();
            const auto fillWidth = (track.getWidth() * 0.5f) * std::abs(value);
            auto fill = value >= 0.0f ? juce::Rectangle<float> { centreX, track.getY(), fillWidth, track.getHeight() }
                                      : juce::Rectangle<float> { centreX - fillWidth, track.getY(), fillWidth, track.getHeight() };
            g.setColour(colour);
            g.fillRoundedRectangle(fill, 3.0f);
            g.setColour(juce::Colour(0x66849196));
            g.drawVerticalLine(static_cast<int>(centreX), track.getY(), track.getBottom());
        }
        else
        {
            g.setColour(colour);
            g.fillRoundedRectangle(track.withWidth(track.getWidth() * value), 3.0f);
        }
    };

    auto primaryMeters = meterArea.removeFromTop(92.0f);
    drawMeter(primaryMeters.removeFromTop(18.0f), "AMOUNT", state.amount, accent, false);
    primaryMeters.removeFromTop(4.0f);
    drawMeter(primaryMeters.removeFromTop(18.0f), "CHAOS", state.chaos, motionColour, false);
    primaryMeters.removeFromTop(4.0f);
    drawMeter(primaryMeters.removeFromTop(18.0f), "BRIGHT", state.brightness, brightColour, true);
    primaryMeters.removeFromTop(4.0f);
    drawMeter(primaryMeters.removeFromTop(18.0f), "DRIVE", state.drive, driveColour, true);
    meterArea.removeFromTop(2.0f);
    drawMeter(meterArea.removeFromTop(18.0f), "MOTION", state.motion, motionColour, true);

    auto sectionArea = meterArea.withTrimmedTop(8.0f);
    const auto sectionGap = 3.0f;
    const auto sectionHeight = juce::jmax(8.0f, (sectionArea.getHeight() - (sectionGap * 6.0f)) / 7.0f);
    for (size_t index = 0; index < state.sectionIntensities.size(); ++index)
    {
        auto row = sectionArea.removeFromTop(sectionHeight);
        sectionArea.removeFromTop(sectionGap);
        g.setFont(juce::FontOptions(7.8f, juce::Font::bold));
        g.setColour(sectionColour(index));
        g.drawFittedText(sectionNames[index], row.removeFromLeft(28.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);
        auto bar = row.reduced(0.0f, 2.0f);
        g.setColour(juce::Colour(0xff223036));
        g.fillRoundedRectangle(bar, 3.0f);
        g.setColour(sectionColour(index));
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * state.sectionIntensities[index]), 3.0f);
    }
}

void RandomMorphPad::mouseDown(const juce::MouseEvent& event)
{
    mouseMoved = false;
    applyMousePosition(event.position, true);
}

void RandomMorphPad::mouseDrag(const juce::MouseEvent& event)
{
    mouseMoved = mouseMoved || event.getDistanceFromDragStart() > 2;
    applyMousePosition(event.position, true);
}

void RandomMorphPad::mouseUp(const juce::MouseEvent& event)
{
    applyMousePosition(event.position, true);
    if (onCommit != nullptr && (mouseMoved || getPadBounds().contains(event.position)))
        onCommit(state.x, state.y);
}

juce::Rectangle<float> RandomMorphPad::getPadBounds() const
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f, 8.0f);
    bounds.removeFromTop(18.0f);
    auto meterArea = bounds.removeFromRight(juce::jlimit(112.0f, 152.0f, bounds.getWidth() * 0.24f));
    juce::ignoreUnused(meterArea);
    bounds.removeFromRight(8.0f);
    return bounds.withTrimmedBottom(32.0f);
}

void RandomMorphPad::applyMousePosition(juce::Point<float> position, bool notify)
{
    const auto pad = getPadBounds();
    state.x = clamp01((position.x - pad.getX()) / juce::jmax(1.0f, pad.getWidth()));
    state.y = clamp01((pad.getBottom() - position.y) / juce::jmax(1.0f, pad.getHeight()));
    repaint();

    if (notify && onChange != nullptr)
        onChange(state.x, state.y);
}

float RandomMorphPad::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour RandomMorphPad::sectionColour(size_t index) noexcept
{
    static constexpr std::array<juce::uint32, 7> colours {
        0xff8ee6c9,
        0xffffd36e,
        0xff7bb7ff,
        0xffff9b78,
        0xffc4a7ff,
        0xff7fd0e6,
        0xffd7e37b
    };

    return juce::Colour(colours[index % colours.size()]);
}
}
