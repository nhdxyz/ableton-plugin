#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class SequencerSceneControls final : public juce::Component
{
public:
    static constexpr size_t sceneCount = 4;

    SequencerSceneControls();

    std::function<void(size_t)> onRecallScene;
    std::function<void(size_t)> onCaptureScene;

    void setSceneState(size_t sceneIndex, bool hasScene, const juce::String& summary);
    void resized() override;

    static juce::String sceneLabel(size_t sceneIndex);

private:
    std::array<juce::TextButton, sceneCount> recallButtons;
    std::array<juce::TextButton, sceneCount> captureButtons;
};
}
