#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SampleFileActions final : public juce::Component
{
public:
    SampleFileActions();

    std::function<void()> onLoadClicked;
    std::function<void()> onClearClicked;

    void resized() override;

private:
    juce::TextButton loadButton { "Load" };
    juce::TextButton clearButton { "Clear" };
};
}
