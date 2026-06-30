#include "Theme.h"

#include <array>

namespace UI
{
namespace
{
const std::array<Theme, 3> themes {
    Theme {
        ThemeId::darkClub,
        "Dark Club",
        juce::Colour(0xff0d1113),
        juce::Colour(0xff141a1d),
        juce::Colour(0xff101619),
        juce::Colour(0xff172326),
        juce::Colour(0xff293339),
        juce::Colour(0xff3e5359),
        juce::Colour(0xffdce7e4),
        juce::Colour(0xffa8b6b8),
        juce::Colour(0xff617078),
        juce::Colour(0xff8ee6c9),
        juce::Colour(0xffa8ffe3),
        juce::Colour(0xff7bb7ff),
        juce::Colour(0xffffd27a),
        juce::Colour(0xffff6b5e),
        juce::Colour(0xff1d272b),
        juce::Colour(0xff243136),
        juce::Colour(0xff26373d),
        juce::Colour(0xff315b52),
        juce::Colour(0xff101619),
        juce::Colour(0xff8ee6c9),
        juce::Colour(0xffd9e3df),
        juce::Colour(0xff151b1f),
        juce::Colour(0xff253037)
    },
    Theme {
        ThemeId::analyzer,
        "Analyzer",
        juce::Colour(0xff090f15),
        juce::Colour(0xff111922),
        juce::Colour(0xff0c141c),
        juce::Colour(0xff172330),
        juce::Colour(0xff273746),
        juce::Colour(0xff3c5365),
        juce::Colour(0xffe3edf2),
        juce::Colour(0xffadbac2),
        juce::Colour(0xff667783),
        juce::Colour(0xff9be7ff),
        juce::Colour(0xffc0f2ff),
        juce::Colour(0xffb7a4ff),
        juce::Colour(0xffffd27a),
        juce::Colour(0xffff6f75),
        juce::Colour(0xff1a2834),
        juce::Colour(0xff243646),
        juce::Colour(0xff2d4354),
        juce::Colour(0xff2d6474),
        juce::Colour(0xff0c141c),
        juce::Colour(0xff9be7ff),
        juce::Colour(0xffdce8ee),
        juce::Colour(0xff111924),
        juce::Colour(0xff233240)
    },
    Theme {
        ThemeId::warmStudio,
        "Warm Studio",
        juce::Colour(0xff11100f),
        juce::Colour(0xff1a1816),
        juce::Colour(0xff151311),
        juce::Colour(0xff231f1b),
        juce::Colour(0xff352f2a),
        juce::Colour(0xff51483f),
        juce::Colour(0xffefe5da),
        juce::Colour(0xffc8b9aa),
        juce::Colour(0xff7b6e64),
        juce::Colour(0xffc6f0c2),
        juce::Colour(0xffe2ffd9),
        juce::Colour(0xffffb479),
        juce::Colour(0xffffd06f),
        juce::Colour(0xffff7468),
        juce::Colour(0xff2c2520),
        juce::Colour(0xff3a3029),
        juce::Colour(0xff44372f),
        juce::Colour(0xff4d5b3e),
        juce::Colour(0xff151311),
        juce::Colour(0xffc6f0c2),
        juce::Colour(0xffeaded2),
        juce::Colour(0xff171311),
        juce::Colour(0xff352b25)
    }
};
}

const Theme& themeFor(ThemeId id)
{
    for (const auto& theme : themes)
        if (theme.id == id)
            return theme;

    return themes.front();
}
}
