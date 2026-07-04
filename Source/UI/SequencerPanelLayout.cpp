#include "SequencerPanelLayout.h"

namespace UI::SequencerPanelLayout
{
namespace
{
void setSliderVisible(SliderSlot slot, bool shouldBeVisible)
{
    slot.slider.setVisible(shouldBeVisible);
    slot.label.setVisible(shouldBeVisible);
}

void layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<SliderSlot> slots)
{
    const auto count = static_cast<int>(slots.size());
    if (count == 0)
        return;

    const auto cellWidth = juce::jmax(1, area.getWidth() / count);
    const auto horizontalPadding = cellWidth < 64 ? 2 : 4;

    for (auto slot : slots)
    {
        setSliderVisible(slot, true);
        slot.slider.setBounds(area.removeFromLeft(cellWidth).reduced(horizontalPadding, 0));
    }
}

}

void layout(juce::Rectangle<int> content, Components components)
{
    components.sectionLabel.setVisible(true);
    components.enabledButton.setVisible(true);
    components.hostSyncStatusLabel.setVisible(true);
    components.rateEighthButton.setVisible(true);
    components.rateSixteenthButton.setVisible(true);
    components.rateThirtySecondButton.setVisible(true);
    components.grooveBox.setVisible(true);
    components.scaleBox.setVisible(true);
    components.chordBox.setVisible(true);
    components.voicingBox.setVisible(true);
    components.chordMemoryButton.setVisible(true);
    components.patternBox.setVisible(true);
    components.grooveTransformBox.setVisible(true);
    components.laneViewBox.setVisible(true);
    components.lockDestinationBox.setVisible(true);
    components.rootDownButton.setVisible(true);
    components.rootUpButton.setVisible(true);
    components.rootValueLabel.setVisible(true);
    components.stepEditor.setVisible(true);
    components.patternActions.setVisible(true);
    components.copyButton.setVisible(true);
    components.rotateLeftButton.setVisible(true);
    components.rotateRightButton.setVisible(true);
    components.exportMidiButton.setVisible(true);
    components.exportChainButton.setVisible(true);
    components.sceneChainControls.setVisible(true);
    components.applyGrooveTransformButton.setVisible(true);
    components.sceneControls.setVisible(true);
    components.grid.setVisible(true);
    components.expandButton.setVisible(true);

    auto header = content.removeFromTop(28);
    components.expandButton.setBounds(header.removeFromRight(30).reduced(3, 1));
    components.sectionLabel.setBounds(header);

    auto workspace = content.withTrimmedTop(8);
    auto controlArea = workspace.removeFromLeft(juce::jlimit(228, 286, workspace.getWidth() / 5)).reduced(18, 12);
    workspace.removeFromLeft(10);
    auto gridArea = workspace.reduced(18, 12);
    const auto showAdvancedControls = controlArea.getHeight() >= 570;

    auto timingRow = controlArea.removeFromTop(34);
    components.enabledButton.setBounds(timingRow.removeFromLeft(62).reduced(4));
    components.hostSyncStatusLabel.setBounds(timingRow.reduced(4));

    auto rateRow = controlArea.removeFromTop(34);
    const auto rateButtonWidth = rateRow.getWidth() / 3;
    components.rateEighthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
    components.rateSixteenthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
    components.rateThirtySecondButton.setBounds(rateRow.reduced(3, 4));

    auto grooveRow = controlArea.removeFromTop(34);
    components.grooveBox.setBounds(grooveRow.removeFromLeft(grooveRow.getWidth() / 2).reduced(4));
    components.scaleBox.setBounds(grooveRow.reduced(4));
    auto harmonyRow = controlArea.removeFromTop(34);
    components.chordBox.setBounds(harmonyRow.removeFromLeft(harmonyRow.getWidth() / 2).reduced(4));
    components.voicingBox.setBounds(harmonyRow.reduced(4));

    auto patternRow = controlArea.removeFromTop(34);
    components.chordMemoryButton.setBounds(patternRow.removeFromLeft(86).reduced(4));
    components.patternBox.setBounds(patternRow.reduced(4));
    components.patternActions.setBounds(controlArea.removeFromTop(SequencerPatternActions::preferredHeight));

    auto rootStepperRow = controlArea.removeFromTop(34).withTrimmedTop(2);
    components.rootDownButton.setBounds(rootStepperRow.removeFromLeft(38).reduced(4));
    components.rootUpButton.setBounds(rootStepperRow.removeFromRight(38).reduced(4));
    components.rootValueLabel.setBounds(rootStepperRow.reduced(4));

    auto laneViewRow = controlArea.removeFromTop(34).withTrimmedTop(2);
    components.laneViewBox.setBounds(laneViewRow.reduced(4));

    components.stepEditor.setBounds(controlArea.removeFromTop(components.stepEditor.preferredHeight() + 4).withTrimmedTop(4));

    auto transformRow = controlArea.removeFromTop(34).withTrimmedTop(2);
    components.lockDestinationBox.setBounds(transformRow.removeFromLeft(transformRow.getWidth() / 2).reduced(4));
    components.grooveTransformBox.setBounds(transformRow.reduced(4));
    components.applyGrooveTransformButton.setBounds(controlArea.removeFromTop(32).reduced(4));

    if (showAdvancedControls)
    {
        auto sceneArea = controlArea.removeFromTop(68).withTrimmedTop(2);
        components.sceneControls.setBounds(sceneArea);

        auto utilityRow = controlArea.removeFromTop(34).withTrimmedTop(2);
        components.copyButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 3).reduced(4));
        components.rotateLeftButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 2).reduced(4));
        components.rotateRightButton.setBounds(utilityRow.reduced(4));
        auto exportRow = controlArea.removeFromTop(34).withTrimmedTop(2);
        components.exportMidiButton.setBounds(exportRow.removeFromLeft(exportRow.getWidth() / 3).reduced(4));
        components.exportChainButton.setBounds(exportRow.removeFromLeft(exportRow.getWidth() / 2).reduced(4));
        components.sceneChainControls.setBounds(exportRow.reduced(2, 1));
    }
    else
    {
        components.copyButton.setVisible(false);
        components.rotateLeftButton.setVisible(false);
        components.rotateRightButton.setVisible(false);
        components.exportMidiButton.setVisible(false);
        components.exportChainButton.setVisible(false);
        components.sceneChainControls.setVisible(false);
        components.sceneControls.setVisible(false);
    }

    setSliderVisible(components.root, false);
    setSliderVisible(components.gate, true);
    setSliderVisible(components.swing, true);
    setSliderVisible(components.chordStrum, true);
    setSliderVisible(components.accent, true);
    setSliderVisible(components.octave, true);
    setSliderVisible(components.probability, true);
    setSliderVisible(components.random, true);
    setSliderVisible(components.lockDepth, true);

    const auto showPrimaryKnobs = controlArea.getHeight() >= 58;
    setSliderVisible(components.gate, showPrimaryKnobs);
    setSliderVisible(components.swing, showPrimaryKnobs);
    setSliderVisible(components.chordStrum, showPrimaryKnobs);
    if (showPrimaryKnobs)
    {
        const auto primaryKnobHeight = juce::jlimit(58, 72, controlArea.getHeight());
        layoutKnobRow(controlArea.removeFromTop(primaryKnobHeight).withTrimmedTop(4),
                      { components.gate, components.swing, components.chordStrum });
    }

    const auto showSecondaryKnobs = controlArea.getHeight() >= 58;
    setSliderVisible(components.accent, showSecondaryKnobs);
    setSliderVisible(components.octave, showSecondaryKnobs);
    setSliderVisible(components.probability, showSecondaryKnobs);
    setSliderVisible(components.random, showSecondaryKnobs);
    setSliderVisible(components.lockDepth, showSecondaryKnobs);
    if (showSecondaryKnobs)
    {
        layoutKnobRow(controlArea.withTrimmedTop(4),
                      { components.accent, components.octave, components.probability, components.random, components.lockDepth });
    }

    components.grid.setBounds(gridArea.reduced(4, 8));
}
}
