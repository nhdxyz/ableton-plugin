#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class PresetCompareActions final : public juce::Component
{
public:
    PresetCompareActions();

    std::function<void()> onCompareClicked;
    std::function<void()> onRevertClicked;

    static constexpr int preferredWidth = 164;

    void setState(bool hasSnapshots, bool showingLoaded, const juce::String& presetName);
    void resized() override;

private:
    juce::TextButton compareButton { "Before" };
    juce::TextButton revertButton { "Revert" };

    void configureButton(juce::TextButton& button,
                         std::function<void()> PresetCompareActions::* callback,
                         const juce::String& tooltip);
};
}
