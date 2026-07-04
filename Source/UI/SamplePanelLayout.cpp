#include "SamplePanelLayout.h"

namespace UI::SamplePanelLayout
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
    components.sourceLabel.setVisible(true);
    components.chopLabel.setVisible(true);
    components.shapeLabel.setVisible(true);
    components.fileActions.setVisible(true);
    components.recorderPanel.setVisible(true);
    components.recipeActions.setVisible(true);
    components.sourceControls.setVisible(true);
    components.playbackControls.setVisible(true);
    components.chopExpandButton.setVisible(true);
    components.waveformDisplay.setVisible(true);
    components.chopPanel.setVisible(true);
    components.sampleStatusLabel.setVisible(true);

    components.sectionLabel.setBounds(content.removeFromTop(28));

    for (auto slot : {
             components.start,
             components.end,
             components.transpose,
             components.pitchRamp,
             components.gain,
             components.mix,
             components.stutterRepeats,
             components.grainSize,
             components.grainSpray,
             components.spectralFreeze })
    {
        setSliderVisible(slot, true);
    }

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
    if (sourceArea.getHeight() < 138)
    {
        const auto compactShapeHeight = sourceArea.getHeight();
        const auto showSecondaryShapeRow = compactShapeHeight >= 72;
        const auto sampleShapeRowHeight = showSecondaryShapeRow ? juce::jmax(36, compactShapeHeight / 2)
                                                                : juce::jmax(36, compactShapeHeight);
        layoutKnobRow(sourceArea.removeFromTop(juce::jmin(sampleShapeRowHeight, sourceArea.getHeight())).withTrimmedTop(2),
                      { components.transpose, components.gain, components.mix, components.pitchRamp });
        if (showSecondaryShapeRow)
        {
            layoutKnobRow(sourceArea.withTrimmedTop(2),
                          { components.stutterRepeats, components.grainSize, components.grainSpray, components.spectralFreeze });
        }
        else
        {
            setSliderVisible(components.stutterRepeats, false);
            setSliderVisible(components.grainSize, false);
            setSliderVisible(components.grainSpray, false);
            setSliderVisible(components.spectralFreeze, false);
        }
    }
    else
    {
        const auto sampleShapeRowHeight = juce::jlimit(46, 64, sourceArea.getHeight() / 3);
        layoutKnobRow(sourceArea.removeFromTop(sampleShapeRowHeight).withTrimmedTop(3),
                      { components.transpose, components.gain, components.mix });
        layoutKnobRow(sourceArea.removeFromTop(sampleShapeRowHeight).withTrimmedTop(3),
                      { components.pitchRamp, components.stutterRepeats, components.grainSize });
        layoutKnobRow(sourceArea.removeFromTop(juce::jmin(sampleShapeRowHeight, sourceArea.getHeight())).withTrimmedTop(3),
                      { components.grainSpray, components.spectralFreeze });
    }

    auto chopHeader = chopArea.removeFromTop(24);
    components.chopExpandButton.setBounds(chopHeader.removeFromRight(30).reduced(3, 1));
    components.chopLabel.setBounds(chopHeader.withTrimmedLeft(4));
    const auto waveformHeight = juce::jlimit(170, 260, chopArea.getHeight() / 2);
    components.waveformDisplay.setBounds(chopArea.removeFromTop(waveformHeight).reduced(4, 6));
    components.chopPanel.setBounds(chopArea.removeFromTop(components.chopPanel.compactHeight()));
    auto cutRow = chopArea.removeFromTop(54).withTrimmedTop(6);
    components.start.slider.setBounds(cutRow.removeFromLeft(cutRow.getWidth() / 2).reduced(48, 6));
    components.end.slider.setBounds(cutRow.reduced(48, 6));
}
}
