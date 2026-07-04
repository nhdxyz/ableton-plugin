#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SampleChopHeader final : public juce::Component
{
public:
    SampleChopHeader();

    std::function<void()> onExpandClicked;

    void applyTheme(const Theme& theme);
    void resized() override;

private:
    juce::Label titleLabel;
    juce::TextButton expandButton { ">" };
};
}
