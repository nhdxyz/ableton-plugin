#pragma once

#include "PluginProcessor.h"
#include "UI/FxRackRow.h"
#include "UI/LookAndFeel.h"
#include "UI/LowEndAssistant.h"
#include "UI/ModCurveDisplay.h"
#include "UI/ModMatrixRow.h"
#include "UI/OutputMeter.h"
#include "UI/PumpCurveDisplay.h"
#include "UI/SampleWaveformDisplay.h"
#include "UI/StepSequencerGrid.h"
#include "UI/XYMacroPad.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>

class NateVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           public juce::FileDragAndDropTarget,
                                           private juce::Timer,
                                           private juce::ListBoxModel
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
        mod,
        sample,
        sequencer,
        effects,
        library
    };

    enum class FxModule
    {
        tone = 0,
        eq,
        distortion,
        bitcrush,
        pump,
        tremolo,
        ring,
        comb,
        phaser,
        flanger,
        chorus,
        delay,
        reverb,
        width,
        guard
    };

    enum class MomentaryFxAction
    {
        none,
        delay,
        space,
        pump,
        mute
    };

    struct FxMomentarySnapshot
    {
        std::array<float, 31> values {};
        FxModule selectedModule = FxModule::guard;
        bool valid = false;
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
    juce::Label synthSourceLabel;
    juce::Label synthVoiceLabel;
    juce::Label synthFilterLabel;
    juce::Label synthAmpLabel;
    juce::Label randomSectionLabel;
    juce::Label modSectionLabel;
    juce::Label modSourceLabel;
    juce::Label modMacroLabel;
    juce::Label modLfoLabel;
    juce::Label modEnvelopeLabel;
    juce::Label modMatrixLabel;
    juce::Label modMatrixStatusLabel;
    juce::Label modInspectorLabel;
    juce::Label modInspectorStatusLabel;
    juce::Label modMatrixSourceHeader;
    juce::Label modMatrixDestinationHeader;
    juce::Label modMatrixAmountHeader;
    juce::Label modMatrixSourceHeaderB;
    juce::Label modMatrixDestinationHeaderB;
    juce::Label modMatrixAmountHeaderB;
    juce::Label modMacroAssignLabel;
    juce::Label modMacroAssignStatusLabel;
    juce::Label sampleSectionLabel;
    juce::Label sampleSourceLabel;
    juce::Label sampleChopLabel;
    juce::Label sampleShapeLabel;
    juce::Label sampleSliceStatusLabel;
    juce::Label sequencerSectionLabel;
    juce::Label hostSyncStatusLabel;
    juce::Label selectedControlHeaderLabel;
    juce::Label selectedControlStatusLabel;
    juce::Label futureSectionLabel;
    juce::Label librarySectionLabel;
    juce::Label sampleNameLabel;
    juce::Label presetStatusLabel;
    juce::Label presetBrowserHeaderLabel;
    juce::Label randomStatusLabel;
    juce::Label performanceStatusLabel;

    juce::ComboBox waveformBox;
    juce::ComboBox osc2WaveBox;
    juce::ComboBox filterModeBox;
    juce::ComboBox filterCharacterBox;
    juce::ComboBox filterSlopeBox;
    juce::ComboBox recipeBox;
    juce::ComboBox randomScopeBox;
    juce::ComboBox sequencerRateBox;
    juce::ComboBox sequencerGrooveBox;
    juce::ComboBox sequencerScaleBox;
    juce::ComboBox sequencerChordBox;
    juce::ComboBox sequencerVoicingBox;
    juce::ComboBox sequencerPatternBox;
    juce::ComboBox sequencerGrooveTransformBox;
    juce::ComboBox sampleModeBox;
    juce::ComboBox sampleSliceStyleBox;
    juce::ComboBox sampleStutterRateBox;
    juce::ComboBox presetBox;
    juce::ComboBox presetCategoryBox;
    juce::ComboBox presetFilterBox;
    juce::ComboBox presetTagBox;
    juce::ComboBox presetSortBox;
    juce::ComboBox presetRatingBox;
    juce::ComboBox presetPackBox;
    juce::ComboBox presetKeyBox;
    juce::ComboBox presetBpmBox;
    juce::ComboBox fxAddBox;
    juce::ComboBox fxPresetBox;
    juce::ComboBox fxDelayRateBox;
    juce::ComboBox fxPumpRateBox;
    juce::ComboBox fxPumpCurveBox;
    juce::ComboBox fxTremoloRateBox;
    juce::ComboBox modInspectorDestinationBox;
    juce::ComboBox modInspectorSourceBox;
    juce::ComboBox modMacroAssignSourceBox;
    juce::ComboBox modMacroAssignDestinationBox;
    juce::ComboBox lfo1ShapeBox;
    juce::ComboBox lfo1SyncRateBox;
    juce::ComboBox lfoCurvePresetBox;
    std::array<juce::ComboBox, 8> modSourceBoxes;
    std::array<juce::ComboBox, 8> modDestinationBoxes;
    juce::ToggleButton monoButton;
    juce::ToggleButton sampleEnabledButton;
    juce::ToggleButton sampleReverseButton;
    juce::ToggleButton sampleStutterEnabledButton;
    juce::ToggleButton sequencerEnabledButton;
    juce::ToggleButton sequencerChordMemoryButton;
    juce::ToggleButton fxDistortionEnabledButton;
    juce::ToggleButton fxBitcrushEnabledButton;
    juce::ToggleButton fxPumpEnabledButton;
    juce::ToggleButton fxTremoloEnabledButton;
    juce::ToggleButton fxRingEnabledButton;
    juce::ToggleButton fxCombEnabledButton;
    juce::ToggleButton fxChorusEnabledButton;
    juce::ToggleButton fxDelayEnabledButton;
    juce::ToggleButton fxDelaySyncButton;
    juce::ToggleButton fxReverbEnabledButton;
    juce::ToggleButton fxWidthEnabledButton;
    juce::ToggleButton fxToneEnabledButton;
    juce::ToggleButton fxEqEnabledButton;
    juce::ToggleButton fxPhaserEnabledButton;
    juce::ToggleButton fxFlangerEnabledButton;
    juce::ToggleButton fxGuardEnabledButton;
    juce::ToggleButton randomLockPitchButton;
    juce::ToggleButton randomLockEnvelopeButton;
    juce::ToggleButton randomLockFilterButton;
    juce::ToggleButton randomLockSourceButton;
    juce::ToggleButton randomLockSampleButton;
    juce::ToggleButton randomLockFxButton;
    juce::ToggleButton randomLockOutputButton;
    juce::ToggleButton randomLockSequencerButton;
    juce::ToggleButton lfo1SyncButton;
    juce::ToggleButton lfo1RetriggerButton;

    juce::Slider octaveSlider;
    juce::Slider tuneSlider;
    juce::Slider osc1LevelSlider;
    juce::Slider osc2OctaveSlider;
    juce::Slider osc2TuneSlider;
    juce::Slider osc2LevelSlider;
    juce::Slider subLevelSlider;
    juce::Slider noiseLevelSlider;
    juce::Slider oscWarpSlider;
    juce::Slider unisonVoicesSlider;
    juce::Slider unisonDetuneSlider;
    juce::Slider unisonBlendSlider;
    juce::Slider unisonSpreadSlider;
    juce::Slider glideSlider;
    juce::Slider macroToneSlider;
    juce::Slider macroDirtSlider;
    juce::Slider macroMotionSlider;
    juce::Slider macroSpaceSlider;
    juce::Slider macroWeightSlider;
    juce::Slider macroBounceSlider;
    juce::Slider macroWarpSlider;
    juce::Slider macroThrowSlider;
    juce::Slider lfo1RateSlider;
    juce::Slider lfo1DepthSlider;
    juce::Slider lfo1PhaseSlider;
    std::array<juce::Slider, 8> lfoCurveSliders;
    juce::Slider modEnv1AttackSlider;
    juce::Slider modEnv1DecaySlider;
    juce::Slider modEnv1SustainSlider;
    juce::Slider modEnv1ReleaseSlider;
    juce::Slider modEnv1DepthSlider;
    std::array<juce::Slider, 8> modAmountSliders;
    juce::Slider modMacroAssignAmountSlider;
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
    juce::Slider samplePitchRampSlider;
    juce::Slider sampleGainSlider;
    juce::Slider sampleMixSlider;
    juce::Slider sampleStutterRepeatsSlider;
    juce::Slider sequencerRootSlider;
    juce::Slider sequencerGateSlider;
    juce::Slider sequencerSwingSlider;
    juce::Slider sequencerChordStrumSlider;
    juce::Slider sequencerAccentSlider;
    juce::Slider sequencerOctaveSlider;
    juce::Slider sequencerProbabilitySlider;
    juce::Slider sequencerRandomSlider;
    juce::Slider fxDistortionAmountSlider;
    juce::Slider fxBitcrushBitsSlider;
    juce::Slider fxBitcrushDownsampleSlider;
    juce::Slider fxBitcrushMixSlider;
    juce::Slider fxPumpDepthSlider;
    juce::Slider fxPumpShapeSlider;
    juce::Slider fxPumpPhaseSlider;
    juce::Slider fxTremoloDepthSlider;
    juce::Slider fxTremoloPanSlider;
    juce::Slider fxTremoloShapeSlider;
    juce::Slider fxTremoloPhaseSlider;
    juce::Slider fxRingFrequencySlider;
    juce::Slider fxRingDepthSlider;
    juce::Slider fxRingMixSlider;
    juce::Slider fxRingBiasSlider;
    juce::Slider fxCombFrequencySlider;
    juce::Slider fxCombFeedbackSlider;
    juce::Slider fxCombDampingSlider;
    juce::Slider fxCombMixSlider;
    juce::Slider fxChorusRateSlider;
    juce::Slider fxChorusDepthSlider;
    juce::Slider fxChorusMixSlider;
    juce::Slider fxDelayTimeSlider;
    juce::Slider fxDelayFeedbackSlider;
    juce::Slider fxDelayMixSlider;
    juce::Slider fxReverbSizeSlider;
    juce::Slider fxReverbDampingSlider;
    juce::Slider fxReverbMixSlider;
    juce::Slider fxWidthAmountSlider;
    juce::Slider fxWidthMonoCutoffSlider;
    juce::Slider fxToneTiltSlider;
    juce::Slider fxToneLowCutSlider;
    juce::Slider fxEqLowGainSlider;
    juce::Slider fxEqMidGainSlider;
    juce::Slider fxEqHighGainSlider;
    juce::Slider fxEqTrimSlider;
    juce::Slider fxPhaserRateSlider;
    juce::Slider fxPhaserDepthSlider;
    juce::Slider fxPhaserMixSlider;
    juce::Slider fxFlangerRateSlider;
    juce::Slider fxFlangerDepthSlider;
    juce::Slider fxFlangerFeedbackSlider;
    juce::Slider fxFlangerMixSlider;
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
    juce::Label oscWarpLabel;
    juce::Label unisonVoicesLabel;
    juce::Label unisonDetuneLabel;
    juce::Label unisonBlendLabel;
    juce::Label unisonSpreadLabel;
    juce::Label glideLabel;
    juce::Label macroToneLabel;
    juce::Label macroDirtLabel;
    juce::Label macroMotionLabel;
    juce::Label macroSpaceLabel;
    juce::Label macroWeightLabel;
    juce::Label macroBounceLabel;
    juce::Label macroWarpLabel;
    juce::Label macroThrowLabel;
    juce::Label lfo1RateLabel;
    juce::Label lfo1DepthLabel;
    juce::Label lfo1PhaseLabel;
    juce::Label modEnv1AttackLabel;
    juce::Label modEnv1DecayLabel;
    juce::Label modEnv1SustainLabel;
    juce::Label modEnv1ReleaseLabel;
    juce::Label modEnv1DepthLabel;
    std::array<juce::Label, 8> modAmountLabels;
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
    juce::Label samplePitchRampLabel;
    juce::Label sampleGainLabel;
    juce::Label sampleMixLabel;
    juce::Label sampleStutterRepeatsLabel;
    juce::Label sequencerRootLabel;
    juce::Label sequencerGateLabel;
    juce::Label sequencerSwingLabel;
    juce::Label sequencerChordStrumLabel;
    juce::Label sequencerAccentLabel;
    juce::Label sequencerOctaveLabel;
    juce::Label sequencerProbabilityLabel;
    juce::Label sequencerRandomLabel;
    juce::Label fxDistortionAmountLabel;
    juce::Label fxBitcrushBitsLabel;
    juce::Label fxBitcrushDownsampleLabel;
    juce::Label fxBitcrushMixLabel;
    juce::Label fxPumpDepthLabel;
    juce::Label fxPumpShapeLabel;
    juce::Label fxPumpPhaseLabel;
    juce::Label fxTremoloDepthLabel;
    juce::Label fxTremoloPanLabel;
    juce::Label fxTremoloShapeLabel;
    juce::Label fxTremoloPhaseLabel;
    juce::Label fxRingFrequencyLabel;
    juce::Label fxRingDepthLabel;
    juce::Label fxRingMixLabel;
    juce::Label fxRingBiasLabel;
    juce::Label fxCombFrequencyLabel;
    juce::Label fxCombFeedbackLabel;
    juce::Label fxCombDampingLabel;
    juce::Label fxCombMixLabel;
    juce::Label fxChorusRateLabel;
    juce::Label fxChorusDepthLabel;
    juce::Label fxChorusMixLabel;
    juce::Label fxDelayTimeLabel;
    juce::Label fxDelayFeedbackLabel;
    juce::Label fxDelayMixLabel;
    juce::Label fxReverbSizeLabel;
    juce::Label fxReverbDampingLabel;
    juce::Label fxReverbMixLabel;
    juce::Label fxWidthAmountLabel;
    juce::Label fxWidthMonoCutoffLabel;
    juce::Label fxToneTiltLabel;
    juce::Label fxToneLowCutLabel;
    juce::Label fxEqLowGainLabel;
    juce::Label fxEqMidGainLabel;
    juce::Label fxEqHighGainLabel;
    juce::Label fxEqTrimLabel;
    juce::Label fxPhaserRateLabel;
    juce::Label fxPhaserDepthLabel;
    juce::Label fxPhaserMixLabel;
    juce::Label fxFlangerRateLabel;
    juce::Label fxFlangerDepthLabel;
    juce::Label fxFlangerFeedbackLabel;
    juce::Label fxFlangerMixLabel;
    juce::Label fxGuardPushLabel;
    juce::Label fxGuardCeilingLabel;

    juce::TextButton generateButton { "Generate" };
    juce::TextButton mutateButton { "Mutate" };
    juce::TextButton variationButton { "Vary" };
    juce::TextButton wildMutateButton { "Wild" };
    juce::TextButton undoRandomButton { "Undo" };
    juce::TextButton redoRandomButton { "Redo" };
    juce::TextButton recallSnapshotAButton { "A" };
    juce::TextButton captureSnapshotAButton { "Set A" };
    juce::TextButton recallSnapshotBButton { "B" };
    juce::TextButton captureSnapshotBButton { "Set B" };
    juce::TextButton loadSampleButton { "Load" };
    juce::TextButton clearSampleButton { "Clear" };
    juce::TextButton randomCutButton { "Rand Cut" };
    juce::TextButton ukgChopButton { "UKG Chop" };
    std::array<juce::TextButton, 8> sampleSliceButtons;
    juce::TextButton sampleSliceStoreButton { "Store" };
    juce::TextButton sampleSliceRecallButton { "Recall" };
    juce::TextButton sampleSliceDiceButton { "Dice" };
    juce::TextButton sampleSliceReverseEditButton { "Rev" };
    juce::TextButton randomSequencerButton { "Rand Seq" };
    juce::TextButton mutateSequencerButton { "Vary" };
    juce::TextButton undoSequencerButton { "Undo" };
    juce::TextButton clearSequencerButton { "Clear" };
    juce::TextButton bassPatternButton { "Bass" };
    juce::TextButton stabPatternButton { "Stab" };
    juce::TextButton ukgPatternButton { "UKG" };
    juce::TextButton applyPatternButton { "Apply" };
    juce::TextButton copySequencerButton { "Copy" };
    juce::TextButton rotateSequencerLeftButton { "Rot <" };
    juce::TextButton rotateSequencerRightButton { "Rot >" };
    juce::TextButton exportSequencerMidiButton { "MIDI" };
    juce::TextButton applyGrooveTransformButton { "Shape" };
    juce::TextButton homeTabButton { "HOME" };
    juce::TextButton synthTabButton { "SYNTH" };
    juce::TextButton labTabButton { "LAB" };
    juce::TextButton modTabButton { "MOD" };
    juce::TextButton sampleTabButton { "SAMPLE" };
    juce::TextButton sequencerTabButton { "SEQ" };
    juce::TextButton effectsTabButton { "FX" };
    juce::TextButton libraryTabButton { "LIBRARY" };
    juce::TextButton selectedControlAddModButton { "MOD+" };
    juce::TextButton selectedControlOpenModButton { "MOD" };
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
    juce::TextButton auditionPresetButton { "Audition" };
    juce::TextButton refreshPresetsButton { "Refresh" };
    juce::TextButton favoritePresetButton { "Fav" };
    juce::TextButton fxMoveUpButton { "Up" };
    juce::TextButton fxMoveDownButton { "Down" };
    juce::TextButton fxResetOrderButton { "Reset" };
    juce::TextButton fxRemoveButton { "Remove" };
    juce::TextButton fxThrowDelayButton { "Delay Throw" };
    juce::TextButton fxThrowSpaceButton { "Space Throw" };
    juce::TextButton fxThrowPumpButton { "Pump Drop" };
    juce::TextButton fxThrowDryButton { "Throw Off" };
    juce::TextButton fxHoldDelayButton { "Hold Dly" };
    juce::TextButton fxHoldSpaceButton { "Hold Spc" };
    juce::TextButton fxHoldPumpButton { "Hold Pump" };
    juce::TextButton fxMuteDropButton { "Mute Drop" };
    juce::TextButton fxApplyPresetButton { "Load" };
    juce::TextButton modInspectorAddButton { "Add" };
    juce::TextButton modInspectorClearButton { "Clear" };
    juce::TextButton modMacroAssignAddButton { "Add" };
    juce::TextButton modMacroAssignReplaceButton { "Replace" };
    juce::TextButton modMacroAssignClearButton { "Clear" };
    std::array<juce::ToggleButton, 8> modSlotEnabledButtons;
    std::array<juce::TextButton, 8> modSlotDeleteButtons;
    UI::FxRackRow fxToneSlotButton { "Tone" };
    UI::FxRackRow fxEqSlotButton { "EQ" };
    UI::FxRackRow fxDistortionSlotButton { "Drive" };
    UI::FxRackRow fxBitcrushSlotButton { "Crush" };
    UI::FxRackRow fxPumpSlotButton { "Pump" };
    UI::FxRackRow fxTremoloSlotButton { "Trem" };
    UI::FxRackRow fxRingSlotButton { "Ring" };
    UI::FxRackRow fxCombSlotButton { "Comb" };
    UI::FxRackRow fxPhaserSlotButton { "Phaser" };
    UI::FxRackRow fxFlangerSlotButton { "Flanger" };
    UI::FxRackRow fxChorusSlotButton { "Chorus" };
    UI::FxRackRow fxDelaySlotButton { "Delay" };
    UI::FxRackRow fxReverbSlotButton { "Reverb" };
    UI::FxRackRow fxWidthSlotButton { "Width" };
    UI::FxRackRow fxGuardSlotButton { "Guard" };
    juce::TextButton keyboardOctaveDownButton { "Oct -" };
    juce::TextButton keyboardOctaveUpButton { "Oct +" };
    juce::TextButton keyboardPanicButton { "Panic" };
    juce::Label keyboardRangeLabel;
    juce::Label fxRackStatusLabel;
    std::array<juce::Label, 14> modSourceRows;
    std::array<juce::Label, 8> modSlotRows;
    juce::TextEditor presetNameEditor;
    juce::TextEditor presetSearchEditor;
    juce::TextEditor presetAuthorEditor;
    juce::ListBox presetBrowserList { "Preset Browser" };
    UI::OutputMeter outputMeter;
    UI::LowEndAssistant lowEndAssistant;
    juce::MidiKeyboardComponent pianoKeyboard;
    UI::ModCurveDisplay lfoCurveDisplay;
    UI::PumpCurveDisplay pumpCurveDisplay;
    UI::SampleWaveformDisplay sampleWaveformDisplay;
    UI::StepSequencerGrid sequencerGrid;
    UI::XYMacroPad performanceXYPad;
    std::array<UI::ModMatrixRow, 8> modMatrixRows;
    int activePresetAuditionNote = -1;
    double presetAuditionNoteOffMs = 0.0;
    juce::String fxRackStatusOverride;
    double fxRackStatusOverrideUntilMs = 0.0;
    juce::String sampleWaveformKey;
    MomentaryFxAction activeMomentaryFxAction = MomentaryFxAction::none;
    FxMomentarySnapshot fxMomentarySnapshot;
    std::vector<NateVSTAudioProcessor::PresetInfo> visiblePresetBrowserPresets;
    bool ignorePresetBrowserSelection = false;
    juce::String selectedControlName;
    juce::String selectedControlParameterID;
    double selectedControlPlainValue = 0.0;
    size_t selectedSampleSliceIndex = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::String getNameForRow(int rowNumber) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void configureHorizontalSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void configureCompactHorizontalSlider(juce::Slider& slider, const juce::String& parameterID);
    void configureSectionLabel(juce::Label& label, const juce::String& text);
    juce::Rectangle<int> layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<juce::Component*> components);
    void chooseSampleFile();
    void exportSequencerMidiClip();
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
    void updateLfoCurveDisplay();
    void updatePumpCurveDisplay();
    void updateHostSyncStatus();
    void updateModMatrixRows();
    void updateModInspectorStatus();
    void updateMacroAssignmentEditorStatus();
    void updateModDestinationIndicators();
    void updateSelectedControlInspector(const juce::String& labelText, const juce::String& parameterID, double plainValue);
    void updateSelectedControlActionState();
    void addModRouteForSelectedControl();
    void focusSelectedControlModDestination();
    juce::String formattedParameterValue(const juce::String& parameterID, double plainValue) const;
    juce::String modulationSummaryForParameter(const juce::String& parameterID) const;
    int modulationDestinationIndexForParameter(const juce::String& parameterID) const;
    void applyLfoCurvePreset(int presetId);
    void updateOutputMeter();
    void updateLowEndAssistant();
    void updatePerformanceSnapshotButtons();
    void updatePerformanceXYPad();
    void updateSequencerGridContext();
    void selectSampleSlice(size_t sliceIndex);
    void updateSampleSliceButtons();
    void updateSampleSliceEditorStatus();
    void applySampleSliceStyleDefaults(size_t sliceIndex);
    void captureCurrentSampleSliceSettings(size_t sliceIndex, bool markCustom);
    void recallSampleSliceSettings(size_t sliceIndex);
    void storeSelectedSampleSliceSettings();
    void recallSelectedSampleSliceSettings();
    void randomizeSelectedSampleSliceSettings();
    void toggleSelectedSampleSliceReverse();
    bool sampleSliceHasCustomSettings(size_t sliceIndex) const;
    void updateSampleWaveformDisplay();
    void timerCallback() override;
    void refreshPresetList();
    void saveCurrentPreset();
    void loadSelectedPreset();
    int selectedRandomMutationScope() const;
    void loadPresetByOffset(int offset);
    void auditionSelectedPreset();
    void releasePresetAuditionNote();
    void toggleFavoritePreset();
    void setSelectedPresetRating();
    void updateFavoritePresetButton();
    void shiftKeyboardOctave(int semitones);
    void updateKeyboardRangeLabel();
    void addFxModule(FxModule module);
    void removeSelectedFxModule();
    void selectFxModule(FxModule module);
    void moveSelectedFxModule(int direction);
    void resetFxModuleOrder();
    void applyDelayThrow();
    void applySpaceThrow();
    void applyPumpDrop();
    void clearFxThrows();
    void beginMomentaryFxAction(MomentaryFxAction action);
    void endMomentaryFxAction(MomentaryFxAction action);
    void applyMomentaryMuteDrop();
    void updateFxPresetBox(bool force = false);
    void applySelectedFxPreset();
    void applyFxModulePreset(FxModule module, int presetId);
    void setModInspectorDestination(int destinationIndex);
    void addInspectedModRoute();
    void addMacroAssignment(bool replaceExisting);
    void clearSelectedMacroAssignments();
    void deleteModRoute(size_t slotIndex);
    void clearInspectedModRoutes();
    FxMomentarySnapshot captureFxMomentarySnapshot() const;
    void restoreFxMomentarySnapshot(const FxMomentarySnapshot& snapshot);
    void setFxRackStatusOverride(const juce::String& message);
    void updateFxRackControls();
    std::array<FxModule, 15> fxDefaultModuleOrder() const;
    std::array<FxModule, 15> fxModuleOrder() const;
    void setFxModuleOrder(const std::array<FxModule, 15>& order);
    int fxOrderPosition(FxModule module) const;
    int fxModuleIndex(FxModule module) const;
    FxModule fxModuleFromIndex(int index) const;
    bool isFxModuleEnabled(FxModule module) const;
    bool shouldShowFxModule(FxModule module) const;
    juce::String fxEnabledParameterID(FxModule module) const;
    juce::String fxModuleName(FxModule module) const;
    juce::String fxModuleSummary(FxModule module) const;
    UI::FxRackRow& fxSlotButton(FxModule module);
    float readPlainParameterValue(const juce::String& parameterID, float fallback) const;
    void setPlainParameterValue(const juce::String& parameterID, float plainValue);
    int selectedMacroAssignmentSourceIndex() const;
    int selectedMacroAssignmentDestinationIndex() const;

    Panel activePanel = Panel::home;
    FxModule selectedFxModule = FxModule::guard;
    FxModule fxPresetBoxModule = FxModule::guard;
    float displayedPeakLeft = 0.0f;
    float displayedPeakRight = 0.0f;
    float displayedRmsLeft = 0.0f;
    float displayedRmsRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessorEditor)
};
