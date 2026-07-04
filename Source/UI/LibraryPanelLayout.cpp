#include "LibraryPanelLayout.h"

namespace UI::LibraryPanelLayout
{
void layout(juce::Rectangle<int> content, Components components)
{
    components.sectionLabel.setVisible(true);
    components.findLabel.setVisible(true);
    components.browserLabel.setVisible(true);
    components.saveLabel.setVisible(true);
    components.inspectorLabel.setVisible(true);
    components.nameEditor.setVisible(true);
    components.categoryBox.setVisible(true);
    components.filterBox.setVisible(true);
    components.tagBox.setVisible(true);
    components.sortBox.setVisible(true);
    components.browserPackFilterBox.setVisible(true);
    components.ratingBox.setVisible(true);
    components.packBox.setVisible(true);
    components.keyBox.setVisible(true);
    components.bpmBox.setVisible(true);
    components.searchEditor.setVisible(true);
    components.authorEditor.setVisible(true);
    components.notesEditor.setVisible(true);
    components.notesTemplateBox.setVisible(true);
    components.saveButton.setVisible(true);
    components.presetBox.setVisible(true);
    components.previousButton.setVisible(true);
    components.nextButton.setVisible(true);
    components.loadButton.setVisible(true);
    components.auditionButton.setVisible(true);
    components.warmPreviewsButton.setVisible(true);
    components.favoriteButton.setVisible(true);
    components.refreshButton.setVisible(true);
    components.compareButton.setVisible(true);
    components.revertButton.setVisible(true);
    components.statusLabel.setVisible(true);
    components.browserHeaderLabel.setVisible(true);
    components.browserList.setVisible(true);
    components.crateMapDisplay.setVisible(true);
    components.librarySummary.setVisible(true);
    components.saveSummary.setVisible(true);
    components.quickFilterBar.setVisible(true);

    components.sectionLabel.setBounds(content.removeFromTop(28));
    auto libraryArea = content.withTrimmedTop(8);
    const auto leftWidth = juce::jlimit(174, 212, libraryArea.getWidth() / 6);
    const auto inspectorWidth = juce::jlimit(232, 286, libraryArea.getWidth() / 4);
    auto findArea = libraryArea.removeFromLeft(leftWidth).reduced(12, 10);
    auto inspectorArea = libraryArea.removeFromRight(inspectorWidth).reduced(12, 10);
    auto browserArea = libraryArea.reduced(12, 10);

    components.findLabel.setBounds(findArea.removeFromTop(24));
    components.searchEditor.setBounds(findArea.removeFromTop(38).reduced(2, 4));
    components.quickFilterBar.setBounds(findArea.removeFromTop(60));

    findArea.removeFromTop(5);
    components.filterBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
    components.tagBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
    components.browserPackFilterBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
    auto sortRefreshRow = findArea.removeFromTop(36).withTrimmedTop(2);
    components.refreshButton.setBounds(sortRefreshRow.removeFromRight(76).reduced(2, 4));
    components.sortBox.setBounds(sortRefreshRow.reduced(2, 4));

    const auto crateMapHeight = juce::jlimit(96, 132, findArea.getHeight() - 36);
    components.crateMapDisplay.setBounds(findArea.removeFromTop(crateMapHeight).reduced(2, 5));
    components.statusLabel.setBounds(findArea.reduced(2, 4));

    components.browserLabel.setBounds(browserArea.removeFromTop(24));
    auto selectedRow = browserArea.removeFromTop(38).withTrimmedTop(2);
    components.previousButton.setBounds(selectedRow.removeFromLeft(38).reduced(2, 4));
    components.nextButton.setBounds(selectedRow.removeFromRight(38).reduced(2, 4));
    components.presetBox.setBounds(selectedRow.reduced(2, 4));
    auto primaryActions = browserArea.removeFromTop(38).withTrimmedTop(3);
    components.loadButton.setBounds(primaryActions.removeFromLeft(76).reduced(2, 4));
    components.auditionButton.setBounds(primaryActions.removeFromLeft(92).reduced(2, 4));
    components.warmPreviewsButton.setBounds(primaryActions.removeFromLeft(72).reduced(2, 4));
    components.favoriteButton.setBounds(primaryActions.removeFromLeft(70).reduced(2, 4));
    auto compareActions = browserArea.removeFromTop(34).withTrimmedTop(1);
    components.compareButton.setBounds(compareActions.removeFromLeft(82).reduced(2, 4));
    components.revertButton.setBounds(compareActions.removeFromLeft(82).reduced(2, 4));
    components.ratingBox.setBounds(compareActions.reduced(2, 4));
    components.browserHeaderLabel.setBounds(browserArea.removeFromTop(24).reduced(6, 3));
    components.browserList.setBounds(browserArea.reduced(2, 5));

    const auto savePanelHeight = juce::jlimit(252, 304, inspectorArea.getHeight() - 96);
    auto saveArea = inspectorArea.removeFromTop(savePanelHeight).reduced(10, 8);
    components.saveLabel.setBounds(saveArea.removeFromTop(24));
    components.saveSummary.setBounds(saveArea.removeFromTop(72).reduced(2, 5));
    components.nameEditor.setBounds(saveArea.removeFromTop(34).withTrimmedTop(1).reduced(2, 4));
    components.categoryBox.setBounds(saveArea.removeFromTop(32).reduced(2, 4));
    auto metadataRow = saveArea.removeFromTop(32);
    components.authorEditor.setBounds(metadataRow.removeFromLeft(metadataRow.getWidth() / 2).reduced(2, 4));
    components.packBox.setBounds(metadataRow.reduced(2, 4));
    auto keyRow = saveArea.removeFromTop(32);
    const auto keyCellWidth = keyRow.getWidth() / 2;
    components.keyBox.setBounds(keyRow.removeFromLeft(keyCellWidth).reduced(2, 4));
    components.bpmBox.setBounds(keyRow.reduced(2, 4));
    auto saveActionRow = saveArea.removeFromTop(32).withTrimmedTop(2);
    components.notesTemplateBox.setBounds(saveActionRow.removeFromLeft(118).reduced(2, 4));
    components.saveButton.setBounds(saveActionRow.reduced(2, 4));
    components.notesEditor.setBounds(saveArea.reduced(2, 4));

    auto summaryArea = inspectorArea.reduced(10, 6);
    components.inspectorLabel.setBounds(summaryArea.removeFromTop(24));
    components.librarySummary.setBounds(summaryArea.reduced(2, 4));
}
}
