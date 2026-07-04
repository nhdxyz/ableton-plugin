#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class PresetPrimaryActions final : public juce::Component
{
public:
    enum class LayoutMode
    {
        home,
        library
    };

    PresetPrimaryActions();

    std::function<void()> onLoadClicked;
    std::function<void()> onAuditionClicked;
    std::function<void()> onWarmPreviewsClicked;
    std::function<void()> onFavoriteClicked;

    static int preferredWidth(LayoutMode mode) noexcept;

    void setLayoutMode(LayoutMode newMode);
    void setPresetState(bool hasPreset, bool isFavorite);
    void setWarmPreviewsEnabled(bool shouldBeEnabled);
    void setAuditionTooltip(const juce::String& tooltipText);
    void setWarmPreviewsTooltip(const juce::String& tooltipText);

    void resized() override;

private:
    juce::TextButton loadButton { "Load" };
    juce::TextButton auditionButton { "Audition" };
    juce::TextButton warmPreviewsButton { "Warm" };
    juce::TextButton favoriteButton { "Fav" };

    LayoutMode mode = LayoutMode::library;

    void configureButton(juce::TextButton& button,
                         std::function<void()> PresetPrimaryActions::* callback,
                         const juce::String& tooltip);
};
}
