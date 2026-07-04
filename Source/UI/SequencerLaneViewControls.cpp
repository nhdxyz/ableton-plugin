#include "SequencerLaneViewControls.h"

namespace UI
{
SequencerLaneViewControls::SequencerLaneViewControls()
{
    setComponentID("SequencerLaneViewControls");

    laneViewBox.addItem("All Lanes", 1);
    laneViewBox.addItem("Groove", 2);
    laneViewBox.addItem("Dynamics", 3);
    laneViewBox.addItem("Ratchets", 4);
    laneViewBox.addItem("Lock/Slide", 5);
    laneViewBox.setSelectedId(1, juce::dontSendNotification);
    laneViewBox.setTextWhenNothingSelected("Lane View");
    laneViewBox.setTooltip("Choose which step lanes are emphasized in the SEQ piano roll");
    laneViewBox.onChange = [this]
    {
        if (onLaneViewChanged != nullptr)
            onLaneViewChanged(juce::jlimit(0, 4, laneViewBox.getSelectedId() - 1));
    };
    addAndMakeVisible(laneViewBox);
}

void SequencerLaneViewControls::resized()
{
    laneViewBox.setBounds(getLocalBounds().reduced(4));
}
}
