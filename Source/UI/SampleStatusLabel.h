#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class SampleStatusLabel final : public juce::Label
{
public:
    SampleStatusLabel();

    void applyTheme(const Theme& theme);
    void setSampleState(const juce::String& sampleName,
                        bool sampleMissing,
                        const juce::String& missingFileName);

private:
    void refreshColour();

    bool missing = false;
    juce::Colour normalColour { 0xffa8b6b8 };
    juce::Colour missingColour { 0xffffc36b };
};
}
