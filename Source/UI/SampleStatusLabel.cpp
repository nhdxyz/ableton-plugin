#include "SampleStatusLabel.h"

namespace UI
{
SampleStatusLabel::SampleStatusLabel()
{
    setComponentID("SampleStatusLabel");
    setText("No sample", juce::dontSendNotification);
    setJustificationType(juce::Justification::centredLeft);
    setMinimumHorizontalScale(0.62f);
    setTooltip("Loaded sampler source");
    refreshColour();
}

void SampleStatusLabel::applyTheme(const Theme& theme)
{
    normalColour = theme.textMuted;
    missingColour = theme.warning;
    refreshColour();
}

void SampleStatusLabel::setSampleState(const juce::String& sampleName,
                                       bool sampleMissing,
                                       const juce::String& missingFileName)
{
    missing = sampleMissing;

    const auto displayName = sampleName.isNotEmpty() ? sampleName
                           : sampleMissing && missingFileName.isNotEmpty() ? "Missing: " + missingFileName
                                                                            : juce::String("No sample");
    setText(displayName, juce::dontSendNotification);

    setTooltip(sampleMissing ? "The saved sampler file is missing. Relink or load a new sample before chop playback."
                             : sampleName.isNotEmpty() ? "Loaded sampler source: " + sampleName
                                                       : "No sampler source loaded");
    refreshColour();
}

void SampleStatusLabel::refreshColour()
{
    setColour(juce::Label::textColourId, missing ? missingColour : normalColour);
}
}
