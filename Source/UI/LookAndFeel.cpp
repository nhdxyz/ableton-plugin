#include "LookAndFeel.h"

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
                            .reduced(5.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto knobBounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);

    const auto isActive = slider.isMouseOverOrDragging();

    g.setColour(isActive ? juce::Colour(0xff162225) : juce::Colour(0xff111619));
    g.fillEllipse(knobBounds);

    g.setColour(isActive ? juce::Colour(0xff5f736f) : juce::Colour(0xff303c42));
    g.drawEllipse(knobBounds, isActive ? 2.0f : 1.5f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                      rotaryStartAngle, angle, true);
    g.setColour(isActive ? juce::Colour(0xffa8ffe3) : juce::Colour(0xff8ee6c9));
    g.strokePath(arc, juce::PathStrokeType(isActive ? 3.8f : 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRectangle(-1.4f, -radius + 9.0f, 2.8f, radius * 0.45f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.fillPath(pointer);
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
