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
    components.rateControls.setVisible(true);
    components.grooveControls.setVisible(true);
    components.patternControls.setVisible(true);
    components.grooveTransformBox.setVisible(true);
    components.laneViewBox.setVisible(true);
    components.lockDestinationBox.setVisible(true);
    components.rootControls.setVisible(true);
    components.stepEditor.setVisible(true);
    components.utilityActions.setVisible(true);
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

    components.rateControls.setBounds(controlArea.removeFromTop(34));

    components.grooveControls.setBounds(controlArea.removeFromTop(SequencerGrooveControls::preferredHeight));

    components.patternControls.setBounds(controlArea.removeFromTop(SequencerPatternControls::preferredHeight));

    components.rootControls.setBounds(controlArea.removeFromTop(34).withTrimmedTop(2));

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

        auto utilityArea = controlArea.removeFromTop(SequencerUtilityActions::preferredHeight);
        components.utilityActions.setBounds(utilityArea);
        auto chainRow = utilityArea.removeFromBottom(utilityArea.getHeight() / 2).withTrimmedTop(2);
        chainRow.removeFromLeft(chainRow.getWidth() * 2 / 3);
        components.sceneChainControls.setBounds(chainRow.reduced(2, 1));
    }
    else
    {
        components.utilityActions.setVisible(false);
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
