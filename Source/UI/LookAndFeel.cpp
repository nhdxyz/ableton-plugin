#include "LookAndFeel.h"

#include <cmath>

namespace UI
{
LookAndFeel::LookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colour(0xff8ee6c9));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff8ee6c9));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff263035));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff171d20));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff344047));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d272b));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff315b52));
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPos,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height))
                            .reduced(1.5f);
    const auto isActive = slider.isMouseOverOrDragging();
    const auto cellBounds = bounds.reduced(0.5f, 1.5f);
    const auto radius = juce::jmin(cellBounds.getWidth(), cellBounds.getHeight()) * 0.46f;
    const auto centre = cellBounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto knobBounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    const auto modulationAmount = static_cast<float>(static_cast<double>(slider.getProperties().getWithDefault("modAmount", 0.0)));
    const auto modulationDepth = juce::jlimit(0.0f, 1.0f, std::abs(modulationAmount));
    const auto modulationRouteCount = static_cast<int>(slider.getProperties().getWithDefault("modRouteCount", 0));

    g.setColour(isActive ? juce::Colour(0xff172326) : juce::Colour(0xff101619));
    g.fillRoundedRectangle(cellBounds, 6.0f);
    g.setColour(isActive ? juce::Colour(0xff3e5359) : juce::Colour(0xff222d32));
    g.drawRoundedRectangle(cellBounds, 6.0f, isActive ? 1.6f : 1.0f);

    g.setColour(juce::Colour(0xff080c0e));
    g.fillEllipse(knobBounds.translated(0.0f, 1.5f));
    g.setColour(isActive ? juce::Colour(0xff18262a) : juce::Colour(0xff12191c));
    g.fillEllipse(knobBounds);

    g.setColour(isActive ? juce::Colour(0xff6f8984) : juce::Colour(0xff2d3a40));
    g.drawEllipse(knobBounds, isActive ? 2.4f : 1.8f);

    juce::Path rail;
    rail.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                       rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff273238));
    g.strokePath(rail, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(isActive ? juce::Colour(0xffa8ffe3) : juce::Colour(0xff8ee6c9));
    g.strokePath(valueArc, juce::PathStrokeType(isActive ? 4.8f : 3.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.9f, -radius + 8.0f, 3.8f, radius * 0.52f, 1.4f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.fillPath(pointer);

    g.setColour(isActive ? juce::Colour(0xffe9fff7) : juce::Colour(0xffd3e1df));
    g.fillEllipse(juce::Rectangle<float>(radius * 0.22f, radius * 0.22f).withCentre(centre));

    g.setColour(isActive ? juce::Colour(0x338ee6c9) : juce::Colour(0x00101619));
    g.drawEllipse(knobBounds.expanded(4.0f), 1.0f);

    if (modulationDepth > 0.001f)
    {
        const auto ringRadius = radius + 2.5f;
        const auto ringColour = modulationAmount >= 0.0f ? juce::Colour(0xff8ee6c9)
                                                         : juce::Colour(0xffffa36f);
        const auto ringTravel = (rotaryEndAngle - rotaryStartAngle) * modulationDepth;
        const auto ringEnd = juce::jlimit(rotaryStartAngle,
                                          rotaryEndAngle,
                                          angle + (modulationAmount >= 0.0f ? ringTravel : -ringTravel));

        juce::Path modRail;
        modRail.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(ringColour.withAlpha(0.16f));
        g.strokePath(modRail, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path modArc;
        modArc.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                             juce::jmin(angle, ringEnd), juce::jmax(angle, ringEnd), true);
        g.setColour(ringColour);
        g.strokePath(modArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (modulationRouteCount > 0)
        {
            auto badge = cellBounds.withSizeKeepingCentre(42.0f, 14.0f);
            badge.setY(cellBounds.getY() + 4.0f);
            badge.setX(cellBounds.getRight() - badge.getWidth() - 5.0f);

            g.setColour(ringColour.withAlpha(0.18f));
            g.fillRoundedRectangle(badge, 4.0f);
            g.setColour(ringColour.withAlpha(0.72f));
            g.drawRoundedRectangle(badge, 4.0f, 1.0f);

            const auto percent = juce::roundToInt(modulationAmount * 100.0f);
            auto badgeText = "M" + juce::String(modulationRouteCount) + " "
                + (percent >= 0 ? "+" : "") + juce::String(percent);
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            g.setColour(juce::Colour(0xffedf7f4));
            g.drawFittedText(badgeText, badge.toNearestInt().reduced(2, 0), juce::Justification::centred, 1);
        }
    }
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g,
                                       juce::Button& button,
                                       const juce::Colour& backgroundColour,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto isOn = button.getToggleState();
    auto fill = backgroundColour;

    if (isOn)
        fill = juce::Colour(0xff315b52);
    else if (shouldDrawButtonAsDown)
        fill = juce::Colour(0xff26373d);
    else if (shouldDrawButtonAsHighlighted)
        fill = juce::Colour(0xff243136);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(isOn ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff39484e));
    g.drawRoundedRectangle(bounds, 5.0f, isOn ? 1.4f : 1.0f);
}

void LookAndFeel::drawButtonText(juce::Graphics& g,
                                 juce::TextButton& button,
                                 bool,
                                 bool)
{
    g.setFont(juce::FontOptions(12.0f, button.getToggleState() ? juce::Font::bold : juce::Font::plain));
    g.setColour(button.getToggleState() ? juce::Colour(0xffedf7f4) : juce::Colour(0xffb7c5c7));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2), juce::Justification::centred, 1);
}
}
