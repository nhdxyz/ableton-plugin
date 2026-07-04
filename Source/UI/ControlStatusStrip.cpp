#include "ControlStatusStrip.h"

namespace UI
{
ControlStatusStrip::ControlStatusStrip()
{
    headerLabel.setText("CONTROL", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    headerLabel.setMinimumHorizontalScale(0.72f);
    headerLabel.setTooltip("Last changed control, automation ID, and modulation route status");
    addAndMakeVisible(headerLabel);

    statusLabel.setText("Touch a control for value, automation ID, and modulation routes", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(11.0f));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setMinimumHorizontalScale(0.62f);
    statusLabel.setTooltip("Last changed control, automation ID, value, and active modulation routes");
    addAndMakeVisible(statusLabel);

    undoButton.setTooltip("Undo the last captured sound-design edit");
    undoButton.setEnabled(false);
    undoButton.onClick = [this]
    {
        if (onUndoClicked != nullptr)
            onUndoClicked();
    };
    addAndMakeVisible(undoButton);

    redoButton.setTooltip("Redo the last undone sound-design edit");
    redoButton.setEnabled(false);
    redoButton.onClick = [this]
    {
        if (onRedoClicked != nullptr)
            onRedoClicked();
    };
    addAndMakeVisible(redoButton);

    addModButton.setTooltip("Touch a modulatable control, then add the selected MOD source to it");
    addModButton.setEnabled(false);
    addModButton.onClick = [this]
    {
        if (onAddModClicked != nullptr)
            onAddModClicked();
    };
    addAndMakeVisible(addModButton);

    openModButton.setTooltip("Open the MOD panel focused on the selected control");
    openModButton.setEnabled(false);
    openModButton.onClick = [this]
    {
        if (onOpenModClicked != nullptr)
            onOpenModClicked();
    };
    addAndMakeVisible(openModButton);
}

void ControlStatusStrip::resized()
{
    auto area = getLocalBounds();
    headerLabel.setBounds(area.removeFromLeft(72).reduced(2, 0));
    undoButton.setBounds(area.removeFromRight(78).reduced(2, 0));
    redoButton.setBounds(area.removeFromRight(78).reduced(2, 0));
    addModButton.setBounds(area.removeFromRight(58).reduced(2, 0));
    openModButton.setBounds(area.removeFromRight(50).reduced(2, 0));
    statusLabel.setBounds(area.reduced(2, 0));
}

void ControlStatusStrip::applyTheme(const Theme& theme)
{
    headerLabel.setColour(juce::Label::textColourId, theme.accent);
    headerLabel.setColour(juce::Label::backgroundColourId, theme.panel.withAlpha(0.28f));
    headerLabel.setColour(juce::Label::outlineColourId, theme.outline);

    statusLabel.setColour(juce::Label::textColourId, theme.textMuted);
    statusLabel.setColour(juce::Label::backgroundColourId, theme.panelAlt.withAlpha(0.93f));
    statusLabel.setColour(juce::Label::outlineColourId, theme.outline);
}

void ControlStatusStrip::setStatus(const juce::String& text,
                                   const juce::String& tooltip,
                                   juce::Colour textColour,
                                   juce::Colour backgroundColour)
{
    statusLabel.setText(text, juce::dontSendNotification);
    statusLabel.setTooltip(tooltip);
    statusLabel.setColour(juce::Label::textColourId, textColour);
    statusLabel.setColour(juce::Label::backgroundColourId, backgroundColour);
}

void ControlStatusStrip::setModActionState(bool enabled,
                                           const juce::String& addTooltip,
                                           const juce::String& openTooltip)
{
    addModButton.setEnabled(enabled);
    openModButton.setEnabled(enabled);
    addModButton.setTooltip(addTooltip);
    openModButton.setTooltip(openTooltip);
}

void ControlStatusStrip::setHistoryActionState(bool canUndo,
                                               bool canRedo,
                                               const juce::String& undoTooltip,
                                               const juce::String& redoTooltip)
{
    undoButton.setEnabled(canUndo);
    redoButton.setEnabled(canRedo);
    undoButton.setTooltip(undoTooltip);
    redoButton.setTooltip(redoTooltip);
}
}
