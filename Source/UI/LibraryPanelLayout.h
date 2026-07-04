#pragma once

#include "PresetCompareActions.h"
#include "PresetQuickFilterBar.h"
#include "PresetCrateMapDisplay.h"
#include "PresetLibrarySummary.h"
#include "PresetPrimaryActions.h"
#include "PresetSaveSummary.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::LibraryPanelLayout
{
struct Components
{
    juce::Label& sectionLabel;
    juce::Label& findLabel;
    juce::Label& browserLabel;
    juce::Label& saveLabel;
    juce::Label& inspectorLabel;
    juce::Label& statusLabel;
    juce::Label& browserHeaderLabel;

    juce::TextEditor& nameEditor;
    juce::TextEditor& searchEditor;
    juce::TextEditor& authorEditor;
    juce::TextEditor& notesEditor;

    juce::ComboBox& categoryBox;
    juce::ComboBox& filterBox;
    juce::ComboBox& tagBox;
    juce::ComboBox& sortBox;
    juce::ComboBox& browserPackFilterBox;
    juce::ComboBox& ratingBox;
    juce::ComboBox& packBox;
    juce::ComboBox& keyBox;
    juce::ComboBox& bpmBox;
    juce::ComboBox& notesTemplateBox;
    juce::ComboBox& presetBox;

    juce::TextButton& saveButton;
    juce::TextButton& previousButton;
    juce::TextButton& nextButton;
    PresetPrimaryActions& primaryActions;
    juce::TextButton& refreshButton;
    PresetCompareActions& compareActions;
    PresetQuickFilterBar& quickFilterBar;

    juce::ListBox& browserList;
    PresetCrateMapDisplay& crateMapDisplay;
    PresetLibrarySummary& librarySummary;
    PresetSaveSummary& saveSummary;
};

void layout(juce::Rectangle<int> content, Components components);
}
