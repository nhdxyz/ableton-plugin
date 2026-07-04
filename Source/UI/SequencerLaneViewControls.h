#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SequencerLaneViewControls final : public juce::Component
{
public:
    static constexpr int preferredHeight = 34;

    SequencerLaneViewControls();

    std::function<void(int laneViewMode)> onLaneViewChanged;

    void resized() override;

private:
    juce::ComboBox laneViewBox;
};
}
