#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>

namespace UI
{
class PageButtonStrip final : public juce::Component
{
public:
    enum class Orientation
    {
        horizontal,
        vertical
    };

    struct Item
    {
        juce::String label;
        juce::String tooltip;
    };

    void setItems(std::initializer_list<Item> newItems);
    void setOrientation(Orientation newOrientation);
    void setActiveIndex(size_t index);
    void resized() override;

    std::function<void(size_t)> onItemSelected;

private:
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    Orientation orientation = Orientation::horizontal;
    size_t activeIndex = 0;
};
}
