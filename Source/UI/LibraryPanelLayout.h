#pragma once

#include "PresetCrateMapDisplay.h"
#include "PresetLibrarySummary.h"
#include "PresetSaveSummary.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

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
    juce::TextButton& loadButton;
    juce::TextButton& auditionButton;
    juce::TextButton& warmPreviewsButton;
    juce::TextButton& favoriteButton;
    juce::TextButton& refreshButton;
    juce::TextButton& compareButton;
    juce::TextButton& revertButton;
    std::array<juce::TextButton, 10>& quickFilterButtons;

    juce::ListBox& browserList;
    PresetCrateMapDisplay& crateMapDisplay;
    PresetLibrarySummary& librarySummary;
    PresetSaveSummary& saveSummary;
};

void layout(juce::Rectangle<int> content, Components components);
}
