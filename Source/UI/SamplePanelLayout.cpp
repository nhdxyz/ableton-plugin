#include "SamplePanelLayout.h"

namespace UI::SamplePanelLayout
{
void layout(juce::Rectangle<int> content, Components components)
{
    components.sectionLabel.setVisible(true);
    components.sourceLabel.setVisible(true);
    components.shapeLabel.setVisible(true);
    components.fileActions.setVisible(true);
    components.recorderPanel.setVisible(true);
    components.recipeActions.setVisible(true);
    components.rangeControls.setVisible(true);
    components.shapeControls.setVisible(true);
    components.sourceControls.setVisible(true);
    components.playbackControls.setVisible(true);
    components.chopHeader.setVisible(true);
    components.waveformDisplay.setVisible(true);
    components.chopPanel.setVisible(true);
    components.sampleStatusLabel.setVisible(true);

    components.sectionLabel.setBounds(content.removeFromTop(28));

    auto workspace = content.withTrimmedTop(8);
    auto sourceArea = workspace.removeFromLeft(juce::jlimit(260, 310, workspace.getWidth() / 4)).reduced(18, 12);
    workspace.removeFromLeft(10);
    auto chopArea = workspace.reduced(18, 12);

    components.sourceLabel.setBounds(sourceArea.removeFromTop(22));
    components.fileActions.setBounds(sourceArea.removeFromTop(30));
    components.sourceControls.setBounds(sourceArea.removeFromTop(SampleSourceControls::preferredHeight));

    components.sampleStatusLabel.setBounds(sourceArea.removeFromTop(24).reduced(5, 3));
    components.recorderPanel.setBounds(sourceArea.removeFromTop(components.recorderPanel.preferredHeight()));

    components.playbackControls.setBounds(sourceArea.removeFromTop(SamplePlaybackControls::preferredHeight));
    auto cutRecipeRow = sourceArea.removeFromTop(32).withTrimmedTop(2);
    components.recipeActions.setBounds(cutRecipeRow);

    components.shapeLabel.setBounds(sourceArea.removeFromTop(18).withTrimmedLeft(4));
    components.shapeControls.setBounds(sourceArea);

    components.chopHeader.setBounds(chopArea.removeFromTop(24));
    const auto waveformHeight = juce::jlimit(170, 260, chopArea.getHeight() / 2);
    components.waveformDisplay.setBounds(chopArea.removeFromTop(waveformHeight).reduced(4, 6));
    components.chopPanel.setBounds(chopArea.removeFromTop(components.chopPanel.compactHeight()));
    components.rangeControls.setBounds(chopArea.removeFromTop(54));
}
}
