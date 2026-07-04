#include "SequencerSceneControls.h"

namespace UI
{
namespace
{
void placeSceneSlot(juce::TextButton& recallButton,
                    juce::TextButton& captureButton,
                    juce::Rectangle<int>& row,
                    int slotsRemaining)
{
    auto slotArea = row.removeFromLeft(row.getWidth() / slotsRemaining);
    recallButton.setBounds(slotArea.removeFromLeft(slotArea.getWidth() / 2).reduced(3, 4));
    captureButton.setBounds(slotArea.reduced(3, 4));
}
}

SequencerSceneControls::SequencerSceneControls()
{
    setComponentID("SequencerSceneControls");

    for (size_t index = 0; index < sceneCount; ++index)
    {
        auto& recallButton = recallButtons[index];
        auto& captureButton = captureButtons[index];
        const auto label = sceneLabel(index);

        recallButton.setButtonText(label);
        recallButton.setTooltip("Recall pattern scene " + label);
        recallButton.setWantsKeyboardFocus(false);
        recallButton.setMouseClickGrabsKeyboardFocus(false);
        recallButton.onClick = [this, index]
        {
            if (onRecallScene != nullptr)
                onRecallScene(index);
        };
        addAndMakeVisible(recallButton);

        captureButton.setButtonText("Set " + label);
        captureButton.setTooltip("Capture the current sequencer pattern into scene " + label);
        captureButton.setWantsKeyboardFocus(false);
        captureButton.setMouseClickGrabsKeyboardFocus(false);
        captureButton.onClick = [this, index]
        {
            if (onCaptureScene != nullptr)
                onCaptureScene(index);
        };
        addAndMakeVisible(captureButton);
    }
}

void SequencerSceneControls::setSceneState(size_t sceneIndex, bool hasScene, const juce::String& summary)
{
    if (! juce::isPositiveAndBelow(sceneIndex, recallButtons.size()))
        return;

    const auto label = sceneLabel(sceneIndex);
    recallButtons[sceneIndex].setEnabled(hasScene);
    recallButtons[sceneIndex].setButtonText(label + (hasScene ? "*" : ""));
    recallButtons[sceneIndex].setTooltip(summary);
    captureButtons[sceneIndex].setTooltip("Capture current pattern to " + label + " | " + summary);
}

void SequencerSceneControls::resized()
{
    auto area = getLocalBounds();
    auto topRow = area.removeFromTop(32);
    auto bottomRow = area.withTrimmedTop(2);

    placeSceneSlot(recallButtons[0], captureButtons[0], topRow, 2);
    placeSceneSlot(recallButtons[1], captureButtons[1], topRow, 1);
    placeSceneSlot(recallButtons[2], captureButtons[2], bottomRow, 2);
    placeSceneSlot(recallButtons[3], captureButtons[3], bottomRow, 1);
}

juce::String SequencerSceneControls::sceneLabel(size_t sceneIndex)
{
    switch (sceneIndex)
    {
        case 0: return "A";
        case 1: return "B";
        case 2: return "Fill";
        case 3: return "Drop";
        default: return {};
    }
}
}
