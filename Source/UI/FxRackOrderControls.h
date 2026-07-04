#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class FxRackOrderControls final : public juce::Component
{
public:
    FxRackOrderControls();

    std::function<void(int)> onMoveRequested;
    std::function<void()> onResetRequested;
    std::function<void()> onRemoveRequested;

    void setButtonStates(bool canMoveUp, bool canMoveDown, bool canRemove);
    void resized() override;

private:
    juce::TextButton moveUpButton { "Up" };
    juce::TextButton moveDownButton { "Down" };
    juce::TextButton resetButton { "Reset" };
    juce::TextButton removeButton { "Remove" };
};
}
