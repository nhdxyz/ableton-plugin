#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    NateVSTAudioProcessor processor;
    NateVSTAudioProcessorEditor editor(processor);

    const auto issues = editor.runLayoutAudit();
    if (! issues.isEmpty())
    {
        std::cerr << "Editor layout audit failed with " << issues.size() << " issue(s):\n";
        for (const auto& issue : issues)
            std::cerr << " - " << issue << '\n';

        return 1;
    }

    std::cout << "Editor layout audit passed for all panels and FX detail modules.\n";
    return 0;
}
