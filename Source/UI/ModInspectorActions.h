#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class ModInspectorActions final : public juce::Component
{
public:
    ModInspectorActions();

    std::function<void()> onAddClicked;
    std::function<void()> onClearClicked;

    static constexpr int preferredWidth = 124;

    void setClearEnabled(bool shouldBeEnabled);
    void resized() override;

private:
    juce::TextButton addButton { "Add" };
    juce::TextButton clearButton { "Clear" };

    void configureButton(juce::TextButton& button,
                         std::function<void()> ModInspectorActions::* callback,
                         const juce::String& tooltip);
};
}
