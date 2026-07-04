#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SampleRecipeActions final : public juce::Component
{
public:
    SampleRecipeActions();

    std::function<void()> onRandomCutClicked;
    std::function<void()> onUkgChopClicked;

    void resized() override;

private:
    juce::TextButton randomCutButton { "Rand Cut" };
    juce::TextButton ukgChopButton { "UKG Chop" };
};
}
