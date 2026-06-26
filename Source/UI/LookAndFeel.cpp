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
                                   juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height))
                            .reduced(5.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto knobBounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);

    g.setColour(juce::Colour(0xff111619));
    g.fillEllipse(knobBounds);

    g.setColour(juce::Colour(0xff303c42));
    g.drawEllipse(knobBounds, 1.5f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                      rotaryStartAngle, angle, true);
    g.setColour(juce::Colour(0xff8ee6c9));
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRectangle(-1.4f, -radius + 9.0f, 2.8f, radius * 0.45f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.fillPath(pointer);
}
}

