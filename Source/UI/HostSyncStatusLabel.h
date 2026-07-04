#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class HostSyncStatusLabel final : public juce::Label
{
public:
    enum class State
    {
        free,
        hostStopped,
        hostNoPpq,
        lockedPlaying
    };

    HostSyncStatusLabel();

    void applyTheme(const Theme& theme);
    void setHostSyncStatus(State state, int bpm, double ppqPosition);

private:
    State currentState = State::free;
    int currentBpm = 124;
    double currentPpqPosition = 0.0;
};
}
