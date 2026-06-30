#include "LookAndFeel.h"

#include <cmath>

namespace
{
juce::String shortModSourceName(juce::String sourceName)
{
    sourceName = sourceName.trim();
    if (sourceName == "LFO 1") return "L1";
    if (sourceName == "LFO 2") return "L2";
    if (sourceName == "Mod Env 1") return "ENV";
    if (sourceName == "Velocity") return "VEL";
    if (sourceName == "S&H") return "S&H";
    if (sourceName == "Smooth") return "SM";
    if (sourceName == "Chaos") return "CH";

    juce::String compact;
    for (auto index = 0; index < sourceName.length() && compact.length() < 3; ++index)
    {
        const auto character = sourceName[index];
        if (juce::CharacterFunctions::isLetterOrDigit(character))
            compact += juce::String::charToString(juce::CharacterFunctions::toUpperCase(character));
    }

    return compact.isNotEmpty() ? compact : juce::String("MOD");
}

juce::String firstModSourceName(const juce::Slider& slider)
{
    const auto sourceSummary = slider.getProperties().getWithDefault("modSourceSummary", {}).toString();
    juce::StringArray sources;
    sources.addTokens(sourceSummary, ",", "");
    sources.trim();
    sources.removeEmptyStrings();

    return sources.isEmpty() ? juce::String {} : sources[0];
}

juce::Colour modulationSourceColour(const juce::String& sourceName)
{
    if (sourceName == "LFO 1") return juce::Colour(0xff8ee6c9);
    if (sourceName == "LFO 2") return juce::Colour(0xff7fb7ff);
    if (sourceName == "Mod Env 1") return juce::Colour(0xffffd27a);
    if (sourceName == "Velocity") return juce::Colour(0xff9be58f);
    if (sourceName == "Tone") return juce::Colour(0xff8ee6c9);
    if (sourceName == "Dirt") return juce::Colour(0xffff9f66);
    if (sourceName == "Motion") return juce::Colour(0xff7fb7ff);
    if (sourceName == "Space") return juce::Colour(0xffb8a1ff);
    if (sourceName == "Weight") return juce::Colour(0xffa7d38b);
    if (sourceName == "Bounce") return juce::Colour(0xffffd27a);
    if (sourceName == "Warp") return juce::Colour(0xffff7d9e);
    if (sourceName == "Throw") return juce::Colour(0xff9ce7ff);
    if (sourceName == "S&H") return juce::Colour(0xffc0de73);
    if (sourceName == "Smooth") return juce::Colour(0xff76d7c4);
    if (sourceName == "Chaos") return juce::Colour(0xffff7a66);

    return juce::Colour(0xff8ee6c9);
}

juce::Colour modulationAccentColour(const juce::Slider& slider, float modulationAmount)
{
    auto accent = modulationSourceColour(firstModSourceName(slider));
    if (modulationAmount < 0.0f)
        accent = accent.interpolatedWith(juce::Colour(0xffff7a66), 0.46f);

    return accent;
}

juce::String modulationBadgeText(const juce::Slider& slider, float modulationAmount, int routeCount)
{
    const auto sourceSummary = slider.getProperties().getWithDefault("modSourceSummary", {}).toString();
    juce::StringArray sources;
    sources.addTokens(sourceSummary, ",", "");
    sources.trim();
    sources.removeEmptyStrings();

    auto sourceText = routeCount > 0 ? juce::String("M") + juce::String(routeCount) : juce::String("M");
    if (! sources.isEmpty())
    {
        sourceText = shortModSourceName(sources[0]);
        if (routeCount > 1)
            sourceText += "+" + juce::String(routeCount - 1);
    }

    const auto percent = juce::roundToInt(modulationAmount * 100.0f);
    return sourceText + " " + (percent >= 0 ? "+" : "") + juce::String(percent);
}
}

namespace UI
{
LookAndFeel::LookAndFeel()
{
    setTheme(themeFor(ThemeId::darkClub));
}

void LookAndFeel::setTheme(const Theme& theme)
{
    currentTheme = theme;
    setColour(juce::Slider::thumbColourId, currentTheme.accent);
    setColour(juce::Slider::rotarySliderFillColourId, currentTheme.accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, currentTheme.outline);
    setColour(juce::ComboBox::backgroundColourId, currentTheme.panelRaised);
    setColour(juce::ComboBox::outlineColourId, currentTheme.outlineStrong);
    setColour(juce::TextButton::buttonColourId, currentTheme.button);
    setColour(juce::TextButton::buttonOnColourId, currentTheme.buttonOn);
    setColour(juce::TextEditor::backgroundColourId, currentTheme.field);
    setColour(juce::TextEditor::outlineColourId, currentTheme.outlineStrong);
    setColour(juce::TextEditor::focusedOutlineColourId, currentTheme.fieldFocus);
    setColour(juce::TextEditor::textColourId, currentTheme.text);
    setColour(juce::Label::textColourId, currentTheme.text);
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
    const auto& theme = currentTheme;

    g.setColour(isActive ? theme.panelRaised : theme.panelAlt);
    g.fillRoundedRectangle(cellBounds, 6.0f);
    g.setColour(isActive ? theme.outlineStrong : theme.outline);
    g.drawRoundedRectangle(cellBounds, 6.0f, isActive ? 1.6f : 1.0f);

    g.setColour(theme.background.darker(0.35f));
    g.fillEllipse(knobBounds.translated(0.0f, 1.5f));
    g.setColour(isActive ? theme.panelRaised : theme.panel);
    g.fillEllipse(knobBounds);

    g.setColour(isActive ? theme.textDim.brighter(0.18f) : theme.outlineStrong);
    g.drawEllipse(knobBounds, isActive ? 2.4f : 1.8f);

    juce::Path rail;
    rail.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                       rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(theme.outline);
    g.strokePath(rail, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(isActive ? theme.accentBright : theme.accent);
    g.strokePath(valueArc, juce::PathStrokeType(isActive ? 4.8f : 3.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.9f, -radius + 8.0f, 3.8f, radius * 0.52f, 1.4f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.fillPath(pointer);

    g.setColour(isActive ? theme.text : theme.textMuted);
    g.fillEllipse(juce::Rectangle<float>(radius * 0.22f, radius * 0.22f).withCentre(centre));

    g.setColour(isActive ? theme.accent.withAlpha(0.20f) : theme.panelAlt.withAlpha(0.0f));
    g.drawEllipse(knobBounds.expanded(4.0f), 1.0f);

    if (modulationDepth > 0.001f)
    {
        const auto ringRadius = radius + 2.5f;
        const auto ringColour = modulationAccentColour(slider, modulationAmount);
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
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            const auto badgeText = modulationBadgeText(slider, modulationAmount, modulationRouteCount);
            auto badge = cellBounds.withSizeKeepingCentre(56.0f, 14.0f);
            badge.setY(cellBounds.getY() + 4.0f);
            badge.setX(cellBounds.getRight() - badge.getWidth() - 5.0f);

            g.setColour(ringColour.withAlpha(0.18f));
            g.fillRoundedRectangle(badge, 4.0f);
            g.setColour(ringColour.withAlpha(0.72f));
            g.drawRoundedRectangle(badge, 4.0f, 1.0f);

            g.setColour(theme.text);
            g.drawFittedText(badgeText, badge.toNearestInt().reduced(2, 0), juce::Justification::centred, 1);
        }
    }
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPos,
                                   float minSliderPos,
                                   float maxSliderPos,
                                   const juce::Slider::SliderStyle style,
                                   juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    juce::ignoreUnused(minSliderPos, maxSliderPos);

    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height))
                            .reduced(1.0f, 2.0f);
    const auto isActive = slider.isMouseOverOrDragging();
    const auto trackBounds = bounds.withHeight(7.0f).withCentre(bounds.getCentre());
    const auto trackStart = trackBounds.getX();
    const auto trackEnd = trackBounds.getRight();
    const auto currentX = juce::jlimit(trackStart, trackEnd, sliderPos);
    const auto modulationAmount = static_cast<float>(static_cast<double>(slider.getProperties().getWithDefault("modAmount", 0.0)));
    const auto modulationDepth = juce::jlimit(0.0f, 1.0f, std::abs(modulationAmount));
    const auto modulationRouteCount = static_cast<int>(slider.getProperties().getWithDefault("modRouteCount", 0));
    const auto& theme = currentTheme;

    g.setColour(isActive ? theme.panelRaised : theme.panelAlt);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(isActive ? theme.outlineStrong : theme.outline);
    g.drawRoundedRectangle(bounds, 4.0f, isActive ? 1.4f : 1.0f);

    g.setColour(theme.outline);
    g.fillRoundedRectangle(trackBounds, 3.5f);

    if (currentX > trackStart + 0.5f)
    {
        auto valueBounds = trackBounds.withRight(currentX);
        g.setColour(isActive ? theme.accentBright : theme.accent);
        g.fillRoundedRectangle(valueBounds, 3.5f);
    }

    if (modulationDepth > 0.001f)
    {
        const auto ringColour = modulationAccentColour(slider, modulationAmount);
        const auto modulationEnd = juce::jlimit(trackStart,
                                                trackEnd,
                                                currentX + modulationAmount * (trackEnd - trackStart));
        auto modulationBounds = trackBounds.withY(trackBounds.getY() - 5.0f).withHeight(3.0f);
        modulationBounds.setLeft(juce::jmin(currentX, modulationEnd));
        modulationBounds.setRight(juce::jmax(currentX, modulationEnd));

        g.setColour(ringColour.withAlpha(0.22f));
        g.fillRoundedRectangle(trackBounds.expanded(0.0f, 3.5f), 6.0f);
        g.setColour(ringColour);
        g.fillRoundedRectangle(modulationBounds, 1.5f);

        if (modulationRouteCount > 0 && bounds.getWidth() >= 74.0f)
        {
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            const auto badgeText = modulationBadgeText(slider, modulationAmount, modulationRouteCount);
            auto badge = juce::Rectangle<float>(56.0f, 14.0f);
            badge.setRight(bounds.getRight() - 5.0f);
            badge.setY(bounds.getY() + 3.0f);

            g.setColour(ringColour.withAlpha(0.18f));
            g.fillRoundedRectangle(badge, 4.0f);
            g.setColour(ringColour.withAlpha(0.72f));
            g.drawRoundedRectangle(badge, 4.0f, 1.0f);

            g.setColour(theme.text);
            g.drawFittedText(badgeText, badge.toNearestInt().reduced(2, 0), juce::Justification::centred, 1);
        }
    }

    const auto thumbBounds = juce::Rectangle<float>(12.0f, 18.0f).withCentre({ currentX, trackBounds.getCentreY() });
    g.setColour(theme.background.darker(0.35f));
    g.fillRoundedRectangle(thumbBounds.translated(0.0f, 1.0f), 4.0f);
    g.setColour(isActive ? theme.accentBright : theme.textMuted);
    g.fillRoundedRectangle(thumbBounds, 4.0f);
    g.setColour(isActive ? theme.accent : theme.outlineStrong);
    g.drawRoundedRectangle(thumbBounds, 4.0f, 1.0f);
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
    const auto& theme = currentTheme;

    if (isOn)
        fill = theme.buttonOn;
    else if (shouldDrawButtonAsDown)
        fill = theme.buttonDown;
    else if (shouldDrawButtonAsHighlighted)
        fill = theme.buttonHover;

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(isOn ? theme.accent : theme.outlineStrong);
    g.drawRoundedRectangle(bounds, 5.0f, isOn ? 1.4f : 1.0f);
}

void LookAndFeel::drawButtonText(juce::Graphics& g,
                                 juce::TextButton& button,
                                 bool,
                                 bool)
{
    const auto& theme = currentTheme;
    g.setFont(juce::FontOptions(12.0f, button.getToggleState() ? juce::Font::bold : juce::Font::plain));
    g.setColour(button.getToggleState() ? theme.text : theme.textMuted);
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2), juce::Justification::centred, 1);
}
}
