#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
enum class ThemeId
{
    darkClub = 0,
    analyzer,
    warmStudio
};

struct Theme
{
    ThemeId id = ThemeId::darkClub;
    juce::String name;

    juce::Colour background;
    juce::Colour panel;
    juce::Colour panelAlt;
    juce::Colour panelRaised;
    juce::Colour outline;
    juce::Colour outlineStrong;

    juce::Colour text;
    juce::Colour textMuted;
    juce::Colour textDim;
    juce::Colour accent;
    juce::Colour accentBright;
    juce::Colour accentSecondary;
    juce::Colour warning;
    juce::Colour danger;

    juce::Colour button;
    juce::Colour buttonHover;
    juce::Colour buttonDown;
    juce::Colour buttonOn;
    juce::Colour field;
    juce::Colour fieldFocus;
    juce::Colour keyboardWhite;
    juce::Colour keyboardBlack;
    juce::Colour keyboardText;
};

const Theme& themeFor(ThemeId id);
}
