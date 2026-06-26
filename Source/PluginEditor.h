#pragma once

#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/StepSequencerGrid.h"

#include <juce_gui_extra/juce_gui_extra.h>

class NateVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           public juce::FileDragAndDropTarget,
                                           private juce::Timer
{
public:
    explicit NateVSTAudioProcessorEditor(NateVSTAudioProcessor& processor);
    ~NateVSTAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    enum class Panel
    {
        home,
        synth,
        lab,
        sample,
        sequencer,
        effects,
        library
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    NateVSTAudioProcessor& audioProcessor;
    UI::LookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label homeSectionLabel;
    juce::Label homeEngineLabel;
    juce::Label homeShapeLabel;
    juce::Label homeLabLabel;
    juce::Label homeLibraryLabel;
    juce::Label synthSectionLabel;
    juce::Label randomSectionLabel;
    juce::Label sampleSectionLabel;
    juce::Label sequencerSectionLabel;
    juce::Label futureSectionLabel;
    juce::Label librarySectionLabel;
    juce::Label sampleNameLabel;
    juce::Label presetStatusLabel;

    juce::ComboBox waveformBox;
    juce::ComboBox filterModeBox;
    juce::ComboBox recipeBox;
    juce::ComboBox sequencerRateBox;
    juce::ComboBox presetBox;
    juce::ToggleButton monoButton;
    juce::ToggleButton sampleEnabledButton;
    juce::ToggleButton sampleReverseButton;
    juce::ToggleButton sequencerEnabledButton;
    juce::ToggleButton fxDistortionEnabledButton;
    juce::ToggleButton fxChorusEnabledButton;
    juce::ToggleButton fxDelayEnabledButton;
    juce::ToggleButton fxReverbEnabledButton;

    juce::Slider octaveSlider;
    juce::Slider tuneSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider filterEnvSlider;
    juce::Slider driveSlider;
    juce::Slider outputSlider;
    juce::Slider randomAmountSlider;
    juce::Slider randomChaosSlider;
    juce::Slider brightnessSlider;
    juce::Slider driveBiasSlider;
    juce::Slider motionBiasSlider;
    juce::Slider sampleStartSlider;
    juce::Slider sampleEndSlider;
    juce::Slider sampleTransposeSlider;
    juce::Slider sampleGainSlider;
    juce::Slider sampleMixSlider;
    juce::Slider sequencerRootSlider;
    juce::Slider sequencerGateSlider;
    juce::Slider sequencerRandomSlider;
    juce::Slider fxDistortionAmountSlider;
    juce::Slider fxChorusRateSlider;
    juce::Slider fxChorusDepthSlider;
    juce::Slider fxChorusMixSlider;
    juce::Slider fxDelayTimeSlider;
    juce::Slider fxDelayFeedbackSlider;
    juce::Slider fxDelayMixSlider;
    juce::Slider fxReverbSizeSlider;
    juce::Slider fxReverbDampingSlider;
    juce::Slider fxReverbMixSlider;

    juce::Label octaveLabel;
    juce::Label tuneLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label filterEnvLabel;
    juce::Label driveLabel;
    juce::Label outputLabel;
    juce::Label randomAmountLabel;
    juce::Label randomChaosLabel;
    juce::Label brightnessLabel;
    juce::Label driveBiasLabel;
    juce::Label motionBiasLabel;
    juce::Label sampleStartLabel;
    juce::Label sampleEndLabel;
    juce::Label sampleTransposeLabel;
    juce::Label sampleGainLabel;
    juce::Label sampleMixLabel;
    juce::Label sequencerRootLabel;
    juce::Label sequencerGateLabel;
    juce::Label sequencerRandomLabel;
    juce::Label fxDistortionAmountLabel;
    juce::Label fxChorusRateLabel;
    juce::Label fxChorusDepthLabel;
    juce::Label fxChorusMixLabel;
    juce::Label fxDelayTimeLabel;
    juce::Label fxDelayFeedbackLabel;
    juce::Label fxDelayMixLabel;
    juce::Label fxReverbSizeLabel;
    juce::Label fxReverbDampingLabel;
    juce::Label fxReverbMixLabel;

    juce::TextButton generateButton { "Generate" };
    juce::TextButton mutateButton { "Mutate" };
    juce::TextButton variationButton { "Variation" };
    juce::TextButton loadSampleButton { "Load" };
    juce::TextButton clearSampleButton { "Clear" };
    juce::TextButton randomCutButton { "Rand Cut" };
    juce::TextButton randomSequencerButton { "Rand Seq" };
    juce::TextButton clearSequencerButton { "Clear" };
    juce::TextButton homeTabButton { "HOME" };
    juce::TextButton synthTabButton { "SYNTH" };
    juce::TextButton labTabButton { "LAB" };
    juce::TextButton sampleTabButton { "SAMPLE" };
    juce::TextButton sequencerTabButton { "SEQ" };
    juce::TextButton effectsTabButton { "FX" };
    juce::TextButton libraryTabButton { "LIBRARY" };
    juce::TextButton sineWaveButton { "Sine" };
    juce::TextButton sawWaveButton { "Saw" };
    juce::TextButton squareWaveButton { "Square" };
    juce::TextButton triangleWaveButton { "Tri" };
    juce::TextButton lowpassFilterButton { "LP" };
    juce::TextButton bandpassFilterButton { "BP" };
    juce::TextButton highpassFilterButton { "HP" };
    juce::TextButton rateEighthButton { "1/8" };
    juce::TextButton rateSixteenthButton { "1/16" };
    juce::TextButton rateThirtySecondButton { "1/32" };
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };
    juce::TextButton refreshPresetsButton { "Refresh" };
    juce::TextEditor presetNameEditor;
    UI::StepSequencerGrid sequencerGrid;

    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    void configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void configureHorizontalSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void configureSectionLabel(juce::Label& label, const juce::String& text);
    juce::Rectangle<int> layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<juce::Component*> components);
    void chooseSampleFile();
    void loadSampleFile(const juce::File& file);
    void updateSampleNameLabel();
    void setActivePanel(Panel panel);
    void updatePanelVisibility();
    void updateTabButtons();
    void hidePanelComponents();
    void setSliderVisible(juce::Slider& slider, juce::Label& label, bool shouldBeVisible);
    void setChoiceParameter(const juce::String& parameterID, int choiceIndex);
    void updateSegmentedSelectors();
    void timerCallback() override;
    void refreshPresetList();
    void saveCurrentPreset();
    void loadSelectedPreset();

    Panel activePanel = Panel::home;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessorEditor)
};
