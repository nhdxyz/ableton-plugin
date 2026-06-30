#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <vector>

namespace UI
{
class MacroAssignmentPad final : public juce::Component,
                                 public juce::TooltipClient
{
public:
    struct Destination
    {
        int index = 0;
        juce::String name;
        float amount = 0.0f;
        bool assigned = false;
        bool enabled = true;
    };

    struct State
    {
        std::array<juce::String, 8> macroNames;
        std::array<int, 8> macroSourceIndices {};
        std::array<int, 8> macroRouteCounts {};
        int selectedSourceIndex = 4;
        int selectedDestinationIndex = 1;
        float targetAmount = 0.3f;
        std::vector<Destination> destinations;
        juce::String summary;
    };

    std::function<void(int)> onSourceSelected;
    std::function<void(int)> onDestinationSelected;
    std::function<void(int, int, float)> onTargetAmountPreview;
    std::function<void(int, int, float)> onTargetAmountCommit;
    std::function<void(int)> onClearSource;

    MacroAssignmentPad();

    void setState(State newState);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    State state;
    bool editingAmount = false;
    int editingDestinationIndex = 0;

    juce::Rectangle<float> getContentBounds() const;
    juce::Rectangle<float> getMacroRowBounds() const;
    juce::Rectangle<float> getTargetBounds() const;
    juce::Rectangle<float> getDestinationGridBounds() const;
    juce::Rectangle<float> getClearBounds() const;
    juce::Rectangle<float> macroBounds(size_t macroIndex) const;
    juce::Rectangle<float> destinationBounds(size_t destinationIndex) const;
    int sourceIndexForPosition(juce::Point<float> position) const;
    int destinationIndexForPosition(juce::Point<float> position) const;
    void previewAmountAt(juce::Point<float> position);
    void commitAmount();
    int selectedMacroArrayIndex() const noexcept;
    float amountFromPosition(juce::Point<float> position, juce::Rectangle<float> area) const noexcept;
    juce::String selectedDestinationName() const;

    static float clampBipolar(float value) noexcept;
    static juce::Colour macroColour(size_t index) noexcept;
    static juce::String compactName(const juce::String& name);
};
}
