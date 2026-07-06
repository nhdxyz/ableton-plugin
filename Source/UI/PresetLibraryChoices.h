#pragma once

#include <juce_core/juce_core.h>

namespace UI::PresetLibraryChoices
{
juce::StringArray categoryChoices();
juce::StringArray filterChoices();
juce::StringArray sortChoices();
juce::StringArray packChoices();
juce::StringArray keyChoices();
juce::StringArray bpmChoices();
juce::StringArray tagChoices();

int parseBpm(const juce::String& text);
juce::String formatBpm(int bpm);
}
