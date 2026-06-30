#include "../Source/UI/Theme.h"

#include <cmath>
#include <iostream>

namespace
{
double linearChannel(float channel)
{
    if (channel <= 0.03928f)
        return static_cast<double>(channel) / 12.92;

    return std::pow((static_cast<double>(channel) + 0.055) / 1.055, 2.4);
}

double relativeLuminance(juce::Colour colour)
{
    return 0.2126 * linearChannel(colour.getFloatRed())
        + 0.7152 * linearChannel(colour.getFloatGreen())
        + 0.0722 * linearChannel(colour.getFloatBlue());
}

double contrastRatio(juce::Colour foreground, juce::Colour background)
{
    const auto first = relativeLuminance(foreground);
    const auto second = relativeLuminance(background);
    const auto light = std::max(first, second);
    const auto dark = std::min(first, second);
    return (light + 0.05) / (dark + 0.05);
}

bool requireContrast(const UI::Theme& theme,
                     const char* foregroundName,
                     juce::Colour foreground,
                     const char* backgroundName,
                     juce::Colour background,
                     double minimumRatio)
{
    const auto ratio = contrastRatio(foreground, background);
    if (ratio >= minimumRatio)
        return true;

    std::cerr << theme.name << ": " << foregroundName << " on " << backgroundName
              << " contrast " << ratio << " is below " << minimumRatio << "\n";
    return false;
}
}

int main()
{
    auto failed = false;

    for (const auto id : { UI::ThemeId::darkClub, UI::ThemeId::analyzer, UI::ThemeId::warmStudio })
    {
        const auto& theme = UI::themeFor(id);
        failed = ! requireContrast(theme, "text", theme.text, "background", theme.background, 7.0) || failed;
        failed = ! requireContrast(theme, "text", theme.text, "panel", theme.panel, 7.0) || failed;
        failed = ! requireContrast(theme, "text", theme.text, "panelAlt", theme.panelAlt, 7.0) || failed;
        failed = ! requireContrast(theme, "textMuted", theme.textMuted, "panelAlt", theme.panelAlt, 4.5) || failed;
        failed = ! requireContrast(theme, "accent", theme.accent, "background", theme.background, 4.5) || failed;
        failed = ! requireContrast(theme, "accentBright", theme.accentBright, "panelAlt", theme.panelAlt, 4.5) || failed;
        failed = ! requireContrast(theme, "warning", theme.warning, "panelAlt", theme.panelAlt, 4.5) || failed;
        failed = ! requireContrast(theme, "danger", theme.danger, "panelAlt", theme.panelAlt, 4.5) || failed;
        failed = ! requireContrast(theme, "keyboardText", theme.keyboardText, "keyboardWhite", theme.keyboardWhite, 4.5) || failed;
    }

    if (failed)
        return 1;

    return 0;
}
