#include "SequencerEnabledButton.h"

namespace UI
{
SequencerEnabledButton::SequencerEnabledButton(juce::AudioProcessorValueTreeState& valueTreeState)
    : attachment(valueTreeState, Parameters::ID::sequencerEnabled, *this)
{
    setComponentID("SequencerEnabledButton");
    setButtonText("On");
    setTooltip("Enable or bypass the sequencer");
}
}
