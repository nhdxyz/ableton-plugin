#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class MacroAssignmentActions final : public juce::Component
{
public:
    MacroAssignmentActions();

    std::function<void()> onAddClicked;
    std::function<void()> onReplaceClicked;
    std::function<void()> onClearClicked;

    static constexpr int preferredWidth = 208;

    void setState(bool selectedRouteExists, bool hasRoutesForSelectedMacro);
    void resized() override;

private:
    juce::TextButton addButton { "Add" };
    juce::TextButton replaceButton { "Replace" };
    juce::TextButton clearButton { "Clear" };

    void configureButton(juce::TextButton& button,
                         std::function<void()> MacroAssignmentActions::* callback,
                         const juce::String& tooltip);
};
}
