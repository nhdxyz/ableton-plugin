#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class FxRackRow final : public juce::Button
{
public:
    explicit FxRackRow(const juce::String& name = {});

    void setState(const juce::String& name,
                  const juce::String& summary,
                  int orderIndex,
                  bool enabled,
                  bool selected,
                  bool pinned);

    void paintButton(juce::Graphics& g,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

private:
    juce::String moduleName;
    juce::String moduleSummary;
    int moduleOrderIndex = 0;
    bool moduleEnabled = false;
    bool moduleSelected = false;
    bool modulePinned = false;
};
}
