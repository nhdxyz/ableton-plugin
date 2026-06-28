#include "FilterResponseDisplay.h"

#include <cmath>

namespace UI
{
namespace
{
constexpr auto minFrequencyHz = 20.0f;
constexpr auto maxFrequencyHz = 20000.0f;
constexpr auto frequencyRangeRatio = maxFrequencyHz / minFrequencyHz;
constexpr auto responseCeiling = 1.32f;

bool nearlyEqual(float left, float right)
{
    return std::abs(left - right) < 0.001f;
}

float resonanceNormalised(float resonance)
{
    return juce::jlimit(0.0f, 1.0f, (resonance - 0.1f) / 1.3f);
}

float log2Ratio(float numerator, float denominator)
{
    numerator = juce::jmax(0.001f, numerator);
    denominator = juce::jmax(0.001f, denominator);
    return std::log(numerator / denominator) / std::log(2.0f);
}

float smoothStep(float value)
{
    value = juce::jlimit(0.0f, 1.0f, value);
    return value * value * (3.0f - (2.0f * value));
}
}

void FilterResponseDisplay::setState(const State& newState)
{
    State clipped;
    clipped.cutoffHz = juce::jlimit(minFrequencyHz, maxFrequencyHz, newState.cutoffHz);
    clipped.resonance = juce::jlimit(0.1f, 1.4f, newState.resonance);
    clipped.envAmount = juce::jlimit(-1.0f, 1.0f, newState.envAmount);
    clipped.drive = juce::jlimit(0.0f, 1.0f, newState.drive);
    clipped.cutoffModAmount = juce::jlimit(-1.0f, 1.0f, newState.cutoffModAmount);
    clipped.resonanceModAmount = juce::jlimit(-1.0f, 1.0f, newState.resonanceModAmount);
    clipped.envModAmount = juce::jlimit(-1.0f, 1.0f, newState.envModAmount);
    clipped.driveModAmount = juce::jlimit(-1.0f, 1.0f, newState.driveModAmount);
    clipped.modRouteCount = juce::jmax(0, newState.modRouteCount);
    clipped.modSourceSummary = newState.modSourceSummary.trim();
    clipped.mode = juce::jlimit(0, 2, newState.mode);
    clipped.character = juce::jlimit(0, 3, newState.character);
    clipped.slope = juce::jlimit(0, 1, newState.slope);

    if (nearlyEqual(state.cutoffHz, clipped.cutoffHz)
        && nearlyEqual(state.resonance, clipped.resonance)
        && nearlyEqual(state.envAmount, clipped.envAmount)
        && nearlyEqual(state.drive, clipped.drive)
        && nearlyEqual(state.cutoffModAmount, clipped.cutoffModAmount)
        && nearlyEqual(state.resonanceModAmount, clipped.resonanceModAmount)
        && nearlyEqual(state.envModAmount, clipped.envModAmount)
        && nearlyEqual(state.driveModAmount, clipped.driveModAmount)
        && state.modRouteCount == clipped.modRouteCount
        && state.modSourceSummary == clipped.modSourceSummary
        && state.mode == clipped.mode
        && state.character == clipped.character
        && state.slope == clipped.slope)
    {
        return;
    }

    state = clipped;
    repaint();
}

void FilterResponseDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto accent = characterColour(state.character);
    const auto plot = bounds.withTrimmedTop(17.0f).reduced(8.0f, 6.0f);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(accent.withAlpha(0.58f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = getLocalBounds().removeFromTop(16).reduced(8, 1);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(accent);
    g.drawText("FILTER", header, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffa9b7bb));
    g.drawText(modeText(state.mode) + " " + slopeText(state.slope) + " " + characterText(state.character),
               header,
               juce::Justification::centredRight);

    g.setColour(juce::Colour(0x223d4a50));
    for (auto line = 1; line < 4; ++line)
    {
        const auto y = plot.getY() + (plot.getHeight() * static_cast<float>(line) / 4.0f);
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
    }

    const auto drawFrequencyMarker = [&] (float frequencyHz, const juce::String& label)
    {
        const auto x = plot.getX() + (plot.getWidth() * normaliseFrequency(frequencyHz));
        g.setColour(juce::Colour(0x305b6e75));
        g.drawLine(x, plot.getY(), x, plot.getBottom(), 1.0f);
        g.setColour(juce::Colour(0xff617078));
        g.setFont(juce::FontOptions(8.5f));
        g.drawText(label,
                   juce::Rectangle<float>(x - 15.0f, plot.getBottom() - 10.0f, 30.0f, 10.0f),
                   juce::Justification::centred);
    };

    drawFrequencyMarker(80.0f, "80");
    drawFrequencyMarker(1000.0f, "1K");
    drawFrequencyMarker(10000.0f, "10K");

    const auto cutoffX = plot.getX() + (plot.getWidth() * normaliseFrequency(state.cutoffHz));

    if (std::abs(state.cutoffModAmount) > 0.01f)
    {
        auto lowState = state;
        auto highState = state;
        const auto lowScale = std::pow(2.0f, juce::jmin(0.0f, state.cutoffModAmount) * 2.5f);
        const auto highScale = std::pow(2.0f, juce::jmax(0.0f, state.cutoffModAmount) * 2.5f);
        lowState.cutoffHz = juce::jlimit(minFrequencyHz, maxFrequencyHz, state.cutoffHz * lowScale);
        highState.cutoffHz = juce::jlimit(minFrequencyHz, maxFrequencyHz, state.cutoffHz * highScale);
        lowState.resonance = juce::jlimit(0.1f, 1.4f, state.resonance + (juce::jmin(0.0f, state.resonanceModAmount) * 0.55f));
        highState.resonance = juce::jlimit(0.1f, 1.4f, state.resonance + (juce::jmax(0.0f, state.resonanceModAmount) * 0.55f));
        lowState.drive = juce::jlimit(0.0f, 1.0f, state.drive + (juce::jmin(0.0f, state.driveModAmount) * 0.35f));
        highState.drive = juce::jlimit(0.0f, 1.0f, state.drive + (juce::jmax(0.0f, state.driveModAmount) * 0.35f));

        const auto lowX = plot.getX() + (plot.getWidth() * normaliseFrequency(lowState.cutoffHz));
        const auto highX = plot.getX() + (plot.getWidth() * normaliseFrequency(highState.cutoffHz));
        auto range = juce::Rectangle<float>(juce::jmin(lowX, highX),
                                            plot.getY() + 1.0f,
                                            juce::jmax(2.0f, std::abs(highX - lowX)),
                                            plot.getHeight() - 2.0f);
        g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.12f));
        g.fillRoundedRectangle(range, 3.0f);
        g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.58f));
        g.strokePath(makeResponsePath(plot, lowState), juce::PathStrokeType(1.0f));
        g.strokePath(makeResponsePath(plot, highState), juce::PathStrokeType(1.0f));
    }

    g.setColour(accent.withAlpha(0.58f));
    g.drawLine(cutoffX, plot.getY(), cutoffX, plot.getBottom(), 1.2f);

    if (std::abs(state.envAmount) > 0.01f)
    {
        const auto envCutoff = state.cutoffHz * std::pow(2.0f, state.envAmount * 2.0f);
        const auto envX = plot.getX() + (plot.getWidth() * normaliseFrequency(envCutoff));
        const auto envColour = state.envAmount >= 0.0f ? juce::Colour(0xff8ee6c9)
                                                       : juce::Colour(0xffff9ab3);
        g.setColour(envColour.withAlpha(0.62f));
        g.drawLine(envX, plot.getY() + 1.0f, envX, plot.getBottom() - 1.0f, 1.0f);
    }

    auto responsePath = makeResponsePath(plot, state);
    auto fillPath = responsePath;
    fillPath.lineTo(plot.getRight(), plot.getBottom());
    fillPath.lineTo(plot.getX(), plot.getBottom());
    fillPath.closeSubPath();

    const auto driveGlow = smoothStep(state.drive);
    g.setColour(accent.withAlpha(0.12f + (driveGlow * 0.12f)));
    g.fillPath(fillPath);
    g.setColour(accent.withAlpha(0.92f));
    g.strokePath(responsePath, juce::PathStrokeType(1.7f + (driveGlow * 0.5f)));

    if (state.modRouteCount > 0)
    {
        auto modBadge = juce::Rectangle<float>(plot.getRight() - 106.0f,
                                               plot.getY(),
                                               106.0f,
                                               13.0f);
        const auto badgeColour = state.cutoffModAmount >= 0.0f ? juce::Colour(0xff7bb7ff)
                                                               : juce::Colour(0xffff9ab3);
        g.setColour(juce::Colour(0xff11191d).withAlpha(0.92f));
        g.fillRoundedRectangle(modBadge, 3.0f);
        g.setColour(badgeColour.withAlpha(0.72f));
        g.drawRoundedRectangle(modBadge, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffd7e3e6));
        g.drawText("MOD " + modulationText(state.cutoffModAmount)
                       + (state.modRouteCount > 1 ? " " + juce::String(state.modRouteCount) + "R" : ""),
                   modBadge.reduced(4.0f, 0.0f),
                   juce::Justification::centred);

        auto detail = bounds.reduced(8.0f, 4.0f).removeFromBottom(10.0f);
        g.setFont(juce::FontOptions(8.0f));
        g.setColour(juce::Colour(0xff8a989e));
        const auto sourceText = state.modSourceSummary.isNotEmpty() ? state.modSourceSummary : "Routes";
        const auto detailText = "CUT " + modulationText(state.cutoffModAmount)
            + "  RES " + modulationText(state.resonanceModAmount)
            + "  ENV " + modulationText(state.envModAmount)
            + "  DRV " + modulationText(state.driveModAmount)
            + "  " + sourceText;
        g.drawFittedText(detailText, detail.toNearestInt(), juce::Justification::centredLeft, 1, 0.74f);
    }

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffd7e3e6));
    const auto frequencyText = state.cutoffHz >= 1000.0f
        ? juce::String(state.cutoffHz / 1000.0f, 1) + "K"
        : juce::String(juce::roundToInt(state.cutoffHz));
    g.drawText(frequencyText + "Hz",
               juce::Rectangle<float>(cutoffX - 24.0f, plot.getY() + 1.0f, 48.0f, 11.0f),
               juce::Justification::centred);
}

float FilterResponseDisplay::normaliseFrequency(float frequencyHz)
{
    frequencyHz = juce::jlimit(minFrequencyHz, maxFrequencyHz, frequencyHz);
    return juce::jlimit(0.0f, 1.0f, std::log(frequencyHz / minFrequencyHz) / std::log(frequencyRangeRatio));
}

float FilterResponseDisplay::frequencyForX(float xNormalised)
{
    xNormalised = juce::jlimit(0.0f, 1.0f, xNormalised);
    return minFrequencyHz * std::pow(frequencyRangeRatio, xNormalised);
}

float FilterResponseDisplay::responseAt(const State& responseState, float frequencyHz)
{
    const auto cutoff = juce::jlimit(minFrequencyHz, maxFrequencyHz, responseState.cutoffHz);
    const auto resonanceAmount = resonanceNormalised(responseState.resonance);
    const auto slopePower = responseState.slope == 0 ? 2.4f : 4.8f;
    auto base = 0.0f;

    switch (juce::jlimit(0, 2, responseState.mode))
    {
        case 1:
        {
            const auto distance = std::abs(log2Ratio(frequencyHz, cutoff));
            const auto bandwidth = 0.78f - (resonanceAmount * 0.42f);
            base = std::exp(-((distance * distance) / juce::jmax(0.08f, bandwidth * bandwidth)));
            break;
        }

        case 2:
            base = 1.0f / (1.0f + std::pow(cutoff / juce::jmax(frequencyHz, 0.001f), slopePower));
            break;

        case 0:
        default:
            base = 1.0f / (1.0f + std::pow(frequencyHz / cutoff, slopePower));
            break;
    }

    const auto octaveDistance = log2Ratio(frequencyHz, cutoff);
    const auto resonanceWidth = 0.38f - (resonanceAmount * 0.24f);
    const auto resonancePeak = std::exp(-((octaveDistance * octaveDistance)
                                         / (2.0f * resonanceWidth * resonanceWidth)))
        * resonanceAmount
        * (responseState.mode == 1 ? 0.45f : 0.34f);

    const auto driveLift = smoothStep(responseState.drive) * 0.05f;
    return juce::jlimit(0.0f, responseCeiling, base + resonancePeak + driveLift);
}

juce::Path FilterResponseDisplay::makeResponsePath(juce::Rectangle<float> plotBounds, const State& responseState)
{
    juce::Path path;
    constexpr auto points = 112;

    for (auto index = 0; index < points; ++index)
    {
        const auto xNormalised = static_cast<float>(index) / static_cast<float>(points - 1);
        const auto frequencyHz = frequencyForX(xNormalised);
        const auto response = responseAt(responseState, frequencyHz) / responseCeiling;
        const auto x = plotBounds.getX() + (plotBounds.getWidth() * xNormalised);
        const auto y = plotBounds.getBottom() - (plotBounds.getHeight() * response);

        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

juce::String FilterResponseDisplay::modulationText(float amount)
{
    amount = juce::jlimit(-1.0f, 1.0f, amount);
    return (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%";
}

juce::String FilterResponseDisplay::modeText(int mode)
{
    switch (juce::jlimit(0, 2, mode))
    {
        case 1: return "BP";
        case 2: return "HP";
        case 0:
        default: return "LP";
    }
}

juce::String FilterResponseDisplay::characterText(int character)
{
    switch (juce::jlimit(0, 3, character))
    {
        case 1: return "WARM";
        case 2: return "ACID";
        case 3: return "DIRTY";
        case 0:
        default: return "CLEAN";
    }
}

juce::String FilterResponseDisplay::slopeText(int slope)
{
    return juce::jlimit(0, 1, slope) == 0 ? "12" : "24";
}

juce::Colour FilterResponseDisplay::characterColour(int character)
{
    switch (juce::jlimit(0, 3, character))
    {
        case 1: return juce::Colour(0xffffc29a);
        case 2: return juce::Colour(0xffffdf70);
        case 3: return juce::Colour(0xffff8fa3);
        case 0:
        default: return juce::Colour(0xff8ee6c9);
    }
}
}
