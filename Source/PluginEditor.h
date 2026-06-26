#pragma once

#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/OutputMeter.h"
#include "UI/StepSequencerGrid.h"

#include <juce_audio_utils/juce_audio_utils.h>
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
    juce::Label randomStatusLabel;

    juce::ComboBox waveformBox;
    juce::ComboBox osc2WaveBox;
    juce::ComboBox filterModeBox;
    juce::ComboBox recipeBox;
    juce::ComboBox sequencerRateBox;
    juce::ComboBox presetBox;
    juce::ComboBox presetCategoryBox;
    juce::ComboBox presetFilterBox;
    juce::ToggleButton monoButton;
    juce::ToggleButton sampleEnabledButton;
    juce::ToggleButton sampleReverseButton;
    juce::ToggleButton sequencerEnabledButton;
    juce::ToggleButton fxDistortionEnabledButton;
    juce::ToggleButton fxChorusEnabledButton;
    juce::ToggleButton fxDelayEnabledButton;
    juce::ToggleButton fxReverbEnabledButton;
    juce::ToggleButton fxToneEnabledButton;
    juce::ToggleButton fxPhaserEnabledButton;
    juce::ToggleButton fxGuardEnabledButton;
    juce::ToggleButton randomLockPitchButton;
    juce::ToggleButton randomLockEnvelopeButton;
    juce::ToggleButton randomLockFilterButton;
    juce::ToggleButton randomLockSourceButton;
    juce::ToggleButton randomLockSampleButton;
    juce::ToggleButton randomLockFxButton;
    juce::ToggleButton randomLockOutputButton;
    juce::ToggleButton randomLockSequencerButton;

    juce::Slider octaveSlider;
    juce::Slider tuneSlider;
    juce::Slider osc1LevelSlider;
    juce::Slider osc2OctaveSlider;
    juce::Slider osc2TuneSlider;
    juce::Slider osc2LevelSlider;
    juce::Slider subLevelSlider;
    juce::Slider noiseLevelSlider;
    juce::Slider unisonVoicesSlider;
    juce::Slider unisonDetuneSlider;
    juce::Slider unisonBlendSlider;
    juce::Slider unisonSpreadSlider;
    juce::Slider macroToneSlider;
    juce::Slider macroDirtSlider;
    juce::Slider macroMotionSlider;
    juce::Slider macroSpaceSlider;
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
    juce::Slider sequencerSwingSlider;
    juce::Slider sequencerAccentSlider;
    juce::Slider sequencerOctaveSlider;
    juce::Slider sequencerProbabilitySlider;
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
    juce::Slider fxToneTiltSlider;
    juce::Slider fxToneLowCutSlider;
    juce::Slider fxPhaserRateSlider;
    juce::Slider fxPhaserDepthSlider;
    juce::Slider fxPhaserMixSlider;
    juce::Slider fxGuardPushSlider;
    juce::Slider fxGuardCeilingSlider;

    juce::Label octaveLabel;
    juce::Label tuneLabel;
    juce::Label osc1LevelLabel;
    juce::Label osc2OctaveLabel;
    juce::Label osc2TuneLabel;
    juce::Label osc2LevelLabel;
    juce::Label subLevelLabel;
    juce::Label noiseLevelLabel;
    juce::Label unisonVoicesLabel;
    juce::Label unisonDetuneLabel;
    juce::Label unisonBlendLabel;
    juce::Label unisonSpreadLabel;
    juce::Label macroToneLabel;
    juce::Label macroDirtLabel;
    juce::Label macroMotionLabel;
    juce::Label macroSpaceLabel;
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
    juce::Label sequencerSwingLabel;
    juce::Label sequencerAccentLabel;
    juce::Label sequencerOctaveLabel;
    juce::Label sequencerProbabilityLabel;
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
    juce::Label fxToneTiltLabel;
    juce::Label fxToneLowCutLabel;
    juce::Label fxPhaserRateLabel;
    juce::Label fxPhaserDepthLabel;
    juce::Label fxPhaserMixLabel;
    juce::Label fxGuardPushLabel;
    juce::Label fxGuardCeilingLabel;

    juce::TextButton generateButton { "Generate" };
    juce::TextButton mutateButton { "Mutate" };
    juce::TextButton variationButton { "Variation" };
    juce::TextButton undoRandomButton { "Undo" };
    juce::TextButton loadSampleButton { "Load" };
    juce::TextButton clearSampleButton { "Clear" };
    juce::TextButton randomCutButton { "Rand Cut" };
    juce::TextButton randomSequencerButton { "Rand Seq" };
    juce::TextButton clearSequencerButton { "Clear" };
    juce::TextButton bassPatternButton { "Bass" };
    juce::TextButton stabPatternButton { "Stab" };
    juce::TextButton copySequencerButton { "Copy" };
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
    juce::TextButton osc2SineWaveButton { "Sine" };
    juce::TextButton osc2SawWaveButton { "Saw" };
    juce::TextButton osc2SquareWaveButton { "Square" };
    juce::TextButton osc2TriangleWaveButton { "Tri" };
    juce::TextButton lowpassFilterButton { "LP" };
    juce::TextButton bandpassFilterButton { "BP" };
    juce::TextButton highpassFilterButton { "HP" };
    juce::TextButton rateEighthButton { "1/8" };
    juce::TextButton rateSixteenthButton { "1/16" };
    juce::TextButton rateThirtySecondButton { "1/32" };
    juce::TextButton previousPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };
    juce::TextButton refreshPresetsButton { "Refresh" };
    juce::TextButton favoritePresetButton { "Fav" };
    juce::TextButton keyboardOctaveDownButton { "Oct -" };
    juce::TextButton keyboardOctaveUpButton { "Oct +" };
    juce::TextButton keyboardPanicButton { "Panic" };
    juce::Label keyboardRangeLabel;
    juce::TextEditor presetNameEditor;
    UI::OutputMeter outputMeter;
    juce::MidiKeyboardComponent pianoKeyboard;
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
    void setRandomStatus(const juce::String& action);
    void setActivePanel(Panel panel);
    void updatePanelVisibility();
    void updateTabButtons();
    void hidePanelComponents();
    void setSliderVisible(juce::Slider& slider, juce::Label& label, bool shouldBeVisible);
    void setChoiceParameter(const juce::String& parameterID, int choiceIndex);
    void updateSegmentedSelectors();
    void updateOutputMeter();
    void timerCallback() override;
    void refreshPresetList();
    void saveCurrentPreset();
    void loadSelectedPreset();
    void loadPresetByOffset(int offset);
    void toggleFavoritePreset();
    void updateFavoritePresetButton();
    void shiftKeyboardOctave(int semitones);
    void updateKeyboardRangeLabel();

    Panel activePanel = Panel::home;
    float displayedPeakLeft = 0.0f;
    float displayedPeakRight = 0.0f;
    float displayedRmsLeft = 0.0f;
    float displayedRmsRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessorEditor)
};
