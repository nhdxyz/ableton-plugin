#include "SequencerRootControls.h"

namespace UI
{
SequencerRootControls::SequencerRootControls()
{
    setComponentID("SequencerRootControls");

    rootDownButton.setTooltip("Move sequencer root note down one semitone");
    rootDownButton.setWantsKeyboardFocus(false);
    rootDownButton.setMouseClickGrabsKeyboardFocus(false);
    rootDownButton.onClick = [this]
    {
        if (onStepRoot != nullptr)
            onStepRoot(-1);
    };
    addAndMakeVisible(rootDownButton);

    rootUpButton.setTooltip("Move sequencer root note up one semitone");
    rootUpButton.setWantsKeyboardFocus(false);
    rootUpButton.setMouseClickGrabsKeyboardFocus(false);
    rootUpButton.onClick = [this]
    {
        if (onStepRoot != nullptr)
            onStepRoot(1);
    };
    addAndMakeVisible(rootUpButton);

    rootValueLabel.setText("C1", juce::dontSendNotification);
    rootValueLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    rootValueLabel.setJustificationType(juce::Justification::centred);
    rootValueLabel.setTooltip("Sequencer root note");
    addAndMakeVisible(rootValueLabel);

    applyTheme(themeFor(ThemeId::darkClub));
    setRootState(36, 0);
}

void SequencerRootControls::applyTheme(const Theme& theme)
{
    rootValueLabel.setColour(juce::Label::textColourId, theme.text);
    rootValueLabel.setColour(juce::Label::backgroundColourId, theme.field);
    rootValueLabel.setColour(juce::Label::outlineColourId, theme.outline);
}

void SequencerRootControls::setRootState(int root, int octave)
{
    root = juce::jlimit(minRoot, maxRoot, root);
    const auto displayRoot = juce::jlimit(0, 127, root + (octave * 12));
    const auto rootName = juce::MidiMessage::getMidiNoteName(root, true, true, 3);
    const auto displayName = juce::MidiMessage::getMidiNoteName(displayRoot, true, true, 3);
    const auto labelText = octave == 0 ? "Root " + rootName
                                       : "Root " + rootName + " -> " + displayName;

    if (rootValueLabel.getText() != labelText)
        rootValueLabel.setText(labelText, juce::dontSendNotification);

    rootValueLabel.setTooltip("Sequencer root note. The arrow buttons move the root by semitone.");
    rootDownButton.setEnabled(root > minRoot);
    rootUpButton.setEnabled(root < maxRoot);
}

void SequencerRootControls::resized()
{
    auto area = getLocalBounds();
    rootDownButton.setBounds(area.removeFromLeft(38).reduced(4));
    rootUpButton.setBounds(area.removeFromRight(38).reduced(4));
    rootValueLabel.setBounds(area.reduced(4));
}
}
