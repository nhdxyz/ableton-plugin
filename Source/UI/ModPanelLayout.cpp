#include "ModPanelLayout.h"

namespace UI::ModPanelLayout
{
namespace
{
constexpr auto matrixTitleHeight = 28;
constexpr auto matrixInspectorHeight = 30;
constexpr auto matrixRouteMapHeight = 46;
constexpr auto matrixHeaderHeight = 18;
constexpr auto matrixMinimumRowHeight = 26;
constexpr auto matrixRouteMapMinimumRowHeight = 26;

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

void layoutSourceRows(SourceControls controls,
                      juce::Rectangle<int> area,
                      int minimumCellHeight,
                      int horizontalReduction,
                      int verticalReduction)
{
    constexpr auto sourceColumnCount = 5;
    constexpr auto sourceRowCount = 4;
    const auto sourceCellHeight = juce::jmax(minimumCellHeight, area.getHeight() / sourceRowCount);

    for (size_t index = 0; index < controls.sourceRows.size(); ++index)
    {
        controls.sourceRows[index].setVisible(true);
        const auto row = static_cast<int>(index) / sourceColumnCount;
        const auto column = static_cast<int>(index) % sourceColumnCount;
        auto rowArea = area.withTrimmedTop(row * sourceCellHeight).withHeight(sourceCellHeight);
        const auto cellWidth = rowArea.getWidth() / sourceColumnCount;
        controls.sourceRows[index].setBounds(rowArea.withTrimmedLeft(column * cellWidth)
                                                    .withWidth(cellWidth)
                                                    .reduced(horizontalReduction, verticalReduction));
    }
}

void layoutCompactSourceRail(SourceControls controls, juce::Rectangle<int>& area)
{
    controls.sourceLabel.setVisible(true);

    auto railArea = area.removeFromTop(94);
    controls.sourceLabel.setBounds(railArea.removeFromTop(18).withTrimmedLeft(4));
    layoutSourceRows(controls, railArea.withTrimmedTop(4), 10, 3, 2);

    area.removeFromTop(8);
}

void layoutSourcesPage(SourceControls sourceControls, GeneratorControls generatorControls, juce::Rectangle<int> area)
{
    sourceControls.sourceLabel.setVisible(true);
    generatorControls.lfo2Label.setVisible(true);
    generatorControls.envelopeLabel.setVisible(true);
    generatorControls.lfo2ShapeBox.setVisible(true);
    generatorControls.lfo2SyncRateBox.setVisible(true);
    generatorControls.lfo2SyncButton.setVisible(true);
    generatorControls.lfo2RetriggerButton.setVisible(true);

    auto sourceArea = area.removeFromLeft(juce::jlimit(420, 590, area.getWidth() / 2));
    area.removeFromLeft(12);
    auto generatorArea = area;

    sourceControls.sourceLabel.setBounds(sourceArea.removeFromTop(22));
    layoutSourceRows(sourceControls, sourceArea.withTrimmedTop(6), 1, 4, 3);

    auto lfo2Area = generatorArea.removeFromTop(juce::jlimit(146, 188, generatorArea.getHeight() / 2));
    generatorArea.removeFromTop(10);
    auto envelopeArea = generatorArea;

    generatorControls.lfo2Label.setBounds(lfo2Area.removeFromTop(22));
    auto lfo2ModeRow = lfo2Area.removeFromTop(36).withTrimmedTop(3);
    generatorControls.lfo2ShapeBox.setBounds(lfo2ModeRow.removeFromLeft(lfo2ModeRow.getWidth() / 2).reduced(4));
    generatorControls.lfo2SyncRateBox.setBounds(lfo2ModeRow.reduced(4));
    auto lfo2ToggleRow = lfo2Area.removeFromTop(34).withTrimmedTop(3);
    generatorControls.lfo2SyncButton.setBounds(lfo2ToggleRow.removeFromLeft(lfo2ToggleRow.getWidth() / 2).reduced(4));
    generatorControls.lfo2RetriggerButton.setBounds(lfo2ToggleRow.reduced(4));
    layoutKnobRow(lfo2Area.withTrimmedTop(4),
                  { generatorControls.lfo2Rate, generatorControls.lfo2Depth, generatorControls.lfo2Phase });

    generatorControls.envelopeLabel.setBounds(envelopeArea.removeFromTop(22));
    layoutKnobRow(envelopeArea.removeFromTop(76).withTrimmedTop(4),
                  { generatorControls.envAttack, generatorControls.envDecay, generatorControls.envSustain });
    layoutKnobRow(envelopeArea.removeFromTop(76).withTrimmedTop(4),
                  { generatorControls.envRelease, generatorControls.envDepth });
}

void layoutMacrosPage(SourceControls sourceControls, MacroControls macroControls, juce::Rectangle<int> area)
{
    layoutCompactSourceRail(sourceControls, area);
    macroControls.macroLabel.setVisible(true);
    macroControls.assignLabel.setVisible(true);
    macroControls.assignStatusLabel.setVisible(true);
    macroControls.assignmentPad.setVisible(true);
    macroControls.performanceMap.setVisible(true);
    macroControls.expandButton.setVisible(true);

    auto macroHeader = area.removeFromTop(26);
    macroControls.macroLabel.setBounds(macroHeader.removeFromLeft(78));
    macroControls.assignLabel.setBounds(macroHeader.removeFromLeft(66));
    macroControls.expandButton.setBounds(macroHeader.removeFromRight(30).reduced(3, 2));
    macroControls.assignStatusLabel.setBounds(macroHeader.reduced(4, 1));

    area.removeFromTop(8);
    const auto mapWidth = juce::jlimit(250, 360, area.getWidth() / 3);
    macroControls.performanceMap.setBounds(area.removeFromLeft(mapWidth).reduced(4));
    area.removeFromLeft(10);
    macroControls.assignmentPad.setBounds(area.reduced(4));
}

void layoutCurvesPage(SourceControls sourceControls, CurveControls curveControls, juce::Rectangle<int> area)
{
    layoutCompactSourceRail(sourceControls, area);
    curveControls.lfoLabel.setVisible(true);
    curveControls.lfo1ShapeBox.setVisible(true);
    curveControls.lfo1SyncRateBox.setVisible(true);
    curveControls.presetBox.setVisible(true);
    curveControls.actionBox.setVisible(true);
    curveControls.syncButton.setVisible(true);
    curveControls.retriggerButton.setVisible(true);
    curveControls.toolStrip.setVisible(true);
    curveControls.curveDisplay.setVisible(true);

    curveControls.lfoLabel.setBounds(area.removeFromTop(22));
    auto controlRow = area.removeFromTop(38).withTrimmedTop(3);
    curveControls.lfo1ShapeBox.setBounds(controlRow.removeFromLeft(juce::jlimit(150, 220, controlRow.getWidth() / 4)).reduced(4));
    curveControls.lfo1SyncRateBox.setBounds(controlRow.removeFromLeft(juce::jlimit(110, 160, controlRow.getWidth() / 4)).reduced(4));
    curveControls.presetBox.setBounds(controlRow.removeFromLeft(juce::jlimit(150, 230, controlRow.getWidth() / 3)).reduced(4));
    curveControls.actionBox.setBounds(controlRow.removeFromLeft(juce::jlimit(130, 180, controlRow.getWidth() / 2)).reduced(4));
    curveControls.syncButton.setBounds(controlRow.removeFromLeft(82).reduced(4));
    curveControls.retriggerButton.setBounds(controlRow.removeFromLeft(88).reduced(4));

    curveControls.toolStrip.setBounds(area.removeFromTop(34).withTrimmedTop(2));

    const auto knobHeight = juce::jlimit(62, 84, area.getHeight() / 4);
    curveControls.curveDisplay.setBounds(area.removeFromTop(area.getHeight() - knobHeight).withTrimmedTop(4));
    layoutKnobRow(area.removeFromTop(knobHeight).withTrimmedTop(6),
                  { curveControls.rate, curveControls.depth, curveControls.phase });
}

void setMatrixRouteVisible(MatrixControls controls, size_t index, bool shouldBeVisible)
{
    controls.matrixRows[index].setVisible(shouldBeVisible);
    controls.slotRows[index].setVisible(shouldBeVisible);
    controls.sourceBoxes[index].setVisible(shouldBeVisible);
    controls.destinationBoxes[index].setVisible(shouldBeVisible);
    controls.amountSliders[index].setVisible(shouldBeVisible);
    controls.enabledButtons[index].setVisible(shouldBeVisible);
    controls.duplicateButtons[index].setVisible(shouldBeVisible);
    controls.deleteButtons[index].setVisible(shouldBeVisible);
}

void placeMatrixHeader(juce::Rectangle<int> header,
                       juce::Label& source,
                       juce::Label& destination,
                       juce::Label& amount)
{
    constexpr auto slotWidth = 26;
    const auto actionWidth = juce::jlimit(70, 78, header.getWidth() / 6);
    const auto fieldAreaWidth = juce::jmax(1, header.getWidth() - slotWidth - actionWidth);
    const auto sourceWidth = juce::jlimit(140, 190, fieldAreaWidth / 4);
    const auto destinationWidth = juce::jlimit(170, 240, fieldAreaWidth / 3);

    header.removeFromLeft(slotWidth);
    header.removeFromRight(actionWidth);
    source.setBounds(header.removeFromLeft(sourceWidth).reduced(5, 0));
    destination.setBounds(header.removeFromLeft(destinationWidth).reduced(5, 0));
    amount.setBounds(header.reduced(5, 0));
}

void placeMatrixRouteRow(MatrixControls controls, size_t index, juce::Rectangle<int>& bank, int rowsRemaining)
{
    const auto rowHeight = juce::jmax(matrixMinimumRowHeight, bank.getHeight() / rowsRemaining);
    auto rowBounds = bank.removeFromTop(rowHeight).reduced(3, 1);
    controls.matrixRows[index].setBounds(rowBounds);

    auto row = rowBounds.reduced(2, 2);
    constexpr auto slotWidth = 26;
    const auto actionWidth = juce::jlimit(70, 78, row.getWidth() / 6);
    const auto fieldAreaWidth = juce::jmax(1, row.getWidth() - slotWidth - actionWidth);
    const auto sourceWidth = juce::jlimit(140, 190, fieldAreaWidth / 4);
    const auto destinationWidth = juce::jlimit(170, 240, fieldAreaWidth / 3);

    controls.slotRows[index].setBounds(row.removeFromLeft(slotWidth).reduced(2, 0));
    controls.sourceBoxes[index].setBounds(row.removeFromLeft(sourceWidth).reduced(3, 0));
    controls.destinationBoxes[index].setBounds(row.removeFromLeft(destinationWidth).reduced(3, 0));
    auto actionArea = row.removeFromRight(actionWidth);
    controls.enabledButtons[index].setBounds(actionArea.removeFromLeft(30).reduced(1, 0));
    controls.duplicateButtons[index].setBounds(actionArea.removeFromLeft(22).reduced(1, 0));
    controls.deleteButtons[index].setBounds(actionArea.reduced(1, 0));
    controls.amountSliders[index].setBounds(row.reduced(3, 0));
}

void layoutMatrixPage(MatrixControls controls, juce::Rectangle<int> matrixArea)
{
    controls.matrixLabel.setVisible(true);
    controls.statusLabel.setVisible(true);
    controls.inspectorLabel.setVisible(true);
    controls.destinationBox.setVisible(true);
    controls.sourceBox.setVisible(true);
    controls.inspectorStatusLabel.setVisible(true);
    controls.inspectorActions.setVisible(true);
    controls.sourceHeader.setVisible(true);
    controls.destinationHeader.setVisible(true);
    controls.amountHeader.setVisible(true);
    controls.sourceHeaderB.setVisible(false);
    controls.destinationHeaderB.setVisible(false);
    controls.amountHeaderB.setVisible(false);

    const auto canShowRouteMap = matrixArea.getHeight() >= matrixTitleHeight
                                                        + matrixInspectorHeight
                                                        + matrixRouteMapHeight
                                                        + matrixHeaderHeight
                                                        + (matrixRouteMapMinimumRowHeight * static_cast<int>(controls.matrixRows.size()));
    auto matrixTitleRow = matrixArea.removeFromTop(matrixTitleHeight);
    controls.matrixLabel.setBounds(matrixTitleRow.removeFromLeft(70).withTrimmedTop(5));
    controls.statusLabel.setBounds(matrixTitleRow.removeFromLeft(150).reduced(3, 5));
    controls.inspectorStatusLabel.setBounds(matrixTitleRow.reduced(5, 5));

    auto inspectorRow = matrixArea.removeFromTop(matrixInspectorHeight);
    controls.inspectorLabel.setBounds(inspectorRow.removeFromLeft(62).withTrimmedTop(6));
    controls.destinationBox.setBounds(inspectorRow.removeFromLeft(150).reduced(3, 4));
    controls.sourceBox.setBounds(inspectorRow.removeFromLeft(138).reduced(3, 4));
    controls.inspectorActions.setBounds(inspectorRow.removeFromLeft(ModInspectorActions::preferredWidth));

    if (canShowRouteMap)
    {
        auto routeMapRow = matrixArea.removeFromTop(matrixRouteMapHeight).withTrimmedTop(4);
        controls.routeMapDisplay.setVisible(true);
        controls.routeMapDisplay.setBounds(routeMapRow.reduced(3, 2));
    }
    else
    {
        controls.routeMapDisplay.setVisible(false);
    }

    placeMatrixHeader(matrixArea.removeFromTop(matrixHeaderHeight).reduced(3, 1),
                      controls.sourceHeader,
                      controls.destinationHeader,
                      controls.amountHeader);

    const auto rowsPerBankToShow = juce::jlimit(1,
                                                static_cast<int>(controls.matrixRows.size()),
                                                matrixArea.getHeight() / matrixMinimumRowHeight);
    for (size_t index = 0; index < controls.slotRows.size(); ++index)
        setMatrixRouteVisible(controls, index, static_cast<int>(index) < rowsPerBankToShow);

    for (auto row = 0; row < rowsPerBankToShow; ++row)
        placeMatrixRouteRow(controls, static_cast<size_t>(row), matrixArea, rowsPerBankToShow - row);
}
}

void layout(juce::Rectangle<int> content, Components components)
{
    components.sectionLabel.setVisible(true);
    components.sectionLabel.setBounds(content.removeFromTop(28));

    auto workflowRow = content.removeFromTop(38).withTrimmedTop(4);
    components.workflowStrip.setVisible(true);
    components.workflowStrip.setBounds(workflowRow);

    auto modContent = content.withTrimmedTop(8).reduced(18, 10);
    switch (components.activePage)
    {
        case Page::sources:
            layoutSourcesPage(components.sources, components.generators, modContent);
            break;

        case Page::macros:
            layoutMacrosPage(components.sources, components.macros, modContent);
            break;

        case Page::curves:
            layoutCurvesPage(components.sources, components.curves, modContent);
            break;

        case Page::matrix:
        default:
            layoutMatrixPage(components.matrix, modContent);
            break;
    }
}
}
