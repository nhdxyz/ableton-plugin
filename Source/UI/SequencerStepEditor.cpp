#include "SequencerStepEditor.h"

namespace UI
{
SequencerStepEditor::SequencerStepEditor()
{
    statusLabel.setText("Select a step", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    statusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    statusLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff31434a));
    statusLabel.setTooltip("Focused editor for the selected piano-roll step");
    addAndMakeVisible(statusLabel);

    const std::array<juce::String, buttonCount> labels {
        "Note -",
        "Note +",
        "Oct -",
        "Oct +",
        "Vel -",
        "Vel +",
        "Prob -",
        "Prob +",
        "Len -",
        "Len +",
        "Rat",
        "Slide",
        "Lock"
    };

    for (size_t index = 0; index < buttons.size(); ++index)
    {
        auto& button = buttons[index];
        button.setButtonText(labels[index]);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.setTooltip("Edit the selected sequencer step: " + labels[index]);
        button.onClick = [this, index]
        {
            if (onAction != nullptr)
                onAction(actionForIndex(index));
        };
        addAndMakeVisible(button);
    }

    setNoSelection();
}

void SequencerStepEditor::setNoSelection()
{
    statusLabel.setText("Select a step", juce::dontSendNotification);
    statusLabel.setTooltip("Click a piano-roll step or lane cell, then edit it here");

    const std::array<juce::String, buttonCount> labels {
        "Note -",
        "Note +",
        "Oct -",
        "Oct +",
        "Vel -",
        "Vel +",
        "Prob -",
        "Prob +",
        "Len -",
        "Len +",
        "Rat",
        "Slide",
        "Lock"
    };

    for (size_t index = 0; index < buttons.size(); ++index)
    {
        buttons[index].setButtonText(labels[index]);
        buttons[index].setEnabled(false);
    }
}

void SequencerStepEditor::setSelectedStep(int stepIndex,
                                          const juce::String& noteName,
                                          bool stepEnabled,
                                          int velocityPercent,
                                          int probabilityPercent,
                                          int lengthPercent,
                                          int ratchet,
                                          bool slide,
                                          int lockPercent)
{
    const auto label = "S" + juce::String(stepIndex + 1)
        + (stepEnabled ? " " + noteName : " empty")
        + " | V" + juce::String(velocityPercent)
        + " P" + juce::String(probabilityPercent)
        + " L" + juce::String(lengthPercent);

    statusLabel.setText(label, juce::dontSendNotification);
    statusLabel.setTooltip("Selected step editor: " + label
                           + " | Ratchet " + juce::String(ratchet) + "x"
                           + (slide ? " | Slide" : "")
                           + " | Lock " + juce::String(lockPercent) + "%");

    for (auto& button : buttons)
        button.setEnabled(true);

    buttons[10].setButtonText("Rat " + juce::String(ratchet) + "x");
    buttons[11].setButtonText(slide ? "Slide On" : "Slide");
    buttons[12].setButtonText(lockPercent > 1 ? "Lock " + juce::String(lockPercent) : "Lock");
}

void SequencerStepEditor::resized()
{
    auto area = getLocalBounds();
    statusLabel.setBounds(area.removeFromTop(24).reduced(4, 2));

    auto pitchRow = area.removeFromTop(26);
    for (auto index = 0; index < 4; ++index)
        buttons[static_cast<size_t>(index)].setBounds(pitchRow.removeFromLeft(pitchRow.getWidth() / (4 - index)).reduced(3, 3));

    auto valueRow = area.removeFromTop(26);
    for (auto index = 4; index < 10; ++index)
        buttons[static_cast<size_t>(index)].setBounds(valueRow.removeFromLeft(valueRow.getWidth() / (10 - index)).reduced(3, 3));

    auto flagRow = area.removeFromTop(26);
    for (auto index = 10; index < 13; ++index)
        buttons[static_cast<size_t>(index)].setBounds(flagRow.removeFromLeft(flagRow.getWidth() / (13 - index)).reduced(3, 3));
}

SequencerStepEditor::Action SequencerStepEditor::actionForIndex(size_t index) noexcept
{
    switch (index)
    {
        case 0: return Action::noteDown;
        case 1: return Action::noteUp;
        case 2: return Action::octaveDown;
        case 3: return Action::octaveUp;
        case 4: return Action::velocityDown;
        case 5: return Action::velocityUp;
        case 6: return Action::probabilityDown;
        case 7: return Action::probabilityUp;
        case 8: return Action::lengthDown;
        case 9: return Action::lengthUp;
        case 10: return Action::ratchet;
        case 11: return Action::slide;
        case 12: return Action::lock;
        default: return Action::noteDown;
    }
}
}
