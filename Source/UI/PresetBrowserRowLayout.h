#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::PresetBrowserRowLayout
{
struct Metrics
{
    int paddedWidth = 0;
    int nameWidth = 0;
    int infoWidth = 0;
    int previewWidth = 0;
    int macroWidth = 0;
    bool compact = false;
    bool showsMacroStrip = true;
};

Metrics forSize(int width, int height);
}
