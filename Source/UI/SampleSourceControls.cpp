#include "SampleSourceControls.h"

namespace UI
{
SampleSourceControls::SampleSourceControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SampleSourceControls");

    enabledButton.setButtonText("On");
    addAndMakeVisible(enabledButton);
    enabledAttachment = std::make_unique<ButtonAttachment>(valueTreeState,
                                                          Parameters::ID::sampleEnabled,
                                                          enabledButton);

    reverseButton.setButtonText("Rev");
    addAndMakeVisible(reverseButton);
    reverseAttachment = std::make_unique<ButtonAttachment>(valueTreeState,
                                                          Parameters::ID::sampleReverse,
                                                          reverseButton);
}

void SampleSourceControls::resized()
{
    auto area = getLocalBounds();
    enabledButton.setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(3, 4));
    reverseButton.setBounds(area.reduced(3, 4));
}
}
