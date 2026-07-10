#pragma once

#include "PluginProcessor.h"
#include "UI/ClubMonitorDisplay.h"
#include "UI/ControlStatusStrip.h"
#include "UI/FilterResponseDisplay.h"
#include "UI/FocusOverlayPanel.h"
#include "UI/FxPerformanceControls.h"
#include "UI/FxRackOrderControls.h"
#include "UI/FxRackRow.h"
#include "UI/HouseLayerRackDisplay.h"
#include "UI/HomeOverviewDisplay.h"
#include "UI/HomeSoundStage.h"
#include "UI/HomeSignalFlowDisplay.h"
#include "UI/HomeSessionDisplay.h"
#include "UI/LfoCurveToolStrip.h"
#include "UI/LookAndFeel.h"
#include "UI/LowEndAssistant.h"
#include "UI/MacroAssignmentActions.h"
#include "UI/MacroAssignmentPad.h"
#include "UI/MacroPerformanceMap.h"
#include "UI/ModInspectorActions.h"
#include "UI/ModCurveDisplay.h"
#include "UI/ModMatrixRow.h"
#include "UI/ModRouteMapDisplay.h"
#include "UI/ModSourceMeter.h"
#include "UI/OscillatorLaneOverview.h"
#include "UI/OutputMeter.h"
#include "UI/OutputOscilloscopeDisplay.h"
#include "UI/OutputSpectrumDisplay.h"
#include "UI/PageButtonStrip.h"
#include "UI/PanelTabBar.h"
#include "UI/PerformanceKeyboard.h"
#include "UI/PresetCompareActions.h"
#include "UI/PresetCrateMapDisplay.h"
#include "UI/PresetLibrarySummary.h"
#include "UI/PresetPrimaryActions.h"
#include "UI/PresetQuickFilterBar.h"
#include "UI/PresetSaveSummary.h"
#include "UI/PumpCurveDisplay.h"
#include "UI/RandomMorphPad.h"
#include "UI/RandomCandidateExplorer.h"
#include "UI/HostSyncStatusLabel.h"
#include "UI/SampleChopHeader.h"
#include "UI/SampleChopPanel.h"
#include "UI/SampleFileActions.h"
#include "UI/SamplePlaybackControls.h"
#include "UI/SampleRangeControls.h"
#include "UI/SampleRecorderPanel.h"
#include "UI/SampleRecipeActions.h"
#include "UI/SampleShapeControls.h"
#include "UI/SampleSourceControls.h"
#include "UI/SampleStatusLabel.h"
#include "UI/SampleWaveformDisplay.h"
#include "UI/SequencerEnabledButton.h"
#include "UI/SequencerGrooveControls.h"
#include "UI/SequencerLaneViewControls.h"
#include "UI/SequencerSceneChainControls.h"
#include "UI/SequencerSceneControls.h"
#include "UI/SequencerPatternControls.h"
#include "UI/SequencerRateControls.h"
#include "UI/SequencerRootControls.h"
#include "UI/SequencerStepEditor.h"
#include "UI/SequencerTransformControls.h"
#include "UI/SequencerUtilityActions.h"
#include "UI/StereoFieldDisplay.h"
#include "UI/StepSequencerGrid.h"
#include "UI/WavetableDisplay.h"
#include "UI/WavetableFrameStrip.h"
#include "UI/XYMacroPad.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <vector>

class NateVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           public juce::FileDragAndDropTarget,
                                           public juce::DragAndDropContainer,
                                           public juce::DragAndDropTarget,
                                           private juce::KeyListener,
                                           private juce::Timer,
                                           private juce::ListBoxModel
{
public:
    explicit NateVSTAudioProcessorEditor(NateVSTAudioProcessor& processor);
    ~NateVSTAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    using juce::Component::keyPressed;
    using juce::Component::keyStateChanged;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    juce::StringArray runLayoutAudit();

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
        library,
        info
    };

    enum class RandomLabPage
    {
        generate = 0,
        mutate,
        recipe,
        history,
        save
    };

    enum class ModWorkflowPage
    {
        sources = 0,
        matrix,
        macros,
        curves
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

    enum class LfoCurveTool
    {
        invert,
        reverse,
        smooth,
        quantize,
        randomize,
        garage
    };

    enum class FocusOverlay
    {
        none,
        macroEditor,
        sampleChopEditor,
        sourceLayerEditor,
        sequencerEditor
    };

    enum class WavetableFrameCardAction
    {
        copy,
        paste,
        storeMorph
    };

    enum class WavetableStackTransform
    {
        reverse,
        rotateLeft,
        rotateRight,
        smooth,
        emphasise
    };

    enum class WavetableFrameSlotEdit
    {
        duplicateAfter,
        deleteFrame,
        moveLeft,
        moveRight
    };

    struct FxMomentarySnapshot
    {
        std::array<float, 34> values {};
        FxModule selectedModule = FxModule::guard;
        bool valid = false;
    };

    struct ModulationMenuTarget
    {
        juce::Component* component = nullptr;
        juce::String labelText;
        juce::String parameterID;
    };

    struct PresetAuditionNote
    {
        int note = -1;
        double startMs = 0.0;
        double stopMs = 0.0;
        float velocity = 0.0f;
        bool started = false;
        bool stopped = false;
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
    juce::Label modLfo2Label;
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
    juce::Label sampleShapeLabel;
    juce::Label sequencerSectionLabel;
    UI::HostSyncStatusLabel hostSyncStatusLabel;
    juce::Label futureSectionLabel;
    juce::Label librarySectionLabel;
    juce::Label libraryFindLabel;
    juce::Label libraryBrowserLabel;
    juce::Label librarySaveLabel;
    juce::Label libraryInspectorLabel;
    juce::Label infoSectionLabel;
    juce::Label infoAboutLabel;
    juce::Label infoWorkflowLabel;
    juce::Label infoDetailsLabel;
    juce::Label infoFocusLabel;
    UI::SampleStatusLabel sampleStatusLabel;
    juce::Label presetStatusLabel;
    juce::Label presetBrowserHeaderLabel;
    juce::Label randomStatusLabel;
    juce::Label randomRecipeInfoLabel;
    juce::Label performanceStatusLabel;
    juce::Label focusOverlayTitleLabel;

    juce::ComboBox waveformBox;
    juce::ComboBox osc2WaveBox;
    juce::ComboBox wavetableToolBox;
    juce::ComboBox wavetableDrawModeBox;
    juce::ComboBox noiseTypeBox;
    juce::ComboBox oscWarpModeBox;
    juce::ComboBox oscWarpBModeBox;
    juce::ComboBox osc2WarpModeBox;
    juce::ComboBox osc2WarpBModeBox;
    juce::ComboBox oscCrossModModeBox;
    juce::ComboBox filterModeBox;
    juce::ComboBox filterCharacterBox;
    juce::ComboBox filterSlopeBox;
    juce::ComboBox recipeBox;
    juce::ComboBox randomScopeBox;
    juce::ComboBox randomSectionActionBox;
    juce::ComboBox randomLockActionBox;
    UI::SequencerGrooveControls sequencerGrooveControls;
    UI::SequencerLaneViewControls sequencerLaneViewControls;
    UI::SequencerTransformControls sequencerTransformControls;
    UI::SampleSourceControls sampleSourceControls;
    UI::SampleRangeControls sampleRangeControls;
    UI::SampleShapeControls sampleShapeControls;
    UI::SamplePlaybackControls samplePlaybackControls;
    juce::ComboBox presetBox;
    juce::ComboBox presetCategoryBox;
    juce::ComboBox presetFilterBox;
    juce::ComboBox presetTagBox;
    juce::ComboBox presetSortBox;
    juce::ComboBox presetBrowserPackFilterBox;
    juce::ComboBox presetRatingBox;
    juce::ComboBox candidateRatingBox;
    juce::ComboBox presetPackBox;
    juce::ComboBox presetKeyBox;
    juce::ComboBox presetBpmBox;
    juce::ComboBox infoTopicBox;
    juce::ComboBox fxAddBox;
    juce::ComboBox fxPresetBox;
    juce::ComboBox fxDistortionModeBox;
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
    juce::ComboBox lfo2ShapeBox;
    juce::ComboBox lfo2SyncRateBox;
    juce::ComboBox lfoCurvePresetBox;
    juce::ComboBox lfoCurveActionBox;
    std::array<juce::ComboBox, 8> modSourceBoxes;
    std::array<juce::ComboBox, 8> modDestinationBoxes;
    juce::ToggleButton monoButton;
    UI::SequencerEnabledButton sequencerEnabledButton;
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
    juce::ToggleButton lfo2SyncButton;
    juce::ToggleButton lfo2RetriggerButton;

    juce::Slider octaveSlider;
    juce::Slider tuneSlider;
    juce::Slider osc1LevelSlider;
    juce::Slider osc2OctaveSlider;
    juce::Slider osc2TuneSlider;
    juce::Slider osc2LevelSlider;
    juce::Slider subLevelSlider;
    juce::Slider noiseLevelSlider;
    juce::Slider noiseDecaySlider;
    juce::Slider oscWarpSlider;
    juce::Slider oscWarpBSlider;
    juce::Slider osc2WarpSlider;
    juce::Slider osc2WarpBSlider;
    juce::Slider oscCrossModAmountSlider;
    juce::Slider oscWavetablePositionSlider;
    juce::Slider osc2WavetablePositionSlider;
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
    juce::Slider lfo2RateSlider;
    juce::Slider lfo2DepthSlider;
    juce::Slider lfo2PhaseSlider;
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
    std::array<juce::Slider, 7> randomSectionIntensitySliders;
    juce::Slider sequencerRootSlider;
    juce::Slider sequencerGateSlider;
    juce::Slider sequencerSwingSlider;
    juce::Slider sequencerChordStrumSlider;
    juce::Slider sequencerAccentSlider;
    juce::Slider sequencerOctaveSlider;
    juce::Slider sequencerProbabilitySlider;
    juce::Slider sequencerRandomSlider;
    juce::Slider sequencerLockDepthSlider;
    juce::Slider fxDistortionAmountSlider;
    juce::Slider fxDistortionBassSafeSlider;
    juce::Slider fxDistortionLowBandSlider;
    juce::Slider fxDistortionMidBandSlider;
    juce::Slider fxDistortionHighBandSlider;
    juce::Slider fxDistortionMixSlider;
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
    juce::Slider fxSendDelaySlider;
    juce::Slider fxReverbSizeSlider;
    juce::Slider fxReverbDampingSlider;
    juce::Slider fxReverbMixSlider;
    juce::Slider fxSendReverbSlider;
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
    juce::Slider fxGuardGlueSlider;
    juce::Slider fxGuardPunchSlider;
    juce::Slider fxGuardClipMixSlider;
    juce::Slider fxGuardCeilingSlider;

    juce::Label octaveLabel;
    juce::Label tuneLabel;
    juce::Label osc1LevelLabel;
    juce::Label osc2OctaveLabel;
    juce::Label osc2TuneLabel;
    juce::Label osc2LevelLabel;
    juce::Label subLevelLabel;
    juce::Label noiseLevelLabel;
    juce::Label noiseDecayLabel;
    juce::Label oscWarpLabel;
    juce::Label oscWarpBLabel;
    juce::Label osc2WarpLabel;
    juce::Label osc2WarpBLabel;
    juce::Label oscCrossModAmountLabel;
    juce::Label oscWavetablePositionLabel;
    juce::Label osc2WavetablePositionLabel;
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
    juce::Label lfo2RateLabel;
    juce::Label lfo2DepthLabel;
    juce::Label lfo2PhaseLabel;
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
    std::array<juce::Label, 7> randomSectionIntensityLabels;
    juce::Label sequencerRootLabel;
    juce::Label sequencerGateLabel;
    juce::Label sequencerSwingLabel;
    juce::Label sequencerChordStrumLabel;
    juce::Label sequencerAccentLabel;
    juce::Label sequencerOctaveLabel;
    juce::Label sequencerProbabilityLabel;
    juce::Label sequencerRandomLabel;
    juce::Label sequencerLockDepthLabel;
    juce::Label fxDistortionAmountLabel;
    juce::Label fxDistortionBassSafeLabel;
    juce::Label fxDistortionLowBandLabel;
    juce::Label fxDistortionMidBandLabel;
    juce::Label fxDistortionHighBandLabel;
    juce::Label fxDistortionMixLabel;
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
    juce::Label fxSendDelayLabel;
    juce::Label fxReverbSizeLabel;
    juce::Label fxReverbDampingLabel;
    juce::Label fxReverbMixLabel;
    juce::Label fxSendReverbLabel;
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
    juce::Label fxGuardGlueLabel;
    juce::Label fxGuardPunchLabel;
    juce::Label fxGuardClipMixLabel;
    juce::Label fxGuardCeilingLabel;

    juce::TextButton generateButton { "Generate" };
    juce::TextButton mutateButton { "Mutate" };
    juce::TextButton variationButton { "Vary" };
    juce::TextButton wildMutateButton { "Wild" };
    juce::TextButton undoRandomButton { "Undo" };
    juce::TextButton redoRandomButton { "Redo" };
    UI::PageButtonStrip randomLabPageStrip;
    std::array<juce::TextButton, 7> randomSectionRollButtons;
    std::array<juce::TextButton, 4> randomCandidateButtons;
    std::array<juce::TextButton, 4> randomCandidateAuditionButtons;
    juce::TextButton promoteCandidateAButton { "To A" };
    juce::TextButton promoteCandidateBButton { "To B" };
    juce::TextButton recallSnapshotAButton { "A" };
    juce::TextButton captureSnapshotAButton { "Set A" };
    juce::TextButton recallSnapshotBButton { "B" };
    juce::TextButton captureSnapshotBButton { "Set B" };
    juce::TextButton recallSnapshotCButton { "C" };
    juce::TextButton captureSnapshotCButton { "Set C" };
    juce::TextButton recallSnapshotDButton { "D" };
    juce::TextButton captureSnapshotDButton { "Set D" };
    UI::SampleFileActions sampleFileActions;
    UI::SampleChopHeader sampleChopHeader;
    UI::SampleRecipeActions sampleRecipeActions;
    UI::SequencerPatternControls sequencerPatternControls;
    UI::SequencerUtilityActions sequencerUtilityActions;
    UI::SequencerSceneChainControls sequencerSceneChainControls;
    UI::SequencerSceneControls sequencerSceneControls;
    UI::SequencerRootControls sequencerRootControls;
    UI::PanelTabBar panelTabBar;
    UI::ControlStatusStrip controlStatusStrip;
    juce::TextButton sineWaveButton { "Sine" };
    juce::TextButton sawWaveButton { "Saw" };
    juce::TextButton squareWaveButton { "Square" };
    juce::TextButton triangleWaveButton { "Tri" };
    juce::TextButton wavetableWaveButton { "WT" };
    juce::TextButton organWaveButton { "Org" };
    juce::TextButton housePianoWaveButton { "Pno" };
    juce::TextButton customWaveButton { "Edit" };
    juce::TextButton waveEditorFocusButton { "Edit Wave" };
    juce::TextButton waveViewModeButton { "3D" };
    juce::TextButton sourceRandomizeButton { "Dice" };
    juce::TextButton sourceCopyButton { "O1 > O2" };
    juce::ToggleButton sourceLockButton { "Lock" };
    juce::TextButton osc2SineWaveButton { "Sine" };
    juce::TextButton osc2SawWaveButton { "Saw" };
    juce::TextButton osc2SquareWaveButton { "Square" };
    juce::TextButton osc2TriangleWaveButton { "Tri" };
    juce::TextButton osc2WavetableWaveButton { "WT" };
    juce::TextButton osc2OrganWaveButton { "Org" };
    juce::TextButton osc2HousePianoWaveButton { "Pno" };
    juce::TextButton osc2CustomWaveButton { "Edit" };
    juce::TextButton lowpassFilterButton { "LP" };
    juce::TextButton bandpassFilterButton { "BP" };
    juce::TextButton highpassFilterButton { "HP" };
    UI::SequencerRateControls sequencerRateControls;
    juce::TextButton previousPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton refreshPresetsButton { "Refresh" };
    UI::PresetPrimaryActions presetPrimaryActions;
    UI::PresetCompareActions presetCompareActions;
    UI::PresetQuickFilterBar presetQuickFilterBar;
    UI::PageButtonStrip modWorkflowStrip;
    juce::TextButton candidateFavoriteButton { "Star" };
    juce::TextButton saveCandidateButton { "Save Slot" };
    UI::FxRackOrderControls fxRackOrderControls;
    UI::FxPerformanceControls fxPerformanceControls;
    juce::TextButton fxApplyPresetButton { "Load" };
    UI::ModInspectorActions modInspectorActions;
    UI::MacroAssignmentActions macroAssignmentActions;
    juce::TextButton homeMacroExpandButton { ">" };
    juce::TextButton headerViewButton { "View" };
    juce::TextButton headerFavoriteButton { "Fav" };
    juce::TextButton homeAnalyzerButton { "Analysis" };
    juce::TextButton homeStageModeButton { "3D" };
    juce::TextButton modMacroExpandButton { ">" };
    juce::TextButton sourceLayerExpandButton { ">" };
    juce::TextButton sequencerExpandButton { ">" };
    juce::TextButton focusOverlayCloseButton { "Close" };
    juce::TextButton sourceFrameTargetOsc1Button { "O1" };
    juce::TextButton sourceFrameTargetOsc2Button { "O2" };
    juce::TextButton sourceFramePreviousButton { "<" };
    juce::TextButton sourceFrameNextButton { ">" };
    juce::TextButton sourceFrameEvolveButton { "Evolve" };
    juce::TextButton sourceFrameCopyButton { "Copy" };
    juce::TextButton sourceFramePasteButton { "Paste" };
    juce::TextButton sourceFrameFillButton { "Fill" };
    juce::TextButton sourceFrameInterpolateButton { "Interp" };
    juce::TextButton sourceStackReverseButton { "Rev" };
    juce::TextButton sourceStackRotateLeftButton { "<" };
    juce::TextButton sourceStackRotateRightButton { ">" };
    juce::TextButton sourceStackSmoothButton { "Smooth" };
    juce::TextButton sourceStackEmphasiseButton { "Punch" };
    juce::TextButton sourceFrameDuplicateSlotButton { "Dup" };
    juce::TextButton sourceFrameDeleteSlotButton { "Del" };
    juce::TextButton sourceFrameMoveLeftButton { "Move <" };
    juce::TextButton sourceFrameMoveRightButton { "Move >" };
    UI::LfoCurveToolStrip lfoCurveToolStrip;
    juce::TextButton infoOpenLabButton { "Open Lab" };
    juce::TextButton infoOpenModButton { "Open MOD" };
    juce::TextButton infoOpenFxButton { "Open FX" };
    juce::TextButton infoOpenLibraryButton { "Open Library" };
    std::array<juce::ToggleButton, 8> modSlotEnabledButtons;
    std::array<juce::TextButton, 8> modSlotDuplicateButtons;
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
    juce::TextButton keyboardHomeButton { "Reset" };
    juce::TextButton keyboardPanicButton { "All Off" };
    juce::Label keyboardRangeLabel;
    juce::Label keyboardPerformanceLabel;
    juce::Label fxRackStatusLabel;
    std::array<UI::ModSourceMeter, 20> modSourceRows;
    std::array<juce::Label, 8> modSlotRows;
    juce::TextEditor presetNameEditor;
    juce::TextEditor presetSearchEditor;
    juce::TextEditor presetAuthorEditor;
    juce::TextEditor presetNotesEditor;
    juce::ComboBox presetNotesTemplateBox;
    juce::TextEditor randomCandidateDetailEditor;
    juce::TextEditor infoAboutEditor;
    juce::TextEditor infoWorkflowEditor;
    juce::TextEditor infoDetailEditor;
    juce::ListBox presetBrowserList { "Preset Browser" };
    UI::OutputMeter outputMeter;
    UI::OutputOscilloscopeDisplay outputOscilloscopeDisplay;
    UI::OutputSpectrumDisplay outputSpectrumDisplay;
    UI::StereoFieldDisplay stereoFieldDisplay;
    UI::ClubMonitorDisplay clubMonitorDisplay;
    UI::HomeOverviewDisplay homeOverviewDisplay;
    UI::HomeSoundStage homeSoundStage;
    UI::HomeSignalFlowDisplay homeSignalFlowDisplay;
    UI::HomeSessionDisplay homeSessionDisplay;
    UI::PresetCrateMapDisplay presetCrateMapDisplay;
    UI::PresetLibrarySummary presetLibrarySummary;
    UI::PresetSaveSummary presetSaveSummary;
    UI::RandomMorphPad randomMorphPad;
    UI::RandomCandidateExplorer randomCandidateExplorer;
    UI::LowEndAssistant lowEndAssistant;
    UI::SampleChopPanel sampleChopPanel;
    UI::SampleRecorderPanel sampleRecorderPanel;
    UI::PerformanceKeyboard pianoKeyboard;
    UI::ModCurveDisplay lfoCurveDisplay;
    UI::PumpCurveDisplay pumpCurveDisplay;
    UI::SampleWaveformDisplay sampleWaveformDisplay;
    UI::SampleWaveformDisplay expandedSampleWaveformDisplay;
    UI::StepSequencerGrid sequencerGrid;
    UI::StepSequencerGrid expandedSequencerGrid;
    UI::SequencerStepEditor sequencerStepEditor;
    UI::WavetableDisplay wavetableDisplay;
    UI::WavetableDisplay expandedWavetableDisplay;
    UI::WavetableFrameStrip sourceLabFrameStrip;
    UI::OscillatorLaneOverview oscillatorLaneOverview;
    UI::HouseLayerRackDisplay houseLayerRackDisplay;
    UI::HouseLayerRackDisplay expandedHouseLayerRackDisplay;
    UI::FilterResponseDisplay filterResponseDisplay;
    UI::FocusOverlayPanel focusOverlayPanel;
    UI::MacroAssignmentPad macroAssignmentPad;
    UI::MacroPerformanceMap macroPerformanceMap;
    UI::MacroAssignmentPad expandedMacroAssignmentPad;
    UI::MacroPerformanceMap expandedMacroPerformanceMap;
    UI::XYMacroPad performanceXYPad;
    UI::ModRouteMapDisplay modRouteMapDisplay;
    std::array<UI::ModMatrixRow, 8> modMatrixRows;
    std::vector<PresetAuditionNote> presetAuditionNotes;
    juce::MemoryBlock presetCompareBeforeSnapshot;
    juce::MemoryBlock presetCompareLoadedSnapshot;
    juce::String presetCompareName;
    bool presetCompareShowingLoaded = true;
    juce::String pendingOverwritePresetName;
    juce::String pendingOverwriteCategory;
    double pendingOverwriteUntilMs = 0.0;
    int activeRandomCandidateAuditionNote = -1;
    int activeRandomCandidateAuditionSlot = -1;
    double randomCandidateAuditionNoteOffMs = 0.0;
    juce::String fxRackStatusOverride;
    double fxRackStatusOverrideUntilMs = 0.0;
    juce::String sampleWaveformKey;
    UI::WavetableDisplay::CustomPointArray wavetableFrameClipboard {};
    bool wavetableFrameClipboardValid = false;
    bool sourceFrameActionTargetOsc2 = false;
    bool sourceFrameActionTargetExplicit = false;
    UI::ThemeId selectedTheme = UI::ThemeId::darkClub;
    bool homeAnalyzerExpanded = true;
    bool homeStagePerspective = true;
    bool wavetablePerspective = true;
    bool keyboardCollapsed = false;
    bool animationsEnabled = true;
    int uiScalePercent = 100;
    std::unique_ptr<juce::PropertiesFile> editorPreferences;
    std::vector<juce::File> sequencerDragMidiFiles;
    int keyboardTypingBaseNote = -1;
    int syncedPianoKeyboardMappingBaseNote = -1;
    std::array<bool, 17> computerKeyboardNotesDown {};
    bool pianoMouseAuditionActive = false;
    bool globalKeyboardListenersInstalled = false;
    MomentaryFxAction activeMomentaryFxAction = MomentaryFxAction::none;
    FxMomentarySnapshot fxMomentarySnapshot;
    std::vector<NateVSTAudioProcessor::PresetInfo> visiblePresetBrowserPresets;
    bool ignorePresetBrowserSelection = false;
    juce::String selectedControlName;
    juce::String selectedControlParameterID;
    double selectedControlPlainValue = 0.0;
    std::vector<ModulationMenuTarget> modulationMenuTargets;
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
    void configureModAmountSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void configureCompactHorizontalSlider(juce::Slider& slider, const juce::String& parameterID);
    void configureRandomSectionSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& parameterID);
    void registerModulationMenuTarget(juce::Component& component, const juce::String& labelText, const juce::String& parameterID);
    const ModulationMenuTarget* findModulationMenuTarget(const juce::Component* component) const;
    int findModRouteAmountIndex(const juce::Component* component) const;
    void beginModSourceDrag(int sourceIndex, juce::Component& sourceComponent);
    bool handleDroppedModSource(int sourceIndex, juce::Point<int> localPosition);
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void showModulationMenuForControl(const ModulationMenuTarget& target, juce::Component& component);
    void showModRouteAmountMenu(size_t slotIndex, juce::Component& component);
    void setModRouteAmount(size_t slotIndex, float amount);
    void applyModRouteShapePreset(size_t slotIndex, int presetId);
    void applyStepLfoPreset(int presetId);
    void resetModRouteShape(size_t slotIndex);
    juce::String modRouteShapeSummary(size_t slotIndex) const;
    void addModRouteForParameter(const juce::String& parameterID, const juce::String& labelText, int sourceIndex);
    void configureSectionLabel(juce::Label& label, const juce::String& text);
    juce::Rectangle<int> layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<juce::Component*> components);
    void chooseSampleFile();
    void exportSequencerMidiClip();
    void exportSequencerSceneChainMidiClip();
    bool beginSequencerMidiDrag(juce::Component& sourceComponent, bool sceneChain);
    void pruneSequencerDragMidiFiles();
    bool revealRecorderExportFile();
    bool beginRecorderTakeDrag(juce::Component& sourceComponent);
    void loadSampleFile(const juce::File& file);
    void updateSampleNameLabel();
    void updateSampleRecorderStatus();
    void setRandomStatus(const juce::String& action);
    void setActivePanel(Panel panel);
    void setActiveRandomLabPage(RandomLabPage page);
    void setActiveModWorkflowPage(ModWorkflowPage page);
    void updatePanelVisibility();
    void updateTabButtons();
    static UI::PanelTabBar::Tab tabForPanel(Panel panel);
    static Panel panelForTab(UI::PanelTabBar::Tab tab);
    static size_t modWorkflowButtonIndexForPage(ModWorkflowPage page);
    static ModWorkflowPage modWorkflowPageForButtonIndex(size_t index);
    const UI::Theme& uiTheme() const noexcept;
    void applyThemeColours();
    void loadEditorPreferences();
    void saveEditorPreferences();
    void showViewMenu();
    void applySelectedTheme();
    int currentKeyboardHeight() const noexcept;
    void openMacroFocusOverlay();
    void openSampleChopFocusOverlay();
    void openSourceLayerFocusOverlay();
    void openSequencerFocusOverlay();
    void closeFocusOverlay();
    void layoutFocusOverlay();
    juce::Rectangle<int> focusOverlayBounds() const;
    void updateRandomLabPageButtons();
    void updateModWorkflowButtons();
    void updateRandomRecipeInfo();
    void updateInfoDetail();
    juce::String infoDetailTextForTopic(int topicId) const;
    void hidePanelComponents();
    void setSliderVisible(juce::Slider& slider, juce::Label& label, bool shouldBeVisible);
    void setChoiceParameter(const juce::String& parameterID, int choiceIndex);
    void updateSegmentedSelectors();
    void updateLfoCurveDisplay();
    void updatePumpCurveDisplay();
    void updateWavetableDisplay();
    void updateOscillatorLaneOverview();
    void updateSourceLabFrameStrip();
    void setSourceFrameActionTarget(bool targetOsc2);
    bool sourceFrameActionTargetIsOsc2() const;
    void updateSourceFrameActionButtons();
    void selectCustomWaveFrame(bool targetOsc2, size_t frameIndex);
    void stepSourceFrameActionTarget(int delta);
    void handleWavetableFrameCardAction(bool targetOsc2, size_t frameIndex, WavetableFrameCardAction action);
    void applySelectedWavetableTool();
    bool wavetableTargetIsOsc2() const;
    size_t currentCustomWaveFrameIndex(bool targetOsc2) const;
    UI::WavetableDisplay::CustomPointArray readCustomWaveFrame(bool targetOsc2, size_t frameIndex) const;
    std::array<UI::WavetableDisplay::CustomPointArray, Parameters::customWaveMorphFrameCount> readCustomWaveFrameSet(bool targetOsc2) const;
    UI::WavetableDisplay::CustomPointArray readMorphedCustomWaveFrame(bool targetOsc2) const;
    void writeCustomWaveFrame(bool targetOsc2, size_t frameIndex, const UI::WavetableDisplay::CustomPointArray& values, const juce::String& editLabel);
    void writeCustomWaveFrameSetWithoutCapture(
        bool targetOsc2,
        const std::array<UI::WavetableDisplay::CustomPointArray, Parameters::customWaveMorphFrameCount>& frames);
    void writeCustomWaveFrameSet(bool targetOsc2,
                                 const std::array<UI::WavetableDisplay::CustomPointArray, Parameters::customWaveMorphFrameCount>& frames,
                                 const juce::String& editLabel,
                                 const juce::String& statusText);
    void importSingleCycleWave(bool targetOsc2);
    void exportSingleCycleWave(bool targetOsc2);
    void importWavetableFrameStack(bool targetOsc2);
    void exportWavetableFrameStack(bool targetOsc2);
    void copyCustomWaveFrameStack(bool sourceOsc2);
    void swapCustomWaveFrameStacks();
    void buildClassicHouseSourceLayers();
    void buildRaveTechnoSourceLayers();
    void copyCurrentCustomWaveFrame(bool targetOsc2);
    void pasteCurrentCustomWaveFrame(bool targetOsc2);
    void duplicateCurrentCustomWaveFrameAcrossStack(bool targetOsc2);
    void interpolateCustomWaveFrameEndpoints(bool targetOsc2);
    void evolveCustomWaveFrameStackFromActiveFrame(bool targetOsc2);
    void applyCustomWaveFrameStackTransform(bool targetOsc2, WavetableStackTransform transform);
    void applyCustomWaveFrameSlotEdit(bool targetOsc2, WavetableFrameSlotEdit edit);
    void storeCustomWaveFrame(bool targetOsc2, size_t frameIndex);
    void loadCustomWaveFrame(bool targetOsc2, size_t frameIndex);
    void bakeCurrentCustomWaveMorph(bool targetOsc2);
    void applySelectedRandomSectionAction();
    void applySelectedRandomLockAction();
    void updateHouseLayerRackDisplay();
    void focusHouseLayer(size_t layerIndex);
    void beginHouseLayerLevelEdit(size_t layerIndex);
    void setHouseLayerLevel(size_t layerIndex, float level);
    void updateFilterResponseDisplay();
    void updateHostSyncStatus();
    void updateModMatrixRows();
    void updateModInspectorStatus();
    void updateMacroAssignmentEditorStatus();
    void updateMacroAssignmentPad();
    void updateModDestinationIndicators();
    float modulationSourceActivityForUi(int sourceIndex) const;
    void updateSelectedControlInspector(const juce::String& labelText, const juce::String& parameterID, double plainValue);
    void updateSelectedControlActionState();
    void captureGlobalEdit(const juce::String& label);
    void triggerGlobalUndo();
    void triggerGlobalRedo();
    void refreshGlobalEditControls();
    void refreshAfterGlobalEditRestore(const juce::String& statusText);
    void addModRouteForSelectedControl();
    void focusSelectedControlModDestination();
    juce::String formattedParameterValue(const juce::String& parameterID, double plainValue) const;
    juce::String modulationSummaryForParameter(const juce::String& parameterID) const;
    int modulationDestinationIndexForParameter(const juce::String& parameterID) const;
    void applyLfoCurvePreset(int presetId);
    void applyLfoCurveTool(LfoCurveTool tool);
    void applySelectedLfoCurveAction();
    void updateOutputMeter();
    void updateOutputSpectrumDisplay();
    void updateOutputOscilloscopeDisplay();
    void updateStereoFieldDisplay();
    void updateClubMonitorDisplay();
    void updateLowEndAssistant();
    void updatePerformanceSnapshotButtons();
    void updatePerformanceXYPad();
    void updateHomeOverviewDisplay();
    void updateHomeSoundStage();
    void updateHomeSignalFlowDisplay();
    void updateHomeSessionDisplay();
    void updateSequencerSceneButtons();
    void updateSequencerGridContext();
    void repaintSequencerGrids();
    void updateSequencerStepEditor();
    void setSelectedSequencerStep(Sequencer::Step step);
    void adjustSelectedSequencerStepNote(int semitones);
    void adjustSelectedSequencerStepValue(int fieldIndex, float delta);
    void toggleSelectedSequencerStepFlag(int flagIndex);
    void selectSampleSlice(size_t sliceIndex);
    void updateSampleSliceButtons();
    void updateSampleSliceEditorStatus();
    void applySampleSliceStyleDefaults(size_t sliceIndex);
    void captureCurrentSampleSliceSettings(size_t sliceIndex, bool markCustom);
    void recallSampleSliceSettings(size_t sliceIndex);
    void editSampleSliceBoundary(size_t boundaryIndex, float position);
    void storeSelectedSampleSliceSettings();
    void recallSelectedSampleSliceSettings();
    void sliceSampleToBeatGrid();
    void clearSelectedSampleSliceMarker();
    void detectSampleSliceMarkers();
    void randomizeSelectedSampleSliceSettings();
    void toggleSelectedSampleSliceReverse();
    void toggleSelectedSampleSliceChoke();
    void cycleSelectedSampleSlicePan();
    void toggleSelectedSampleSliceGhost();
    void cycleSelectedSampleSliceNudge();
    void cycleSelectedSampleSliceFade();
    void sendSampleSlicesToSequencer();
    void bakeSampleRegionToWavetable();
    bool sampleSliceHasCustomSettings(size_t sliceIndex) const;
    void updateSampleWaveformDisplay();
    void timerCallback() override;
    void refreshPresetList();
    void updatePresetCrateMapDisplay();
    void updatePresetLibrarySummary();
    void updatePresetSaveSummary();
    bool isPresetOverwriteArmed(const juce::String& presetName, const juce::String& category) const;
    void armPresetOverwrite(const juce::String& presetName, const juce::String& category);
    void clearPresetOverwriteWarning();
    void saveCurrentPreset();
    void saveActiveRandomCandidatePreset();
    void applyPresetQuickFilter(size_t index);
    void loadSelectedPreset();
    int selectedRandomMutationScope() const;
    void updateRandomMorphPad();
    void applyRandomMorphPad(float x, float y, bool createVariation);
    void triggerRandomGenerate();
    void triggerRandomMutate();
    void triggerRandomVariation();
    void triggerRandomWild();
    void triggerRandomSectionRoll(size_t sectionIndex);
    void recallRandomCandidate(size_t slotIndex);
    void auditionRandomCandidate(size_t slotIndex);
    void releaseRandomCandidateAudition(bool updateStatus = false);
    void promoteActiveRandomCandidate(int snapshotSlotIndex);
    void updateRandomCandidateButtons();
    void updateRandomCandidateDetail();
    void prepareRandomPresetDraft(const juce::String& actionLabel);
    juce::String generatedPresetNotes(const juce::String& category, const juce::String& recipe, int candidateSlotIndex);
    juce::String suggestedPresetCategoryForRecipe() const;
    juce::String suggestedPresetUseForCategory(const juce::String& category) const;
    juce::String suggestedPresetPackForCategory(const juce::String& category) const;
    int suggestedPresetBpmForCategory(const juce::String& category) const;
    void loadPresetByOffset(int offset);
    void auditionSelectedPreset();
    void warmVisiblePresetPreviews();
    void updatePresetAudition();
    void releasePresetAuditionNote();
    void returnKeyboardFocusToPiano();
    void installGlobalKeyboardListeners();
    void removeGlobalKeyboardListeners();
    void installGlobalKeyboardListenersFor(juce::Component& component);
    void removeGlobalKeyboardListenersFor(juce::Component& component);
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void releaseComputerKeyboardNotes();
    int computerKeyboardBaseNote() const noexcept;
    void syncPianoKeyboardComputerMapping();
    void pinPianoKeyboardVisualRange();
    juce::String startPresetAuditionPhrase(const NateVSTAudioProcessor::PresetInfo* presetInfo, int rootNote);
    bool hasPresetCompareSnapshots() const;
    bool capturePresetCompareBefore(const juce::String& presetName);
    void capturePresetCompareLoaded();
    void clearPresetCompareState();
    void updatePresetCompareButtons();
    void togglePresetCompare();
    void revertPresetCompare();
    void restorePresetCompareSnapshot(const juce::MemoryBlock& snapshot, const juce::String& statusText);
    void toggleFavoritePreset();
    void setSelectedPresetRating();
    void updateFavoritePresetButton();
    void shiftKeyboardOctave(int semitones);
    void updateKeyboardRangeLabel();
    void updateKeyboardPerformanceStatus();
    void prepareForManualKeyboardInput(int midiNote);
    void updateManualKeyboardAuditionState();
    void stepSequencerRoot(int semitones);
    void updateSequencerRootStepper();
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
    void duplicateModRoute(size_t slotIndex);
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
    float macroAssignmentAmountForRoute(int sourceIndex, int destinationIndex, float fallback) const;
    int selectedMacroAssignmentSourceIndex() const;
    int selectedMacroAssignmentDestinationIndex() const;

    Panel activePanel = Panel::home;
    FocusOverlay activeFocusOverlay = FocusOverlay::none;
    RandomLabPage activeRandomLabPage = RandomLabPage::generate;
    ModWorkflowPage activeModWorkflowPage = ModWorkflowPage::matrix;
    bool presetNameIsRandomDraft = false;
    bool presetNotesIsRandomDraft = false;
    bool currentPresetDraftIsGenerated = false;
    juce::String currentGeneratedPresetRecipe;
    FxModule selectedFxModule = FxModule::guard;
    FxModule fxPresetBoxModule = FxModule::guard;
    float displayedPeakLeft = 0.0f;
    float displayedPeakRight = 0.0f;
    float displayedRmsLeft = 0.0f;
    float displayedRmsRight = 0.0f;
    std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize> outputSpectrumSnapshot {};
    UI::OutputSpectrumDisplay::BandArray displayedSpectrumBands {};
    UI::OutputOscilloscopeDisplay::SampleArray displayedScopeSamples {};
    float displayedScopeTransient = 0.0f;
    float displayedStereoCorrelation = 0.0f;
    float displayedStereoWidth = 0.0f;
    float displayedStereoBalance = 0.0f;
    float displayedLowStereoRisk = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NateVSTAudioProcessorEditor)
};
