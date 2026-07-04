#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class PresetQuickFilterBar final : public juce::Component
{
public:
    static constexpr size_t filterCount = 10;

    PresetQuickFilterBar();

    std::function<void(size_t)> onFilterSelected;

    void setFilterActive(size_t index, bool active);
    void resized() override;

private:
    std::array<juce::TextButton, filterCount> buttons;

    static juce::String labelForIndex(size_t index);
    void placeFilterRow(size_t firstIndex, size_t endIndex, juce::Rectangle<int> row);
};
}
