#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr auto editorWidth = 940;
constexpr auto editorHeight = 710;
constexpr auto pianoKeyboardHeight = 58;
constexpr auto keyboardControlsWidth = 214;
constexpr auto keyboardLowestNote = 24;
constexpr auto keyboardHighestNote = 96;
constexpr auto keyboardInitialLowestNote = 36;
constexpr auto keyboardMaxLowestVisibleNote = 84;
constexpr auto modTopRowHeight = 148;
constexpr auto modGeneratorRowHeight = 150;
constexpr auto modPanelGap = 6;
constexpr auto firstMacroModSourceIndex = 4;
constexpr auto presetAuditionDurationMs = 720.0;
constexpr auto presetAuditionVelocity = 0.86f;
constexpr auto fxRackStatusOverrideMs = 2200.0;
constexpr std::array<const char*, 31> momentaryFxParameterIDs {
    Parameters::ID::fxDelayEnabled,
    Parameters::ID::fxDelaySync,
    Parameters::ID::fxDelayRate,
    Parameters::ID::fxDelayTime,
    Parameters::ID::fxDelayFeedback,
    Parameters::ID::fxDelayMix,
    Parameters::ID::fxReverbEnabled,
    Parameters::ID::fxReverbSize,
    Parameters::ID::fxReverbDamping,
    Parameters::ID::fxReverbMix,
    Parameters::ID::fxPumpEnabled,
    Parameters::ID::fxPumpRate,
    Parameters::ID::fxPumpCurve,
    Parameters::ID::fxPumpCustomCurve[0],
    Parameters::ID::fxPumpCustomCurve[1],
    Parameters::ID::fxPumpCustomCurve[2],
    Parameters::ID::fxPumpCustomCurve[3],
    Parameters::ID::fxPumpCustomCurve[4],
    Parameters::ID::fxPumpCustomCurve[5],
    Parameters::ID::fxPumpCustomCurve[6],
    Parameters::ID::fxPumpCustomCurve[7],
    Parameters::ID::fxPumpDepth,
    Parameters::ID::fxPumpShape,
    Parameters::ID::fxPumpPhase,
    Parameters::ID::fxWidthEnabled,
    Parameters::ID::fxWidthAmount,
    Parameters::ID::fxWidthMonoCutoff,
    Parameters::ID::fxGuardEnabled,
    Parameters::ID::fxGuardPush,
    Parameters::ID::fxGuardCeiling,
    Parameters::ID::outputGain
};
constexpr auto lastMacroModSourceIndex = 11;

juce::Colour backgroundColour()
{
    return juce::Colour(0xff0d1113);
}

juce::Colour panelColour()
{
    return juce::Colour(0xff141a1d);
}

juce::StringArray presetCategoryChoices()
{
    return { "User", "Bass", "Stab", "Lead", "House", "Tech House", "Techno", "Minimal", "UKG", "UKG/Bass", "UKG/Chops", "House/Chords", "Tech House/Bass", "FX", "Sequence", "Sample" };
}

juce::StringArray presetFilterChoices()
{
    return {
        "All",
        "Favorites",
        "Recent",
        "Rated",
        "5 Stars",
        "4+ Stars",
        "Macro Rich",
        "User",
        "Factory",
        "Bass",
        "Stab",
        "Lead",
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG",
        "FX",
        "Sequence",
        "Sample",
        "Project Pack",
        "Factory Pack",
        "UKG Essentials",
        "UKG Basslines",
        "Garage Chops",
        "House Tools",
        "Tech House Tools",
        "Minimal Tools",
        "Techno Tools",
        "120-124 BPM",
        "125-128 BPM",
        "129-132 BPM",
        "133+ BPM"
    };
}

juce::StringArray presetSortChoices()
{
    return { "Name", "Rating", "Newest", "Category", "Pack", "BPM", "Key", "Author", "Source", "Macros" };
}

juce::StringArray presetPackChoices()
{
    return {
        "User Pack",
        "Project Pack",
        "UKG Essentials",
        "UKG Basslines",
        "Garage Chops",
        "House Tools",
        "Tech House Tools",
        "Minimal Tools",
        "Techno Tools",
        "Factory Pack"
    };
}

juce::StringArray presetKeyChoices()
{
    return {
        "Any Key",
        "C Min",
        "C# Min",
        "D Min",
        "D# Min",
        "E Min",
        "F Min",
        "F# Min",
        "G Min",
        "G# Min",
        "A Min",
        "A# Min",
        "B Min",
        "C Maj",
        "C# Maj",
        "D Maj",
        "D# Maj",
        "E Maj",
        "F Maj",
        "F# Maj",
        "G Maj",
        "G# Maj",
        "A Maj",
        "A# Maj",
        "B Maj"
    };
}

juce::StringArray presetBpmChoices()
{
    return { "Any Tempo", "120 BPM", "122 BPM", "124 BPM", "125 BPM", "126 BPM", "128 BPM", "130 BPM", "132 BPM", "134 BPM", "136 BPM", "138 BPM" };
}

int parsePresetBpm(const juce::String& text)
{
    const auto bpm = text.retainCharacters("0123456789").getIntValue();
    return bpm >= 20 && bpm <= 300 ? bpm : 0;
}

juce::String formatPresetBpm(int bpm)
{
    return bpm >= 20 && bpm <= 300 ? juce::String(bpm) + " BPM" : juce::String("Any Tempo");
}

juce::String presetMacroPreviewText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto summary = preset.macroSummary.trim();
    return summary.isNotEmpty() ? "Macros: " + summary : juce::String("Macros: flat");
}

juce::StringArray presetTagChoices()
{
    return {
        "All Tags",
        "Bass",
        "Chord",
        "Pluck",
        "Stab",
        "Sequenced",
        "Mono Safe",
        "Pump",
        "Wide",
        "FX",
        "Vocal Chop",
        "Sample",
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG"
    };
}

float smoothMeterValue(float current, float target)
{
    target = juce::jlimit(0.0f, 2.0f, target);
    const auto coefficient = target > current ? 0.65f : 0.18f;
    return current + ((target - current) * coefficient);
}

bool parameterIsOneOf(const juce::String& parameterID, std::initializer_list<const char*> ids)
{
    for (const auto* id : ids)
        if (parameterID == id)
            return true;

    return false;
}

bool destinationUsesGlobalModulationSources(int destinationIndex)
{
    return destinationIndex >= 7 && destinationIndex <= 16;
}

int rotaryDragSensitivityForParameter(const juce::String& parameterID)
{
    if (parameterIsOneOf(parameterID, {
            Parameters::ID::oscTune,
            Parameters::ID::osc2Tune,
            Parameters::ID::filterCutoff,
            Parameters::ID::fxRingFrequency,
            Parameters::ID::fxCombFrequency,
            Parameters::ID::fxDelayTime,
            Parameters::ID::fxWidthMonoCutoff,
            Parameters::ID::fxToneLowCut
        }))
        return 86;

    if (parameterIsOneOf(parameterID, {
            Parameters::ID::filterResonance,
            Parameters::ID::filterEnvAmount,
            Parameters::ID::driveAmount,
            Parameters::ID::outputGain,
            Parameters::ID::sampleTranspose,
            Parameters::ID::samplePitchRamp,
            Parameters::ID::sequencerRoot,
            Parameters::ID::sequencerSwing,
            Parameters::ID::fxEqLowGain,
            Parameters::ID::fxEqMidGain,
            Parameters::ID::fxEqHighGain,
            Parameters::ID::fxEqTrim,
            Parameters::ID::fxGuardCeiling
        }))
        return 64;

    if (parameterIsOneOf(parameterID, {
            Parameters::ID::macroTone,
            Parameters::ID::macroDirt,
            Parameters::ID::macroMotion,
            Parameters::ID::macroSpace,
            Parameters::ID::macroWeight,
            Parameters::ID::macroBounce,
            Parameters::ID::macroWarp,
            Parameters::ID::macroThrow,
            Parameters::ID::randomAmount,
            Parameters::ID::randomChaos,
            Parameters::ID::randomBrightnessBias,
            Parameters::ID::randomDriveBias,
            Parameters::ID::randomMotionBias
        }))
        return 58;

    return 52;
}

juce::ModifierKeys::Flags fineDragModifierFlags()
{
    return static_cast<juce::ModifierKeys::Flags>(juce::ModifierKeys::shiftModifier
                                                  | juce::ModifierKeys::commandModifier);
}

void applyFineDragMode(juce::Slider& slider, double sensitivity)
{
    slider.setVelocityBasedMode(false);
    slider.setVelocityModeParameters(sensitivity, 1, 0.0, true, fineDragModifierFlags());
}

juce::String controlFeelTooltip(const juce::String& labelText)
{
    return labelText + ": drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.";
}

juce::String modSourceSummaryText(size_t index)
{
    static const std::array<const char*, 14> sourceTexts {
        "LFO 1: synced shape source",
        "Mod Env: assignable ADSR",
        "Velocity: note force",
        "Tone: cutoff + resonance",
        "Dirt: drive + output trim",
        "Motion: filter env + osc2 tune",
        "Space: delay + reverb sends",
        "Weight: sub + low-end support",
        "Bounce: pump depth + groove",
        "Warp: osc bend + harmonic edge",
        "Throw: delay + reverb push",
        "S&H: stepped random movement",
        "Smooth: slewed random drift",
        "Chaos: wandering random walk"
    };

    if (index < sourceTexts.size())
        return sourceTexts[index];

    return {};
}

juce::StringArray lfoCurvePresetChoices()
{
    return {
        "Manual",
        "Garage Push",
        "Tight Duck",
        "Offbeat Skank",
        "Riser",
        "Fall",
        "Gate Steps",
        "Wobble",
        "Flat"
    };
}

std::array<float, 8> lfoCurvePresetValues(int presetId)
{
    switch (presetId)
    {
        case 2: return { -0.10f, 0.38f, 0.98f, 0.54f, 0.08f, -0.30f, -0.78f, -0.22f };
        case 3: return { -1.00f, -0.65f, -0.22f, 0.34f, 0.76f, 1.00f, 0.42f, -0.12f };
        case 4: return { -0.36f, 0.14f, 0.82f, 0.26f, -0.22f, 0.58f, 1.00f, -0.48f };
        case 5: return { -1.00f, -0.72f, -0.45f, -0.12f, 0.18f, 0.48f, 0.76f, 1.00f };
        case 6: return { 1.00f, 0.72f, 0.45f, 0.12f, -0.18f, -0.48f, -0.76f, -1.00f };
        case 7: return { 1.00f, 1.00f, -0.65f, -0.65f, 0.80f, 0.80f, -1.00f, -1.00f };
        case 8: return { 0.00f, 1.00f, 0.00f, -1.00f, 0.00f, 0.72f, 0.00f, -0.72f };
        case 9: return { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f };
        default: return { 0.00f, 0.58f, 1.00f, 0.42f, -0.18f, -0.72f, -1.00f, -0.36f };
    }
}

double lfoCyclesPerBeatForUi(int rateIndex)
{
    switch (rateIndex)
    {
        case 1: return 2.0;
        case 2: return 3.0;
        case 3: return 4.0;
        default: return 1.0;
    }
}
}

NateVSTAudioProcessorEditor::NateVSTAudioProcessorEditor(NateVSTAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      audioProcessor(processorToUse),
      pianoKeyboard(processorToUse.getMidiKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);
    setSize(editorWidth, editorHeight);

    titleLabel.setText("Nate VST", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf7f4));
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(lowEndAssistant);

    pianoKeyboard.setAvailableRange(keyboardLowestNote, keyboardHighestNote);
    pianoKeyboard.setLowestVisibleKey(keyboardInitialLowestNote);
    pianoKeyboard.setKeyWidth(18.0f);
    pianoKeyboard.setScrollButtonsVisible(true);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffd9e3df));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff151b1f));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff2a363c));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x338ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xaa8ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff253037));
    addAndMakeVisible(pianoKeyboard);

    keyboardOctaveDownButton.setTooltip("Shift the audition keyboard down one octave");
    keyboardOctaveDownButton.onClick = [this] { shiftKeyboardOctave(-12); };
    addAndMakeVisible(keyboardOctaveDownButton);

    keyboardOctaveUpButton.setTooltip("Shift the audition keyboard up one octave");
    keyboardOctaveUpButton.onClick = [this] { shiftKeyboardOctave(12); };
    addAndMakeVisible(keyboardOctaveUpButton);

    keyboardPanicButton.setTooltip("Release all audition notes");
    keyboardPanicButton.onClick = [this]
    {
        releasePresetAuditionNote();
        audioProcessor.getMidiKeyboardState().allNotesOff(0);
    };
    addAndMakeVisible(keyboardPanicButton);

    keyboardRangeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    keyboardRangeLabel.setJustificationType(juce::Justification::centred);
    keyboardRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(keyboardRangeLabel);
    updateKeyboardRangeLabel();

    configureSectionLabel(homeSectionLabel, "HOME");
    configureSectionLabel(homeEngineLabel, "PERFORM");
    configureSectionLabel(homeShapeLabel, "MACROS");
    configureSectionLabel(homeLabLabel, "RANDOM LAB");
    configureSectionLabel(homeLibraryLabel, "LIBRARY");
    configureSectionLabel(synthSectionLabel, "SYNTH");
    configureSectionLabel(synthSourceLabel, "SOURCE MIX");
    configureSectionLabel(synthVoiceLabel, "PITCH + VOICE");
    configureSectionLabel(synthFilterLabel, "FILTER DRIVE");
    configureSectionLabel(synthAmpLabel, "AMP + OUTPUT");
    configureSectionLabel(randomSectionLabel, "LAB");
    configureSectionLabel(modSectionLabel, "MOD");
    configureSectionLabel(modSourceLabel, "SOURCES");
    configureSectionLabel(modMacroLabel, "MACROS");
    configureSectionLabel(modLfoLabel, "LFO 1");
    configureSectionLabel(modEnvelopeLabel, "MOD ENV 1");
    configureSectionLabel(modMatrixLabel, "ROUTING");
    configureSectionLabel(sampleSectionLabel, "SAMPLE");
    configureSectionLabel(sampleSourceLabel, "SOURCE");
    configureSectionLabel(sampleChopLabel, "CHOP");
    configureSectionLabel(sampleShapeLabel, "SHAPE");
    configureSectionLabel(sequencerSectionLabel, "SEQ");
    configureSectionLabel(futureSectionLabel, "FX");
    configureSectionLabel(librarySectionLabel, "LIBRARY");

    hostSyncStatusLabel.setText("INT 124 | FREE", juce::dontSendNotification);
    hostSyncStatusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hostSyncStatusLabel.setJustificationType(juce::Justification::centred);
    hostSyncStatusLabel.setMinimumHorizontalScale(0.72f);
    hostSyncStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7d8b90));
    hostSyncStatusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x22141a1d));
    hostSyncStatusLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff263035));
    hostSyncStatusLabel.setTooltip("Host tempo and transport phase status for sequencer and tempo-synced FX");
    addAndMakeVisible(hostSyncStatusLabel);

    sampleNameLabel.setText("No sample", juce::dontSendNotification);
    sampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    sampleNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(sampleNameLabel);

    presetStatusLabel.setJustificationType(juce::Justification::centredLeft);
    presetStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(presetStatusLabel);

    presetBrowserHeaderLabel.setText("PRESET        CATEGORY      PACK        KEY     BPM   RATE  MACROS", juce::dontSendNotification);
    presetBrowserHeaderLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    presetBrowserHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    presetBrowserHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
    presetBrowserHeaderLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserHeaderLabel.setTooltip("Visible preset browser columns");
    addAndMakeVisible(presetBrowserHeaderLabel);

    presetBrowserList.setModel(this);
    presetBrowserList.setRowHeight(28);
    presetBrowserList.setMultipleSelectionEnabled(false);
    presetBrowserList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff344047));
    presetBrowserList.setColour(juce::ListBox::textColourId, juce::Colour(0xffdce7e4));
    presetBrowserList.setOutlineThickness(1);
    presetBrowserList.setTooltip("Click a preset row to select it. Double-click a row to load it.");
    addAndMakeVisible(presetBrowserList);

    randomStatusLabel.setText("Ready", juce::dontSendNotification);
    randomStatusLabel.setJustificationType(juce::Justification::centredLeft);
    randomStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(randomStatusLabel);

    modMatrixStatusLabel.setJustificationType(juce::Justification::centredRight);
    modMatrixStatusLabel.setFont(juce::FontOptions(11.0f));
    modMatrixStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modMatrixStatusLabel);

    auto configureMatrixHeader = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff617078));
        addAndMakeVisible(label);
    };
    configureMatrixHeader(modMatrixSourceHeader, "SOURCE");
    configureMatrixHeader(modMatrixDestinationHeader, "DESTINATION");
    configureMatrixHeader(modMatrixAmountHeader, "AMOUNT");
    configureMatrixHeader(modMatrixSourceHeaderB, "SOURCE");
    configureMatrixHeader(modMatrixDestinationHeaderB, "DESTINATION");
    configureMatrixHeader(modMatrixAmountHeaderB, "AMOUNT");
    configureMatrixHeader(modInspectorLabel, "INSPECT");
    configureMatrixHeader(modMacroAssignLabel, "ASSIGN");

    modInspectorStatusLabel.setJustificationType(juce::Justification::centredLeft);
    modInspectorStatusLabel.setFont(juce::FontOptions(11.0f));
    modInspectorStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modInspectorStatusLabel);

    modMacroAssignStatusLabel.setJustificationType(juce::Justification::centredLeft);
    modMacroAssignStatusLabel.setFont(juce::FontOptions(10.5f));
    modMacroAssignStatusLabel.setMinimumHorizontalScale(0.64f);
    modMacroAssignStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modMacroAssignStatusLabel);

    performanceStatusLabel.setJustificationType(juce::Justification::centredLeft);
    performanceStatusLabel.setFont(juce::FontOptions(11.0f));
    performanceStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(performanceStatusLabel);

    performanceXYPad.onChange = [this] (float motion, float space)
    {
        setPlainParameterValue(Parameters::ID::macroMotion, motion);
        setPlainParameterValue(Parameters::ID::macroSpace, space);
    };
    addAndMakeVisible(performanceXYPad);

    sampleWaveformDisplay.onRangeChange = [this] (float start, float end)
    {
        setPlainParameterValue(Parameters::ID::sampleStart, start);
        setPlainParameterValue(Parameters::ID::sampleEnd, end);
        setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
        updateSampleSliceButtons();
    };
    addAndMakeVisible(sampleWaveformDisplay);

    fxRackStatusLabel.setJustificationType(juce::Justification::centredLeft);
    fxRackStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(fxRackStatusLabel);

    for (size_t index = 0; index < modSourceRows.size(); ++index)
    {
        auto& label = modSourceRows[index];
        label.setText(modSourceSummaryText(index), juce::dontSendNotification);
        label.setFont(juce::FontOptions(10.5f));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xffc7d7d4));
        label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(label);
    }

    for (size_t index = 0; index < modSlotRows.size(); ++index)
    {
        modSlotRows[index].setText(juce::String(static_cast<int>(index + 1)), juce::dontSendNotification);
        modSlotRows[index].setFont(juce::FontOptions(12.0f, juce::Font::bold));
        modSlotRows[index].setJustificationType(juce::Justification::centred);
        modSlotRows[index].setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
        addAndMakeVisible(modSlotRows[index]);
    }

    for (auto& row : modMatrixRows)
    {
        addAndMakeVisible(row);
        row.toBack();
    }

    presetNameEditor.setTextToShowWhenEmpty("Preset name", juce::Colour(0xff617078));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(presetNameEditor);

    presetSearchEditor.setTextToShowWhenEmpty("Search presets", juce::Colour(0xff617078));
    presetSearchEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetSearchEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetSearchEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetSearchEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(presetSearchEditor);

    presetAuthorEditor.setTextToShowWhenEmpty("Author", juce::Colour(0xff617078));
    presetAuthorEditor.setText("Nate", juce::dontSendNotification);
    presetAuthorEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetAuthorEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetAuthorEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetAuthorEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(presetAuthorEditor);

    waveformBox.addItemList(Parameters::waveformChoices(), 1);
    addAndMakeVisible(waveformBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::oscWave, waveformBox));

    osc2WaveBox.addItemList(Parameters::waveformChoices(), 1);
    addAndMakeVisible(osc2WaveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::osc2Wave, osc2WaveBox));

    filterModeBox.addItemList(Parameters::filterModeChoices(), 1);
    addAndMakeVisible(filterModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterMode, filterModeBox));

    filterCharacterBox.addItemList(Parameters::filterCharacterChoices(), 1);
    filterCharacterBox.setTextWhenNothingSelected("Character");
    filterCharacterBox.setTooltip("Choose the filter drive character");
    addAndMakeVisible(filterCharacterBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterCharacter, filterCharacterBox));

    filterSlopeBox.addItemList(Parameters::filterSlopeChoices(), 1);
    filterSlopeBox.setTextWhenNothingSelected("Slope");
    filterSlopeBox.setTooltip("Choose 12 dB or 24 dB filter slope");
    addAndMakeVisible(filterSlopeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterSlope, filterSlopeBox));

    recipeBox.addItemList(Parameters::randomRecipeChoices(), 1);
    addAndMakeVisible(recipeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomRecipe, recipeBox));

    randomScopeBox.addItemList({ "All", "Source", "Env", "Filter", "Sample", "FX", "Seq", "Macros" }, 1);
    randomScopeBox.setSelectedId(1, juce::dontSendNotification);
    randomScopeBox.setTextWhenNothingSelected("Scope");
    randomScopeBox.setTooltip("Limit Generate, Vary, Mutate, and Wild to one patch section");
    addAndMakeVisible(randomScopeBox);

    sequencerRateBox.addItemList(Parameters::sequencerRateChoices(), 1);
    addAndMakeVisible(sequencerRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerRate, sequencerRateBox));

    sequencerGrooveBox.addItemList(Parameters::sequencerGrooveModeChoices(), 1);
    sequencerGrooveBox.setTextWhenNothingSelected("Groove");
    addAndMakeVisible(sequencerGrooveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerGrooveMode, sequencerGrooveBox));

    sequencerScaleBox.addItemList(Parameters::sequencerScaleChoices(), 1);
    sequencerScaleBox.setTextWhenNothingSelected("Scale");
    addAndMakeVisible(sequencerScaleBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerScale, sequencerScaleBox));

    sequencerChordBox.addItemList(Parameters::sequencerChordModeChoices(), 1);
    sequencerChordBox.setTextWhenNothingSelected("Chord");
    addAndMakeVisible(sequencerChordBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordMode, sequencerChordBox));

    sequencerVoicingBox.addItemList(Parameters::sequencerChordVoicingChoices(), 1);
    sequencerVoicingBox.setTextWhenNothingSelected("Voice");
    addAndMakeVisible(sequencerVoicingBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordVoicing, sequencerVoicingBox));

    sequencerPatternBox.addItem("Bass", 1);
    sequencerPatternBox.addItem("Stab", 2);
    sequencerPatternBox.addItem("UKG 2-Step", 3);
    sequencerPatternBox.addItem("Shuffle Bass", 4);
    sequencerPatternBox.addItem("Organ Skank", 5);
    sequencerPatternBox.addItem("Vocal Chop", 6);
    sequencerPatternBox.addItem("Late Stab", 7);
    sequencerPatternBox.addItem("House Chord", 8);
    sequencerPatternBox.addItem("Tech Bass", 9);
    sequencerPatternBox.addItem("Minimal Pluck", 10);
    sequencerPatternBox.addItem("Techno Pulse", 11);
    sequencerPatternBox.setSelectedId(3, juce::dontSendNotification);
    addAndMakeVisible(sequencerPatternBox);

    sequencerGrooveTransformBox.addItem("Tighten", 1);
    sequencerGrooveTransformBox.addItem("Straight Anchors", 2);
    sequencerGrooveTransformBox.addItem("Swung Ghosts", 3);
    sequencerGrooveTransformBox.addItem("Late Stabs", 4);
    sequencerGrooveTransformBox.addItem("Vocal Push", 5);
    sequencerGrooveTransformBox.addItem("Humanize", 6);
    sequencerGrooveTransformBox.setSelectedId(1, juce::dontSendNotification);
    sequencerGrooveTransformBox.setTooltip("Choose a timing transform for the current sequence");
    addAndMakeVisible(sequencerGrooveTransformBox);

    sampleModeBox.addItem("Gate", 1);
    sampleModeBox.addItem("One Shot", 2);
    sampleModeBox.setTextWhenNothingSelected("Mode");
    addAndMakeVisible(sampleModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::samplePlaybackMode, sampleModeBox));

    sampleSliceStyleBox.addItemList(Parameters::sampleSliceStyleChoices(), 1);
    sampleSliceStyleBox.setTextWhenNothingSelected("Slice Style");
    sampleSliceStyleBox.setTooltip("Choose how the numbered slice pads set pitch, reverse, and stutter behavior");
    addAndMakeVisible(sampleSliceStyleBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleSliceStyle, sampleSliceStyleBox));

    sampleStutterRateBox.addItem("1/8", 1);
    sampleStutterRateBox.addItem("1/16", 2);
    sampleStutterRateBox.addItem("1/32", 3);
    sampleStutterRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(sampleStutterRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleStutterRate, sampleStutterRateBox));

    addAndMakeVisible(presetBox);

    presetCategoryBox.addItemList(presetCategoryChoices(), 1);
    presetCategoryBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetCategoryBox.setEditableText(true);
    presetCategoryBox.setTextWhenNothingSelected("Category / Folder");
    presetCategoryBox.setTooltip("Choose or type a save category. Use slashes for subfolders, like UKG/Bass.");
    addAndMakeVisible(presetCategoryBox);

    presetFilterBox.addItemList(presetFilterChoices(), 1);
    presetFilterBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetFilterBox);

    presetTagBox.addItemList(presetTagChoices(), 1);
    presetTagBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetTagBox);

    presetSortBox.addItemList(presetSortChoices(), 1);
    presetSortBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetSortBox.setTextWhenNothingSelected("Sort");
    presetSortBox.setTooltip("Sort the visible library by name, rating, newest save, category, or source");
    addAndMakeVisible(presetSortBox);

    presetRatingBox.addItem("No Stars", 1);
    presetRatingBox.addItem("1 Star", 2);
    presetRatingBox.addItem("2 Stars", 3);
    presetRatingBox.addItem("3 Stars", 4);
    presetRatingBox.addItem("4 Stars", 5);
    presetRatingBox.addItem("5 Stars", 6);
    presetRatingBox.setSelectedId(1, juce::dontSendNotification);
    presetRatingBox.setTextWhenNothingSelected("Rating");
    presetRatingBox.setTooltip("Rate the selected preset from 1 to 5 stars");
    addAndMakeVisible(presetRatingBox);

    presetPackBox.addItemList(presetPackChoices(), 1);
    presetPackBox.setSelectedId(1, juce::dontSendNotification);
    presetPackBox.setEditableText(true);
    presetPackBox.setTextWhenNothingSelected("Pack");
    presetPackBox.setTooltip("Choose or type a preset pack name");
    addAndMakeVisible(presetPackBox);

    presetKeyBox.addItemList(presetKeyChoices(), 1);
    presetKeyBox.setSelectedId(1, juce::dontSendNotification);
    presetKeyBox.setEditableText(true);
    presetKeyBox.setTextWhenNothingSelected("Key");
    presetKeyBox.setTooltip("Store the musical key for browser search and sorting");
    addAndMakeVisible(presetKeyBox);

    presetBpmBox.addItemList(presetBpmChoices(), 1);
    presetBpmBox.setSelectedId(7, juce::dontSendNotification);
    presetBpmBox.setEditableText(true);
    presetBpmBox.setTextWhenNothingSelected("BPM");
    presetBpmBox.setTooltip("Store the target tempo for browser search, filtering, and sorting");
    addAndMakeVisible(presetBpmBox);

    fxAddBox.addSectionHeading("Tone & Drive");
    fxAddBox.addItem("Tone", 1);
    fxAddBox.addItem("EQ", 2);
    fxAddBox.addItem("Drive", 3);
    fxAddBox.addItem("Crush", 4);
    fxAddBox.addSectionHeading("Movement");
    fxAddBox.addItem("Pump", 5);
    fxAddBox.addItem("Tremolo", 6);
    fxAddBox.addItem("Ring Mod", 7);
    fxAddBox.addItem("Comb", 8);
    fxAddBox.addItem("Phaser", 9);
    fxAddBox.addItem("Flanger", 10);
    fxAddBox.addItem("Chorus", 11);
    fxAddBox.addSectionHeading("Space & Utility");
    fxAddBox.addItem("Delay", 12);
    fxAddBox.addItem("Reverb", 13);
    fxAddBox.addItem("Width", 14);
    fxAddBox.addItem("Guard", 15);
    fxAddBox.setTextWhenNothingSelected("Add FX");
    fxAddBox.setTooltip("Enable a fixed FX module and open its focused editor");
    addAndMakeVisible(fxAddBox);

    fxPresetBox.setTextWhenNothingSelected("Module Preset");
    fxPresetBox.setTooltip("Load a focused preset for the selected FX module");
    addAndMakeVisible(fxPresetBox);

    fxDelayRateBox.addItemList(Parameters::delayRateChoices(), 1);
    fxDelayRateBox.setTextWhenNothingSelected("Rate");
    fxDelayRateBox.setTooltip("Choose the tempo-synced delay division");
    addAndMakeVisible(fxDelayRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelayRate, fxDelayRateBox));

    fxPumpRateBox.addItem("1/4", 1);
    fxPumpRateBox.addItem("1/8", 2);
    fxPumpRateBox.addItem("1/8T", 3);
    fxPumpRateBox.addItem("1/16", 4);
    fxPumpRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxPumpRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpRate, fxPumpRateBox));

    fxPumpCurveBox.addItemList(Parameters::pumpCurveChoices(), 1);
    fxPumpCurveBox.setTextWhenNothingSelected("Curve");
    fxPumpCurveBox.setTooltip("Choose the pump/duck envelope shape");
    addAndMakeVisible(fxPumpCurveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpCurve, fxPumpCurveBox));

    fxTremoloRateBox.addItem("1/4", 1);
    fxTremoloRateBox.addItem("1/8", 2);
    fxTremoloRateBox.addItem("1/8T", 3);
    fxTremoloRateBox.addItem("1/16", 4);
    fxTremoloRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxTremoloRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxTremoloRate, fxTremoloRateBox));

    const auto modDestinationChoices = Parameters::modulationDestinationChoices();
    for (auto index = 1; index < modDestinationChoices.size(); ++index)
        modInspectorDestinationBox.addItem(modDestinationChoices[index], index + 1);
    modInspectorDestinationBox.setSelectedId(2, juce::dontSendNotification);
    modInspectorDestinationBox.setTextWhenNothingSelected("Destination");
    modInspectorDestinationBox.setTooltip("Inspect active modulation routes for one destination");
    addAndMakeVisible(modInspectorDestinationBox);

    const auto modSourceChoices = Parameters::modulationSourceChoices();
    for (auto index = 1; index < modSourceChoices.size(); ++index)
        modInspectorSourceBox.addItem(modSourceChoices[index], index + 1);
    modInspectorSourceBox.setSelectedId(2, juce::dontSendNotification);
    modInspectorSourceBox.setTextWhenNothingSelected("Add Source");
    modInspectorSourceBox.setTooltip("Choose a modulation source to add to the inspected destination");
    addAndMakeVisible(modInspectorSourceBox);

    for (auto index = firstMacroModSourceIndex; index <= lastMacroModSourceIndex && index < modSourceChoices.size(); ++index)
        modMacroAssignSourceBox.addItem(modSourceChoices[index], index + 1);
    modMacroAssignSourceBox.setSelectedId(firstMacroModSourceIndex + 1, juce::dontSendNotification);
    modMacroAssignSourceBox.setTextWhenNothingSelected("Macro");
    modMacroAssignSourceBox.setTooltip("Choose the performance macro to edit");
    addAndMakeVisible(modMacroAssignSourceBox);

    for (auto index = 1; index < modDestinationChoices.size(); ++index)
        modMacroAssignDestinationBox.addItem(modDestinationChoices[index], index + 1);
    modMacroAssignDestinationBox.setSelectedId(2, juce::dontSendNotification);
    modMacroAssignDestinationBox.setTextWhenNothingSelected("Destination");
    modMacroAssignDestinationBox.setTooltip("Choose the destination controlled by the selected macro");
    addAndMakeVisible(modMacroAssignDestinationBox);

    lfo1ShapeBox.addItemList(Parameters::lfoShapeChoices(), 1);
    lfo1ShapeBox.setTextWhenNothingSelected("Shape");
    addAndMakeVisible(lfo1ShapeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Shape, lfo1ShapeBox));

    lfo1SyncRateBox.addItemList(Parameters::lfoSyncRateChoices(), 1);
    lfo1SyncRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(lfo1SyncRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1SyncRate, lfo1SyncRateBox));

    lfoCurvePresetBox.addItemList(lfoCurvePresetChoices(), 1);
    lfoCurvePresetBox.setSelectedId(1, juce::dontSendNotification);
    lfoCurvePresetBox.setTextWhenNothingSelected("Curve Preset");
    lfoCurvePresetBox.setTooltip("Load an LFO curve shape for house, UKG, techno, and minimal movement");
    addAndMakeVisible(lfoCurvePresetBox);

    for (size_t index = 0; index < modSourceBoxes.size(); ++index)
    {
        auto& sourceBox = modSourceBoxes[index];
        sourceBox.addItemList(Parameters::modulationSourceChoices(), 1);
        sourceBox.setTextWhenNothingSelected("Source");
        addAndMakeVisible(sourceBox);
        comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixSource[index], sourceBox));

        auto& destinationBox = modDestinationBoxes[index];
        destinationBox.addItemList(Parameters::modulationDestinationChoices(), 1);
        destinationBox.setTextWhenNothingSelected("Destination");
        addAndMakeVisible(destinationBox);
        comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixDestination[index], destinationBox));

        auto& enabledButton = modSlotEnabledButtons[index];
        enabledButton.setButtonText("On");
        enabledButton.setTooltip("Bypass or enable modulation slot " + juce::String(static_cast<int>(index + 1)));
        addAndMakeVisible(enabledButton);
        buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixEnabled[index], enabledButton));

        auto& deleteButton = modSlotDeleteButtons[index];
        deleteButton.setButtonText("X");
        deleteButton.setTooltip("Delete modulation slot " + juce::String(static_cast<int>(index + 1)));
        deleteButton.onClick = [this, index] { deleteModRoute(index); };
        addAndMakeVisible(deleteButton);
    }

    monoButton.setButtonText("Mono");
    addAndMakeVisible(monoButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::monoMode, monoButton));

    sampleEnabledButton.setButtonText("On");
    addAndMakeVisible(sampleEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleEnabled, sampleEnabledButton));

    sampleReverseButton.setButtonText("Rev");
    addAndMakeVisible(sampleReverseButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleReverse, sampleReverseButton));

    sampleStutterEnabledButton.setButtonText("Stutter");
    addAndMakeVisible(sampleStutterEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleStutterEnabled, sampleStutterEnabledButton));

    sequencerEnabledButton.setButtonText("On");
    addAndMakeVisible(sequencerEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerEnabled, sequencerEnabledButton));

    sequencerChordMemoryButton.setButtonText("Memory");
    sequencerChordMemoryButton.setTooltip("Expand live notes through the selected chord mode and voicing");
    addAndMakeVisible(sequencerChordMemoryButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordMemory, sequencerChordMemoryButton));

    fxDistortionEnabledButton.setButtonText("Dist");
    addAndMakeVisible(fxDistortionEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDistortionEnabled, fxDistortionEnabledButton));

    fxBitcrushEnabledButton.setButtonText("Crush");
    addAndMakeVisible(fxBitcrushEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxBitcrushEnabled, fxBitcrushEnabledButton));

    fxPumpEnabledButton.setButtonText("Pump");
    addAndMakeVisible(fxPumpEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpEnabled, fxPumpEnabledButton));

    fxTremoloEnabledButton.setButtonText("Trem");
    addAndMakeVisible(fxTremoloEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxTremoloEnabled, fxTremoloEnabledButton));

    fxRingEnabledButton.setButtonText("Ring");
    addAndMakeVisible(fxRingEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxRingEnabled, fxRingEnabledButton));

    fxCombEnabledButton.setButtonText("Comb");
    addAndMakeVisible(fxCombEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxCombEnabled, fxCombEnabledButton));

    fxChorusEnabledButton.setButtonText("Chorus");
    addAndMakeVisible(fxChorusEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxChorusEnabled, fxChorusEnabledButton));

    fxDelayEnabledButton.setButtonText("Delay");
    addAndMakeVisible(fxDelayEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelayEnabled, fxDelayEnabledButton));

    fxDelaySyncButton.setButtonText("Sync");
    fxDelaySyncButton.setTooltip("Lock delay time to the host tempo division");
    addAndMakeVisible(fxDelaySyncButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelaySync, fxDelaySyncButton));

    fxReverbEnabledButton.setButtonText("Reverb");
    addAndMakeVisible(fxReverbEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxReverbEnabled, fxReverbEnabledButton));

    fxWidthEnabledButton.setButtonText("Width");
    addAndMakeVisible(fxWidthEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxWidthEnabled, fxWidthEnabledButton));

    fxToneEnabledButton.setButtonText("Tone");
    addAndMakeVisible(fxToneEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxToneEnabled, fxToneEnabledButton));

    fxEqEnabledButton.setButtonText("EQ");
    addAndMakeVisible(fxEqEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxEqEnabled, fxEqEnabledButton));

    fxPhaserEnabledButton.setButtonText("Phaser");
    addAndMakeVisible(fxPhaserEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPhaserEnabled, fxPhaserEnabledButton));

    fxFlangerEnabledButton.setButtonText("Flanger");
    addAndMakeVisible(fxFlangerEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxFlangerEnabled, fxFlangerEnabledButton));

    fxGuardEnabledButton.setButtonText("Guard");
    addAndMakeVisible(fxGuardEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxGuardEnabled, fxGuardEnabledButton));

    randomLockPitchButton.setButtonText("Pitch");
    addAndMakeVisible(randomLockPitchButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockPitch, randomLockPitchButton));

    randomLockEnvelopeButton.setButtonText("Env");
    addAndMakeVisible(randomLockEnvelopeButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockEnvelope, randomLockEnvelopeButton));

    randomLockFilterButton.setButtonText("Filter");
    addAndMakeVisible(randomLockFilterButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockFilter, randomLockFilterButton));

    randomLockSourceButton.setButtonText("Source");
    addAndMakeVisible(randomLockSourceButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSource, randomLockSourceButton));

    randomLockSampleButton.setButtonText("Sample");
    addAndMakeVisible(randomLockSampleButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSample, randomLockSampleButton));

    randomLockFxButton.setButtonText("FX");
    addAndMakeVisible(randomLockFxButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockFx, randomLockFxButton));

    randomLockOutputButton.setButtonText("Output");
    addAndMakeVisible(randomLockOutputButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockOutput, randomLockOutputButton));

    randomLockSequencerButton.setButtonText("Seq");
    addAndMakeVisible(randomLockSequencerButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSequencer, randomLockSequencerButton));

    lfo1SyncButton.setButtonText("Sync");
    addAndMakeVisible(lfo1SyncButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Sync, lfo1SyncButton));

    lfo1RetriggerButton.setButtonText("Retrig");
    addAndMakeVisible(lfo1RetriggerButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Retrigger, lfo1RetriggerButton));

    configureSlider(octaveSlider, octaveLabel, "Oct", Parameters::ID::oscOctave);
    configureSlider(tuneSlider, tuneLabel, "Tune", Parameters::ID::oscTune);
    configureSlider(osc1LevelSlider, osc1LevelLabel, "Osc 1", Parameters::ID::osc1Level);
    configureSlider(osc2OctaveSlider, osc2OctaveLabel, "O2 Oct", Parameters::ID::osc2Octave);
    configureSlider(osc2TuneSlider, osc2TuneLabel, "O2 Tune", Parameters::ID::osc2Tune);
    configureSlider(osc2LevelSlider, osc2LevelLabel, "Osc 2", Parameters::ID::osc2Level);
    configureSlider(subLevelSlider, subLevelLabel, "Sub", Parameters::ID::subLevel);
    configureSlider(noiseLevelSlider, noiseLevelLabel, "Noise", Parameters::ID::noiseLevel);
    configureSlider(oscWarpSlider, oscWarpLabel, "Osc Warp", Parameters::ID::oscWarp);
    configureSlider(unisonVoicesSlider, unisonVoicesLabel, "Voices", Parameters::ID::unisonVoices);
    configureSlider(unisonDetuneSlider, unisonDetuneLabel, "Detune", Parameters::ID::unisonDetune);
    configureSlider(unisonBlendSlider, unisonBlendLabel, "Blend", Parameters::ID::unisonBlend);
    configureSlider(unisonSpreadSlider, unisonSpreadLabel, "Spread", Parameters::ID::unisonSpread);
    configureSlider(glideSlider, glideLabel, "Glide", Parameters::ID::glideTime);
    configureSlider(macroToneSlider, macroToneLabel, "Tone", Parameters::ID::macroTone);
    configureSlider(macroDirtSlider, macroDirtLabel, "Dirt", Parameters::ID::macroDirt);
    configureSlider(macroMotionSlider, macroMotionLabel, "Motion", Parameters::ID::macroMotion);
    configureSlider(macroSpaceSlider, macroSpaceLabel, "Space", Parameters::ID::macroSpace);
    configureSlider(macroWeightSlider, macroWeightLabel, "Weight", Parameters::ID::macroWeight);
    configureSlider(macroBounceSlider, macroBounceLabel, "Bounce", Parameters::ID::macroBounce);
    configureSlider(macroWarpSlider, macroWarpLabel, "Warp", Parameters::ID::macroWarp);
    configureSlider(macroThrowSlider, macroThrowLabel, "Throw", Parameters::ID::macroThrow);
    configureSlider(lfo1RateSlider, lfo1RateLabel, "Rate", Parameters::ID::lfo1Rate);
    configureSlider(lfo1DepthSlider, lfo1DepthLabel, "Depth", Parameters::ID::lfo1Depth);
    configureSlider(lfo1PhaseSlider, lfo1PhaseLabel, "Phase", Parameters::ID::lfo1Phase);
    lfoCurveDisplay.onPointChange = [this] (size_t index, float value)
    {
        if (index < Parameters::ID::lfo1Curve.size())
        {
            lfoCurvePresetBox.setSelectedId(1, juce::dontSendNotification);
            setPlainParameterValue(Parameters::ID::lfo1Curve[index], value);
        }
    };
    addAndMakeVisible(lfoCurveDisplay);
    pumpCurveDisplay.onPointChange = [this] (size_t index, float value)
    {
        if (index < Parameters::ID::fxPumpCustomCurve.size())
        {
            setPlainParameterValue(Parameters::ID::fxPumpCurve, 5.0f);
            setPlainParameterValue(Parameters::ID::fxPumpCustomCurve[index], value);
        }
    };
    addAndMakeVisible(pumpCurveDisplay);

    for (size_t index = 0; index < lfoCurveSliders.size(); ++index)
        configureCompactHorizontalSlider(lfoCurveSliders[index], Parameters::ID::lfo1Curve[index]);

    configureSlider(modEnv1AttackSlider, modEnv1AttackLabel, "Attack", Parameters::ID::modEnv1Attack);
    configureSlider(modEnv1DecaySlider, modEnv1DecayLabel, "Decay", Parameters::ID::modEnv1Decay);
    configureSlider(modEnv1SustainSlider, modEnv1SustainLabel, "Sustain", Parameters::ID::modEnv1Sustain);
    configureSlider(modEnv1ReleaseSlider, modEnv1ReleaseLabel, "Release", Parameters::ID::modEnv1Release);
    configureSlider(modEnv1DepthSlider, modEnv1DepthLabel, "Depth", Parameters::ID::modEnv1Depth);

    for (size_t index = 0; index < modAmountSliders.size(); ++index)
        configureHorizontalSlider(modAmountSliders[index],
                                  modAmountLabels[index],
                                  "Amt " + juce::String(static_cast<int>(index + 1)),
                                  Parameters::ID::modMatrixAmount[index]);

    modMacroAssignAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    modMacroAssignAmountSlider.setRange(-100.0, 100.0, 1.0);
    modMacroAssignAmountSlider.setValue(30.0, juce::dontSendNotification);
    modMacroAssignAmountSlider.setDoubleClickReturnValue(true, 30.0);
    modMacroAssignAmountSlider.setMouseDragSensitivity(135);
    applyFineDragMode(modMacroAssignAmountSlider, 0.36);
    modMacroAssignAmountSlider.setSliderSnapsToMousePosition(false);
    modMacroAssignAmountSlider.setScrollWheelEnabled(false);
    modMacroAssignAmountSlider.setPopupDisplayEnabled(true, true, this);
    modMacroAssignAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 18);
    modMacroAssignAmountSlider.setTextValueSuffix("%");
    modMacroAssignAmountSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    modMacroAssignAmountSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    modMacroAssignAmountSlider.setTooltip(controlFeelTooltip("Macro amount"));
    addAndMakeVisible(modMacroAssignAmountSlider);

    configureSlider(attackSlider, attackLabel, "Attack", Parameters::ID::ampAttack);
    configureSlider(decaySlider, decayLabel, "Decay", Parameters::ID::ampDecay);
    configureSlider(sustainSlider, sustainLabel, "Sustain", Parameters::ID::ampSustain);
    configureSlider(releaseSlider, releaseLabel, "Release", Parameters::ID::ampRelease);
    configureSlider(cutoffSlider, cutoffLabel, "Cutoff", Parameters::ID::filterCutoff);
    configureSlider(resonanceSlider, resonanceLabel, "Res", Parameters::ID::filterResonance);
    configureSlider(filterEnvSlider, filterEnvLabel, "F Env", Parameters::ID::filterEnvAmount);
    configureSlider(driveSlider, driveLabel, "Drive", Parameters::ID::driveAmount);
    configureSlider(outputSlider, outputLabel, "Output", Parameters::ID::outputGain);
    cutoffSlider.onDragStart = [this] { setModInspectorDestination(1); };
    resonanceSlider.onDragStart = [this] { setModInspectorDestination(2); };
    filterEnvSlider.onDragStart = [this] { setModInspectorDestination(3); };
    driveSlider.onDragStart = [this] { setModInspectorDestination(4); };
    osc2TuneSlider.onDragStart = [this] { setModInspectorDestination(5); };
    osc2LevelSlider.onDragStart = [this] { setModInspectorDestination(6); };
    oscWarpSlider.onDragStart = [this] { setModInspectorDestination(17); };
    sampleStartSlider.onDragStart = [this] { setModInspectorDestination(12); };
    sampleMixSlider.onDragStart = [this] { setModInspectorDestination(13); };
    sampleTransposeSlider.onDragStart = [this] { setModInspectorDestination(14); };
    samplePitchRampSlider.onDragStart = [this] { setModInspectorDestination(15); };
    sampleStutterRepeatsSlider.onDragStart = [this] { setModInspectorDestination(16); };
    configureSlider(randomAmountSlider, randomAmountLabel, "Amount", Parameters::ID::randomAmount);
    configureSlider(randomChaosSlider, randomChaosLabel, "Chaos", Parameters::ID::randomChaos);
    configureSlider(brightnessSlider, brightnessLabel, "Bright", Parameters::ID::randomBrightnessBias);
    configureSlider(driveBiasSlider, driveBiasLabel, "Drive Bias", Parameters::ID::randomDriveBias);
    configureSlider(motionBiasSlider, motionBiasLabel, "Motion", Parameters::ID::randomMotionBias);
    configureHorizontalSlider(sampleStartSlider, sampleStartLabel, "Start", Parameters::ID::sampleStart);
    configureHorizontalSlider(sampleEndSlider, sampleEndLabel, "End", Parameters::ID::sampleEnd);
    configureSlider(sampleTransposeSlider, sampleTransposeLabel, "Pitch", Parameters::ID::sampleTranspose);
    configureSlider(samplePitchRampSlider, samplePitchRampLabel, "Ramp", Parameters::ID::samplePitchRamp);
    configureSlider(sampleGainSlider, sampleGainLabel, "Gain", Parameters::ID::sampleGain);
    configureSlider(sampleMixSlider, sampleMixLabel, "Mix", Parameters::ID::sampleMix);
    configureSlider(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, "Repeat", Parameters::ID::sampleStutterRepeats);
    configureSlider(sequencerRootSlider, sequencerRootLabel, "Root", Parameters::ID::sequencerRoot);
    configureSlider(sequencerGateSlider, sequencerGateLabel, "Gate", Parameters::ID::sequencerGate);
    configureSlider(sequencerSwingSlider, sequencerSwingLabel, "Swing", Parameters::ID::sequencerSwing);
    configureSlider(sequencerChordStrumSlider, sequencerChordStrumLabel, "Strum", Parameters::ID::sequencerChordStrum);
    configureSlider(sequencerAccentSlider, sequencerAccentLabel, "Accent", Parameters::ID::sequencerAccent);
    configureSlider(sequencerOctaveSlider, sequencerOctaveLabel, "Oct", Parameters::ID::sequencerOctave);
    configureSlider(sequencerProbabilitySlider, sequencerProbabilityLabel, "Prob", Parameters::ID::sequencerProbability);
    configureSlider(sequencerRandomSlider, sequencerRandomLabel, "Rand", Parameters::ID::sequencerRandomAmount);
    configureSlider(fxDistortionAmountSlider, fxDistortionAmountLabel, "Drive", Parameters::ID::fxDistortionAmount);
    configureSlider(fxBitcrushBitsSlider, fxBitcrushBitsLabel, "Bits", Parameters::ID::fxBitcrushBits);
    configureSlider(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, "Down", Parameters::ID::fxBitcrushDownsample);
    configureSlider(fxBitcrushMixSlider, fxBitcrushMixLabel, "Mix", Parameters::ID::fxBitcrushMix);
    configureSlider(fxPumpDepthSlider, fxPumpDepthLabel, "Depth", Parameters::ID::fxPumpDepth);
    configureSlider(fxPumpShapeSlider, fxPumpShapeLabel, "Shape", Parameters::ID::fxPumpShape);
    configureSlider(fxPumpPhaseSlider, fxPumpPhaseLabel, "Phase", Parameters::ID::fxPumpPhase);
    configureSlider(fxTremoloDepthSlider, fxTremoloDepthLabel, "Depth", Parameters::ID::fxTremoloDepth);
    configureSlider(fxTremoloPanSlider, fxTremoloPanLabel, "Pan", Parameters::ID::fxTremoloPan);
    configureSlider(fxTremoloShapeSlider, fxTremoloShapeLabel, "Shape", Parameters::ID::fxTremoloShape);
    configureSlider(fxTremoloPhaseSlider, fxTremoloPhaseLabel, "Phase", Parameters::ID::fxTremoloPhase);
    configureSlider(fxRingFrequencySlider, fxRingFrequencyLabel, "Freq", Parameters::ID::fxRingFrequency);
    configureSlider(fxRingDepthSlider, fxRingDepthLabel, "Depth", Parameters::ID::fxRingDepth);
    configureSlider(fxRingMixSlider, fxRingMixLabel, "Mix", Parameters::ID::fxRingMix);
    configureSlider(fxRingBiasSlider, fxRingBiasLabel, "Bias", Parameters::ID::fxRingBias);
    configureSlider(fxCombFrequencySlider, fxCombFrequencyLabel, "Freq", Parameters::ID::fxCombFrequency);
    configureSlider(fxCombFeedbackSlider, fxCombFeedbackLabel, "Fdbk", Parameters::ID::fxCombFeedback);
    configureSlider(fxCombDampingSlider, fxCombDampingLabel, "Damp", Parameters::ID::fxCombDamping);
    configureSlider(fxCombMixSlider, fxCombMixLabel, "Mix", Parameters::ID::fxCombMix);
    configureSlider(fxChorusRateSlider, fxChorusRateLabel, "Rate", Parameters::ID::fxChorusRate);
    configureSlider(fxChorusDepthSlider, fxChorusDepthLabel, "Depth", Parameters::ID::fxChorusDepth);
    configureSlider(fxChorusMixSlider, fxChorusMixLabel, "Mix", Parameters::ID::fxChorusMix);
    configureSlider(fxDelayTimeSlider, fxDelayTimeLabel, "Time", Parameters::ID::fxDelayTime);
    configureSlider(fxDelayFeedbackSlider, fxDelayFeedbackLabel, "Fdbk", Parameters::ID::fxDelayFeedback);
    configureSlider(fxDelayMixSlider, fxDelayMixLabel, "Mix", Parameters::ID::fxDelayMix);
    configureSlider(fxReverbSizeSlider, fxReverbSizeLabel, "Size", Parameters::ID::fxReverbSize);
    configureSlider(fxReverbDampingSlider, fxReverbDampingLabel, "Damp", Parameters::ID::fxReverbDamping);
    configureSlider(fxReverbMixSlider, fxReverbMixLabel, "Mix", Parameters::ID::fxReverbMix);
    configureSlider(fxWidthAmountSlider, fxWidthAmountLabel, "Width", Parameters::ID::fxWidthAmount);
    configureSlider(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, "Mono Hz", Parameters::ID::fxWidthMonoCutoff);
    configureSlider(fxToneTiltSlider, fxToneTiltLabel, "Tilt", Parameters::ID::fxToneTilt);
    configureSlider(fxToneLowCutSlider, fxToneLowCutLabel, "Low Cut", Parameters::ID::fxToneLowCut);
    configureSlider(fxEqLowGainSlider, fxEqLowGainLabel, "Low", Parameters::ID::fxEqLowGain);
    configureSlider(fxEqMidGainSlider, fxEqMidGainLabel, "Mid", Parameters::ID::fxEqMidGain);
    configureSlider(fxEqHighGainSlider, fxEqHighGainLabel, "High", Parameters::ID::fxEqHighGain);
    configureSlider(fxEqTrimSlider, fxEqTrimLabel, "Trim", Parameters::ID::fxEqTrim);
    configureSlider(fxPhaserRateSlider, fxPhaserRateLabel, "Rate", Parameters::ID::fxPhaserRate);
    configureSlider(fxPhaserDepthSlider, fxPhaserDepthLabel, "Depth", Parameters::ID::fxPhaserDepth);
    configureSlider(fxPhaserMixSlider, fxPhaserMixLabel, "Mix", Parameters::ID::fxPhaserMix);
    configureSlider(fxFlangerRateSlider, fxFlangerRateLabel, "Rate", Parameters::ID::fxFlangerRate);
    configureSlider(fxFlangerDepthSlider, fxFlangerDepthLabel, "Depth", Parameters::ID::fxFlangerDepth);
    configureSlider(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, "Fdbk", Parameters::ID::fxFlangerFeedback);
    configureSlider(fxFlangerMixSlider, fxFlangerMixLabel, "Mix", Parameters::ID::fxFlangerMix);
    configureSlider(fxGuardPushSlider, fxGuardPushLabel, "Push", Parameters::ID::fxGuardPush);
    configureSlider(fxGuardCeilingSlider, fxGuardCeilingLabel, "Ceil", Parameters::ID::fxGuardCeiling);

    generateButton.onClick = [this]
    {
        audioProcessor.generateRandomPatch(selectedRandomMutationScope());
        setRandomStatus("Generated");
    };
    mutateButton.onClick = [this]
    {
        audioProcessor.mutateRandomPatch(selectedRandomMutationScope());
        setRandomStatus("Mutated");
    };
    variationButton.onClick = [this]
    {
        audioProcessor.createRandomVariation(selectedRandomMutationScope());
        setRandomStatus("Variation");
    };
    wildMutateButton.setTooltip("Make a stronger recipe-aware mutation while respecting active locks");
    wildMutateButton.onClick = [this]
    {
        audioProcessor.wildMutateRandomPatch(selectedRandomMutationScope());
        setRandomStatus("Wild");
    };
    undoRandomButton.onClick = [this]
    {
        setRandomStatus(audioProcessor.undoRandomization() ? "Undo restored" : "Nothing to undo");
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        sequencerGrid.repaint();
    };
    redoRandomButton.setTooltip("Redo the last undone randomization action");
    redoRandomButton.onClick = [this]
    {
        setRandomStatus(audioProcessor.redoRandomization() ? "Redo restored" : "Nothing to redo");
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        sequencerGrid.repaint();
    };
    recallSnapshotAButton.setTooltip("Recall performance snapshot A");
    recallSnapshotAButton.onClick = [this]
    {
        if (audioProcessor.recallPerformanceSnapshot(0))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            sequencerGrid.repaint();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotAButton.setTooltip("Store the current patch in performance snapshot A");
    captureSnapshotAButton.onClick = [this]
    {
        audioProcessor.capturePerformanceSnapshot(0);
        updatePerformanceSnapshotButtons();
    };
    recallSnapshotBButton.setTooltip("Recall performance snapshot B");
    recallSnapshotBButton.onClick = [this]
    {
        if (audioProcessor.recallPerformanceSnapshot(1))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            sequencerGrid.repaint();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotBButton.setTooltip("Store the current patch in performance snapshot B");
    captureSnapshotBButton.onClick = [this]
    {
        audioProcessor.capturePerformanceSnapshot(1);
        updatePerformanceSnapshotButtons();
    };
    loadSampleButton.onClick = [this] { chooseSampleFile(); };
    clearSampleButton.onClick = [this]
    {
        audioProcessor.clearSample();
        sampleWaveformKey = "cleared";
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
    };
    randomCutButton.onClick = [this]
    {
        setRandomStatus(audioProcessor.randomizeSampleCut() ? "Sample randomized" : "Sample skipped");
        updateSampleWaveformDisplay();
    };
    ukgChopButton.onClick = [this]
    {
        if (audioProcessor.randomizeUkgVocalChop())
        {
            sequencerGrid.repaint();
            updateSampleWaveformDisplay();
            setRandomStatus("UKG chop ready");
        }
        else
        {
            setRandomStatus("UKG chop skipped");
        }
    };
    for (size_t index = 0; index < sampleSliceButtons.size(); ++index)
    {
        auto& button = sampleSliceButtons[index];
        button.setButtonText(juce::String(static_cast<int>(index + 1)));
        button.setTooltip("Select and audition sample slice " + juce::String(static_cast<int>(index + 1)));
        button.onClick = [this, index] { selectSampleSlice(index); };
        addAndMakeVisible(button);
    }
    randomSequencerButton.onClick = [this]
    {
        if (audioProcessor.randomizeSequencerPattern())
        {
            sequencerGrid.repaint();
            setRandomStatus("Sequence generated");
        }
        else
        {
            setRandomStatus("Sequence skipped");
        }
    };
    mutateSequencerButton.setTooltip("Create a small variation of the current sequencer pattern");
    mutateSequencerButton.onClick = [this]
    {
        if (audioProcessor.mutateSequencerPattern())
        {
            sequencerGrid.repaint();
            setRandomStatus("Sequence varied");
        }
        else
        {
            setRandomStatus("Sequence skipped");
        }
    };
    undoSequencerButton.setTooltip("Undo the last sequencer utility edit");
    undoSequencerButton.onClick = [this]
    {
        if (audioProcessor.undoSequencerEdit())
        {
            sequencerGrid.repaint();
            updateSegmentedSelectors();
            setRandomStatus("Sequence undo");
        }
        else
        {
            setRandomStatus("No sequence undo");
        }
    };
    clearSequencerButton.onClick = [this]
    {
        audioProcessor.clearSequencerPattern();
        sequencerGrid.repaint();
    };
    bassPatternButton.onClick = [this]
    {
        audioProcessor.applySequencerPatternPreset(0);
        sequencerGrid.repaint();
    };
    stabPatternButton.onClick = [this]
    {
        audioProcessor.applySequencerPatternPreset(1);
        sequencerGrid.repaint();
    };
    ukgPatternButton.onClick = [this]
    {
        audioProcessor.applySequencerPatternPreset(2);
        sequencerGrid.repaint();
    };
    applyPatternButton.onClick = [this]
    {
        const auto selectedId = sequencerPatternBox.getSelectedId();
        audioProcessor.applySequencerPatternPreset(juce::jmax(1, selectedId) - 1);
        sequencerGrid.repaint();
    };
    copySequencerButton.onClick = [this]
    {
        audioProcessor.copySequencerFirstHalfToSecondHalf();
        sequencerGrid.repaint();
    };
    rotateSequencerLeftButton.setTooltip("Shift the whole sequencer pattern one step earlier");
    rotateSequencerLeftButton.onClick = [this]
    {
        audioProcessor.rotateSequencerPattern(-1);
        sequencerGrid.repaint();
        setRandomStatus("Sequence rotated left");
    };
    rotateSequencerRightButton.setTooltip("Shift the whole sequencer pattern one step later");
    rotateSequencerRightButton.onClick = [this]
    {
        audioProcessor.rotateSequencerPattern(1);
        sequencerGrid.repaint();
        setRandomStatus("Sequence rotated right");
    };
    exportSequencerMidiButton.setTooltip("Export the current sequencer pattern as a MIDI clip");
    exportSequencerMidiButton.onClick = [this] { exportSequencerMidiClip(); };
    applyGrooveTransformButton.setTooltip("Apply the selected groove transform to the current sequence");
    applyGrooveTransformButton.onClick = [this]
    {
        const auto selectedId = sequencerGrooveTransformBox.getSelectedId();
        const auto transformIndex = juce::jmax(1, selectedId) - 1;
        if (audioProcessor.applySequencerGrooveTransform(transformIndex))
        {
            sequencerGrid.repaint();
            updateSegmentedSelectors();
            setRandomStatus(sequencerGrooveTransformBox.getText() + " shaped");
        }
        else
        {
            setRandomStatus("Groove skipped");
        }
    };
    homeTabButton.onClick = [this] { setActivePanel(Panel::home); };
    synthTabButton.onClick = [this] { setActivePanel(Panel::synth); };
    labTabButton.onClick = [this] { setActivePanel(Panel::lab); };
    modTabButton.onClick = [this] { setActivePanel(Panel::mod); };
    sampleTabButton.onClick = [this] { setActivePanel(Panel::sample); };
    sequencerTabButton.onClick = [this] { setActivePanel(Panel::sequencer); };
    effectsTabButton.onClick = [this] { setActivePanel(Panel::effects); };
    libraryTabButton.onClick = [this] { setActivePanel(Panel::library); };
    sineWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 0); };
    sawWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 1); };
    squareWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 2); };
    triangleWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 3); };
    osc2SineWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 0); };
    osc2SawWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 1); };
    osc2SquareWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 2); };
    osc2TriangleWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 3); };
    lowpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 0); };
    bandpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 1); };
    highpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 2); };
    rateEighthButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 0); };
    rateSixteenthButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 1); };
    rateThirtySecondButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 2); };
    previousPresetButton.onClick = [this] { loadPresetByOffset(-1); };
    nextPresetButton.onClick = [this] { loadPresetByOffset(1); };
    savePresetButton.onClick = [this] { saveCurrentPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    auditionPresetButton.onClick = [this] { auditionSelectedPreset(); };
    refreshPresetsButton.onClick = [this] { refreshPresetList(); };
    favoritePresetButton.onClick = [this] { toggleFavoritePreset(); };
    presetFilterBox.onChange = [this] { refreshPresetList(); };
    presetTagBox.onChange = [this] { refreshPresetList(); };
    presetSortBox.onChange = [this] { refreshPresetList(); };
    presetRatingBox.onChange = [this] { setSelectedPresetRating(); };
    presetSearchEditor.onTextChange = [this] { refreshPresetList(); };
    presetBox.onChange = [this]
    {
        updateFavoritePresetButton();

        const auto selectedName = presetBox.getText();
        for (auto index = 0; index < getNumRows(); ++index)
        {
            if (visiblePresetBrowserPresets[static_cast<size_t>(index)].name != selectedName)
                continue;

            ignorePresetBrowserSelection = true;
            presetBrowserList.selectRow(index);
            ignorePresetBrowserSelection = false;
            break;
        }
    };
    fxAddBox.onChange = [this]
    {
        const auto selectedId = fxAddBox.getSelectedId();
        if (selectedId > 0)
            addFxModule(static_cast<FxModule>(selectedId - 1));
    };
    fxPresetBox.onChange = [this] { applySelectedFxPreset(); };
    modInspectorDestinationBox.onChange = [this] { updateModInspectorStatus(); };
    modMacroAssignSourceBox.onChange = [this] { updateMacroAssignmentEditorStatus(); };
    modMacroAssignDestinationBox.onChange = [this] { updateMacroAssignmentEditorStatus(); };
    modMacroAssignAmountSlider.onValueChange = [this] { updateMacroAssignmentEditorStatus(); };
    lfoCurvePresetBox.onChange = [this]
    {
        const auto selectedId = lfoCurvePresetBox.getSelectedId();
        if (selectedId > 1)
            applyLfoCurvePreset(selectedId);
    };
    fxRemoveButton.onClick = [this] { removeSelectedFxModule(); };
    fxMoveUpButton.onClick = [this] { moveSelectedFxModule(-1); };
    fxMoveDownButton.onClick = [this] { moveSelectedFxModule(1); };
    fxResetOrderButton.onClick = [this] { resetFxModuleOrder(); };
    fxThrowDelayButton.onClick = [this] { applyDelayThrow(); };
    fxThrowSpaceButton.onClick = [this] { applySpaceThrow(); };
    fxThrowPumpButton.onClick = [this] { applyPumpDrop(); };
    fxThrowDryButton.onClick = [this] { clearFxThrows(); };
    fxHoldDelayButton.setTooltip("Hold for a temporary delay throw, release to restore");
    fxHoldDelayButton.onStateChange = [this]
    {
        if (fxHoldDelayButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::delay);
        else
            endMomentaryFxAction(MomentaryFxAction::delay);
    };
    fxHoldSpaceButton.setTooltip("Hold for a temporary delay and reverb wash, release to restore");
    fxHoldSpaceButton.onStateChange = [this]
    {
        if (fxHoldSpaceButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::space);
        else
            endMomentaryFxAction(MomentaryFxAction::space);
    };
    fxHoldPumpButton.setTooltip("Hold for a temporary pump/duck move, release to restore");
    fxHoldPumpButton.onStateChange = [this]
    {
        if (fxHoldPumpButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::pump);
        else
            endMomentaryFxAction(MomentaryFxAction::pump);
    };
    fxMuteDropButton.setTooltip("Hold for a temporary mute drop, release to restore");
    fxMuteDropButton.onStateChange = [this]
    {
        if (fxMuteDropButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::mute);
        else
            endMomentaryFxAction(MomentaryFxAction::mute);
    };
    fxApplyPresetButton.setTooltip("Reload the selected FX module preset");
    fxApplyPresetButton.onClick = [this] { applySelectedFxPreset(); };
    modInspectorAddButton.setTooltip("Add the selected source to the inspected destination");
    modInspectorAddButton.onClick = [this] { addInspectedModRoute(); };
    modInspectorClearButton.setTooltip("Delete all active routes targeting the inspected destination");
    modInspectorClearButton.onClick = [this] { clearInspectedModRoutes(); };
    modMacroAssignAddButton.setTooltip("Add or update this macro assignment");
    modMacroAssignAddButton.onClick = [this] { addMacroAssignment(false); };
    modMacroAssignReplaceButton.setTooltip("Replace all assignments for the selected macro with this one destination");
    modMacroAssignReplaceButton.onClick = [this] { addMacroAssignment(true); };
    modMacroAssignClearButton.setTooltip("Delete all routes owned by the selected macro");
    modMacroAssignClearButton.onClick = [this] { clearSelectedMacroAssignments(); };
    fxToneSlotButton.onClick = [this] { selectFxModule(FxModule::tone); };
    fxEqSlotButton.onClick = [this] { selectFxModule(FxModule::eq); };
    fxDistortionSlotButton.onClick = [this] { selectFxModule(FxModule::distortion); };
    fxBitcrushSlotButton.onClick = [this] { selectFxModule(FxModule::bitcrush); };
    fxPumpSlotButton.onClick = [this] { selectFxModule(FxModule::pump); };
    fxTremoloSlotButton.onClick = [this] { selectFxModule(FxModule::tremolo); };
    fxRingSlotButton.onClick = [this] { selectFxModule(FxModule::ring); };
    fxCombSlotButton.onClick = [this] { selectFxModule(FxModule::comb); };
    fxPhaserSlotButton.onClick = [this] { selectFxModule(FxModule::phaser); };
    fxFlangerSlotButton.onClick = [this] { selectFxModule(FxModule::flanger); };
    fxChorusSlotButton.onClick = [this] { selectFxModule(FxModule::chorus); };
    fxDelaySlotButton.onClick = [this] { selectFxModule(FxModule::delay); };
    fxReverbSlotButton.onClick = [this] { selectFxModule(FxModule::reverb); };
    fxWidthSlotButton.onClick = [this] { selectFxModule(FxModule::width); };
    fxGuardSlotButton.onClick = [this] { selectFxModule(FxModule::guard); };

    auto updateLockStatus = [this] { setRandomStatus("Locks updated"); };
    randomLockPitchButton.onClick = updateLockStatus;
    randomLockEnvelopeButton.onClick = updateLockStatus;
    randomLockFilterButton.onClick = updateLockStatus;
    randomLockSourceButton.onClick = updateLockStatus;
    randomLockSampleButton.onClick = updateLockStatus;
    randomLockFxButton.onClick = updateLockStatus;
    randomLockOutputButton.onClick = updateLockStatus;
    randomLockSequencerButton.onClick = updateLockStatus;

    addAndMakeVisible(generateButton);
    addAndMakeVisible(mutateButton);
    addAndMakeVisible(variationButton);
    addAndMakeVisible(wildMutateButton);
    addAndMakeVisible(undoRandomButton);
    addAndMakeVisible(redoRandomButton);
    addAndMakeVisible(recallSnapshotAButton);
    addAndMakeVisible(captureSnapshotAButton);
    addAndMakeVisible(recallSnapshotBButton);
    addAndMakeVisible(captureSnapshotBButton);
    addAndMakeVisible(loadSampleButton);
    addAndMakeVisible(clearSampleButton);
    addAndMakeVisible(randomCutButton);
    addAndMakeVisible(ukgChopButton);
    addAndMakeVisible(randomSequencerButton);
    addAndMakeVisible(mutateSequencerButton);
    addAndMakeVisible(undoSequencerButton);
    addAndMakeVisible(clearSequencerButton);
    addAndMakeVisible(bassPatternButton);
    addAndMakeVisible(stabPatternButton);
    addAndMakeVisible(ukgPatternButton);
    addAndMakeVisible(applyPatternButton);
    addAndMakeVisible(copySequencerButton);
    addAndMakeVisible(rotateSequencerLeftButton);
    addAndMakeVisible(rotateSequencerRightButton);
    addAndMakeVisible(exportSequencerMidiButton);
    addAndMakeVisible(applyGrooveTransformButton);
    addAndMakeVisible(homeTabButton);
    addAndMakeVisible(synthTabButton);
    addAndMakeVisible(labTabButton);
    addAndMakeVisible(modTabButton);
    addAndMakeVisible(sampleTabButton);
    addAndMakeVisible(sequencerTabButton);
    addAndMakeVisible(effectsTabButton);
    addAndMakeVisible(libraryTabButton);
    addAndMakeVisible(sineWaveButton);
    addAndMakeVisible(sawWaveButton);
    addAndMakeVisible(squareWaveButton);
    addAndMakeVisible(triangleWaveButton);
    addAndMakeVisible(osc2SineWaveButton);
    addAndMakeVisible(osc2SawWaveButton);
    addAndMakeVisible(osc2SquareWaveButton);
    addAndMakeVisible(osc2TriangleWaveButton);
    addAndMakeVisible(lowpassFilterButton);
    addAndMakeVisible(bandpassFilterButton);
    addAndMakeVisible(highpassFilterButton);
    addAndMakeVisible(rateEighthButton);
    addAndMakeVisible(rateSixteenthButton);
    addAndMakeVisible(rateThirtySecondButton);
    addAndMakeVisible(previousPresetButton);
    addAndMakeVisible(nextPresetButton);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(auditionPresetButton);
    addAndMakeVisible(refreshPresetsButton);
    addAndMakeVisible(favoritePresetButton);
    addAndMakeVisible(fxMoveUpButton);
    addAndMakeVisible(fxMoveDownButton);
    addAndMakeVisible(fxResetOrderButton);
    addAndMakeVisible(fxRemoveButton);
    addAndMakeVisible(fxThrowDelayButton);
    addAndMakeVisible(fxThrowSpaceButton);
    addAndMakeVisible(fxThrowPumpButton);
    addAndMakeVisible(fxThrowDryButton);
    addAndMakeVisible(fxHoldDelayButton);
    addAndMakeVisible(fxHoldSpaceButton);
    addAndMakeVisible(fxHoldPumpButton);
    addAndMakeVisible(fxMuteDropButton);
    addAndMakeVisible(fxApplyPresetButton);
    addAndMakeVisible(modInspectorAddButton);
    addAndMakeVisible(modInspectorClearButton);
    addAndMakeVisible(modMacroAssignAddButton);
    addAndMakeVisible(modMacroAssignReplaceButton);
    addAndMakeVisible(modMacroAssignClearButton);
    addAndMakeVisible(fxToneSlotButton);
    addAndMakeVisible(fxEqSlotButton);
    addAndMakeVisible(fxDistortionSlotButton);
    addAndMakeVisible(fxBitcrushSlotButton);
    addAndMakeVisible(fxPumpSlotButton);
    addAndMakeVisible(fxTremoloSlotButton);
    addAndMakeVisible(fxRingSlotButton);
    addAndMakeVisible(fxCombSlotButton);
    addAndMakeVisible(fxPhaserSlotButton);
    addAndMakeVisible(fxFlangerSlotButton);
    addAndMakeVisible(fxChorusSlotButton);
    addAndMakeVisible(fxDelaySlotButton);
    addAndMakeVisible(fxReverbSlotButton);
    addAndMakeVisible(fxWidthSlotButton);
    addAndMakeVisible(fxGuardSlotButton);

    sequencerGrid.setCallbacks(
        [this] (int index) { return audioProcessor.getSequencerStep(index); },
        [this] (int index, Sequencer::Step step) { audioProcessor.setSequencerStep(index, step); });
    addAndMakeVisible(sequencerGrid);
    updateSampleNameLabel();
    refreshPresetList();
    setActivePanel(Panel::home);
    startTimerHz(12);
}

NateVSTAudioProcessorEditor::~NateVSTAudioProcessorEditor()
{
    restoreFxMomentarySnapshot(fxMomentarySnapshot);
    releasePresetAuditionNote();
    stopTimer();
    presetBrowserList.setModel(nullptr);
    setLookAndFeel(nullptr);
}

void NateVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour());

    auto bounds = getLocalBounds().reduced(16);
    const auto topArea = bounds.removeFromTop(48);
    const auto keyboardArea = bounds.removeFromBottom(pianoKeyboardHeight);
    bounds.removeFromBottom(10);
    const auto contentArea = bounds.reduced(0, 8);

    g.setColour(panelColour());
    g.fillRoundedRectangle(topArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(contentArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(keyboardArea.toFloat(), 7.0f);

    g.setColour(juce::Colour(0xff293339));
    g.drawRoundedRectangle(topArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(contentArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(keyboardArea.toFloat(), 7.0f, 1.0f);
    g.drawVerticalLine(keyboardArea.getX() + keyboardControlsWidth,
                       static_cast<float>(keyboardArea.getY() + 9),
                       static_cast<float>(keyboardArea.getBottom() - 9));

    if (activePanel == Panel::home)
    {
        auto homeContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = homeContent.removeFromTop(224);
        auto engineArea = topRow.removeFromLeft(330).reduced(5);
        auto shapeArea = topRow.reduced(5);
        auto bottomRow = homeContent.withTrimmedTop(16);
        auto labArea = bottomRow.removeFromLeft(330).reduced(5);
        auto libraryArea = bottomRow.reduced(5);

        for (auto area : { engineArea, shapeArea, labArea, libraryArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::synth)
    {
        auto synthContent = contentArea.reduced(18).withTrimmedTop(36);
        synthContent.removeFromTop(44);
        synthContent.removeFromTop(44);
        synthContent.removeFromTop(8);

        auto topCards = synthContent.removeFromTop((synthContent.getHeight() - 10) / 2);
        synthContent.removeFromTop(10);
        auto bottomCards = synthContent;
        auto sourceArea = topCards.removeFromLeft(330).reduced(5);
        auto voiceArea = topCards.reduced(5);
        auto filterArea = bottomCards.removeFromLeft(330).reduced(5);
        auto ampArea = bottomCards.reduced(5);

        for (auto area : { sourceArea, voiceArea, filterArea, ampArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::mod)
    {
        auto modContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = modContent.removeFromTop(modTopRowHeight);
        auto sourceArea = topRow.removeFromLeft(300).reduced(5);
        auto macroArea = topRow.reduced(5);
        modContent.removeFromTop(modPanelGap);
        auto controlsRow = modContent.removeFromTop(modGeneratorRowHeight);
        auto lfoArea = controlsRow.removeFromLeft(450).reduced(5);
        auto envelopeArea = controlsRow.reduced(5);
        modContent.removeFromTop(modPanelGap);
        auto matrixArea = modContent.reduced(5);

        for (auto area : { sourceArea, macroArea, lfoArea, envelopeArea, matrixArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::sample)
    {
        auto sampleContent = contentArea.reduced(18);
        sampleContent.removeFromTop(28);
        auto sourceArea = sampleContent.removeFromTop(88).reduced(5);
        auto chopArea = sampleContent.removeFromTop(192).reduced(5);
        auto shapeArea = sampleContent.reduced(5);

        for (auto area : { sourceArea, chopArea, shapeArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::sequencer)
    {
        auto sequencerContent = contentArea.reduced(18).withTrimmedTop(36);
        auto controlArea = sequencerContent.removeFromTop(202).reduced(5);
        auto gridArea = sequencerContent.reduced(5, 7);

        for (auto area : { controlArea, gridArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::effects)
    {
        auto fxContent = contentArea.reduced(18);
        fxContent.removeFromTop(28);
        auto commandArea = fxContent.removeFromTop(44).reduced(5);
        auto performArea = fxContent.removeFromTop(42).reduced(5);
        fxContent.removeFromTop(8);
        auto rackArea = fxContent.removeFromLeft(260).reduced(5);
        auto detailArea = fxContent.reduced(5);

        for (auto area : { commandArea, performArea, rackArea, detailArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }

        g.setColour(juce::Colour(0xff879299));
        g.setFont(12.0f);
        g.drawText("RACK", rackArea.removeFromTop(24).reduced(12, 0), juce::Justification::centredLeft);
        g.drawText(fxModuleName(selectedFxModule).toUpperCase(), detailArea.removeFromTop(24).reduced(14, 0), juce::Justification::centredLeft);
    }
}

void NateVSTAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    auto top = bounds.removeFromTop(42);

    outputMeter.setBounds(top.removeFromRight(156).reduced(6, 5));
    titleLabel.setBounds(top.removeFromLeft(132).reduced(8, 0));

    auto placeTab = [&top] (juce::TextButton& button, int width)
    {
        button.setBounds(top.removeFromLeft(width).reduced(3, 4));
    };

    placeTab(homeTabButton, 74);
    placeTab(synthTabButton, 74);
    placeTab(labTabButton, 60);
    placeTab(modTabButton, 60);
    placeTab(sampleTabButton, 84);
    placeTab(sequencerTabButton, 62);
    placeTab(effectsTabButton, 56);
    placeTab(libraryTabButton, 96);

    bounds.removeFromTop(14);
    auto keyboardArea = bounds.removeFromBottom(pianoKeyboardHeight);
    auto keyboardControlArea = keyboardArea.removeFromLeft(keyboardControlsWidth).reduced(8, 6);
    keyboardOctaveDownButton.setBounds(keyboardControlArea.removeFromLeft(42).reduced(2, 3));
    keyboardRangeLabel.setBounds(keyboardControlArea.removeFromLeft(52).reduced(2, 3));
    keyboardOctaveUpButton.setBounds(keyboardControlArea.removeFromLeft(42).reduced(2, 3));
    keyboardPanicButton.setBounds(keyboardControlArea.removeFromLeft(62).reduced(2, 3));
    pianoKeyboard.setBounds(keyboardArea.reduced(8, 6));
    bounds.removeFromBottom(10);
    auto content = bounds.reduced(18);
    hidePanelComponents();
    updateTabButtons();

    switch (activePanel)
    {
        case Panel::home:
        {
            homeSectionLabel.setVisible(true);
            homeEngineLabel.setVisible(true);
            homeShapeLabel.setVisible(true);
            homeLabLabel.setVisible(true);
            homeLibraryLabel.setVisible(true);
            recipeBox.setVisible(true);
            randomScopeBox.setVisible(true);
            generateButton.setVisible(true);
            mutateButton.setVisible(true);
            variationButton.setVisible(true);
            wildMutateButton.setVisible(true);
            undoRandomButton.setVisible(true);
            redoRandomButton.setVisible(true);
            presetBox.setVisible(true);
            presetCategoryBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            auditionPresetButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            presetNameEditor.setVisible(true);
            savePresetButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            randomStatusLabel.setVisible(true);
            lowEndAssistant.setVisible(true);
            performanceXYPad.setVisible(true);
            performanceStatusLabel.setVisible(true);
            recallSnapshotAButton.setVisible(true);
            captureSnapshotAButton.setVisible(true);
            recallSnapshotBButton.setVisible(true);
            captureSnapshotBButton.setVisible(true);

            homeSectionLabel.setBounds(content.removeFromTop(28));
            auto dashboard = content.withTrimmedTop(8);
            auto topRow = dashboard.removeFromTop(224);
            auto performArea = topRow.removeFromLeft(330).reduced(18, 12);
            auto macroArea = topRow.reduced(18, 12);
            auto bottomRow = dashboard.withTrimmedTop(16);
            auto labArea = bottomRow.removeFromLeft(330).reduced(18, 12);
            auto libraryArea = bottomRow.reduced(18, 12);

            homeEngineLabel.setBounds(performArea.removeFromTop(24));
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            layoutKnobRow(performArea.removeFromTop(92).withTrimmedTop(6), { &subLevelSlider, &cutoffSlider, &driveSlider, &outputSlider });
            lowEndAssistant.setBounds(performArea.removeFromTop(62).reduced(2, 4));

            homeShapeLabel.setBounds(macroArea.removeFromTop(24));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroWeightSlider, macroWeightLabel, true);
            setSliderVisible(macroBounceSlider, macroBounceLabel, true);
            setSliderVisible(macroWarpSlider, macroWarpLabel, true);
            setSliderVisible(macroThrowSlider, macroThrowLabel, true);
            auto macroControlArea = macroArea.removeFromTop(128).withTrimmedTop(4);
            performanceXYPad.setBounds(macroControlArea.removeFromRight(136).reduced(4, 0));
            layoutKnobRow(macroControlArea.removeFromTop(60), {
                &macroToneSlider,
                &macroDirtSlider,
                &macroWeightSlider
            });
            layoutKnobRow(macroControlArea.withTrimmedTop(4), {
                &macroBounceSlider,
                &macroWarpSlider,
                &macroThrowSlider
            });
            auto snapshotRow = macroArea.removeFromTop(40).withTrimmedTop(5);
            performanceStatusLabel.setBounds(snapshotRow.removeFromLeft(156).reduced(4, 4));
            recallSnapshotAButton.setBounds(snapshotRow.removeFromLeft(48).reduced(3, 4));
            captureSnapshotAButton.setBounds(snapshotRow.removeFromLeft(72).reduced(3, 4));
            recallSnapshotBButton.setBounds(snapshotRow.removeFromLeft(48).reduced(3, 4));
            captureSnapshotBButton.setBounds(snapshotRow.removeFromLeft(72).reduced(3, 4));

            homeLabLabel.setBounds(labArea.removeFromTop(24));
            auto randomSelectRow = labArea.removeFromTop(38);
            recipeBox.setBounds(randomSelectRow.removeFromLeft(176).reduced(3, 4));
            randomScopeBox.setBounds(randomSelectRow.reduced(3, 4));
            auto labButtonRow = labArea.removeFromTop(36).withTrimmedTop(4);
            generateButton.setBounds(labButtonRow.removeFromLeft(82).reduced(3, 4));
            mutateButton.setBounds(labButtonRow.removeFromLeft(74).reduced(3, 4));
            variationButton.setBounds(labButtonRow.removeFromLeft(62).reduced(3, 4));
            wildMutateButton.setBounds(labButtonRow.removeFromLeft(56).reduced(3, 4));
            auto randomStatusRow = labArea.removeFromTop(34).withTrimmedTop(6);
            undoRandomButton.setBounds(randomStatusRow.removeFromLeft(58).reduced(3, 4));
            redoRandomButton.setBounds(randomStatusRow.removeFromLeft(58).reduced(3, 4));
            randomStatusLabel.setBounds(randomStatusRow.reduced(5, 4));

            homeLibraryLabel.setBounds(libraryArea.removeFromTop(24));
            auto loadRow = libraryArea.removeFromTop(42);
            previousPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            presetBox.setBounds(loadRow.removeFromLeft(202).reduced(3, 4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            loadPresetButton.setBounds(loadRow.removeFromLeft(70).reduced(3, 4));
            auditionPresetButton.setBounds(loadRow.removeFromLeft(82).reduced(3, 4));
            favoritePresetButton.setBounds(loadRow.removeFromLeft(58).reduced(3, 4));
            auto saveRow = libraryArea.removeFromTop(42).withTrimmedTop(4);
            presetCategoryBox.setBounds(saveRow.removeFromLeft(130).reduced(3, 4));
            presetNameEditor.setBounds(saveRow.removeFromLeft(212).reduced(3, 4));
            savePresetButton.setBounds(saveRow.removeFromLeft(90).reduced(3, 4));
            presetStatusLabel.setBounds(libraryArea.removeFromTop(34).reduced(5, 4));
            break;
        }

        case Panel::synth:
        {
            synthSectionLabel.setVisible(true);
            synthSourceLabel.setVisible(true);
            synthVoiceLabel.setVisible(true);
            synthFilterLabel.setVisible(true);
            synthAmpLabel.setVisible(true);
            sineWaveButton.setVisible(true);
            sawWaveButton.setVisible(true);
            squareWaveButton.setVisible(true);
            triangleWaveButton.setVisible(true);
            osc2SineWaveButton.setVisible(true);
            osc2SawWaveButton.setVisible(true);
            osc2SquareWaveButton.setVisible(true);
            osc2TriangleWaveButton.setVisible(true);
            lowpassFilterButton.setVisible(true);
            bandpassFilterButton.setVisible(true);
            highpassFilterButton.setVisible(true);
            filterCharacterBox.setVisible(true);
            filterSlopeBox.setVisible(true);
            monoButton.setVisible(true);
            synthSectionLabel.setBounds(content.removeFromTop(28));
            auto selectorRow = content.removeFromTop(44);
            auto waveRow = selectorRow.removeFromLeft(320);
            const auto waveButtonWidth = waveRow.getWidth() / 4;
            sineWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            sawWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            squareWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            triangleWaveButton.setBounds(waveRow.reduced(3, 4));
            auto filterRow = selectorRow.removeFromLeft(180);
            const auto filterButtonWidth = filterRow.getWidth() / 3;
            lowpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            bandpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            highpassFilterButton.setBounds(filterRow.reduced(3, 4));
            filterCharacterBox.setBounds(selectorRow.removeFromLeft(126).reduced(4));
            filterSlopeBox.setBounds(selectorRow.removeFromLeft(82).reduced(4));
            monoButton.setBounds(selectorRow.removeFromLeft(90).reduced(4));
            auto osc2Row = content.removeFromTop(44).withTrimmedTop(2);
            const auto osc2WaveButtonWidth = osc2Row.getWidth() / 4;
            osc2SineWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SawWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SquareWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2TriangleWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            content.removeFromTop(8);
            setSliderVisible(osc1LevelSlider, osc1LevelLabel, true);
            setSliderVisible(osc2LevelSlider, osc2LevelLabel, true);
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(noiseLevelSlider, noiseLevelLabel, true);
            setSliderVisible(oscWarpSlider, oscWarpLabel, true);
            setSliderVisible(osc2OctaveSlider, osc2OctaveLabel, true);
            setSliderVisible(osc2TuneSlider, osc2TuneLabel, true);
            setSliderVisible(octaveSlider, octaveLabel, true);
            setSliderVisible(tuneSlider, tuneLabel, true);
            setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, true);
            setSliderVisible(unisonDetuneSlider, unisonDetuneLabel, true);
            setSliderVisible(unisonBlendSlider, unisonBlendLabel, true);
            setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, true);
            setSliderVisible(glideSlider, glideLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(resonanceSlider, resonanceLabel, true);
            setSliderVisible(filterEnvSlider, filterEnvLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            setSliderVisible(attackSlider, attackLabel, true);
            setSliderVisible(decaySlider, decayLabel, true);
            setSliderVisible(sustainSlider, sustainLabel, true);
            setSliderVisible(releaseSlider, releaseLabel, true);
            auto cards = content;
            auto topCards = cards.removeFromTop((cards.getHeight() - 10) / 2);
            cards.removeFromTop(10);
            auto bottomCards = cards;

            auto sourceArea = topCards.removeFromLeft(330).reduced(18, 10);
            auto voiceArea = topCards.reduced(18, 10);
            auto filterArea = bottomCards.removeFromLeft(330).reduced(18, 10);
            auto ampArea = bottomCards.reduced(18, 10);

            synthSourceLabel.setBounds(sourceArea.removeFromTop(22));
            layoutKnobRow(sourceArea.removeFromTop(104).withTrimmedTop(5), {
                &osc1LevelSlider,
                &osc2LevelSlider,
                &subLevelSlider,
                &noiseLevelSlider
            });

            synthVoiceLabel.setBounds(voiceArea.removeFromTop(22));
            layoutKnobRow(voiceArea.removeFromTop(72).withTrimmedTop(3), {
                &octaveSlider,
                &tuneSlider,
                &osc2OctaveSlider,
                &osc2TuneSlider,
                &glideSlider,
                &oscWarpSlider
            });
            layoutKnobRow(voiceArea.removeFromTop(72).withTrimmedTop(5), {
                &unisonVoicesSlider,
                &unisonDetuneSlider,
                &unisonBlendSlider,
                &unisonSpreadSlider
            });

            synthFilterLabel.setBounds(filterArea.removeFromTop(22));
            layoutKnobRow(filterArea.removeFromTop(104).withTrimmedTop(5), {
                &cutoffSlider,
                &resonanceSlider,
                &filterEnvSlider,
                &driveSlider
            });

            synthAmpLabel.setBounds(ampArea.removeFromTop(22));
            layoutKnobRow(ampArea.removeFromTop(104).withTrimmedTop(5), {
                &attackSlider,
                &decaySlider,
                &sustainSlider,
                &releaseSlider,
                &outputSlider
            });
            break;
        }

        case Panel::lab:
        {
            randomSectionLabel.setVisible(true);
            recipeBox.setVisible(true);
            randomScopeBox.setVisible(true);
            generateButton.setVisible(true);
            mutateButton.setVisible(true);
            variationButton.setVisible(true);
            wildMutateButton.setVisible(true);
            undoRandomButton.setVisible(true);
            redoRandomButton.setVisible(true);
            randomLockPitchButton.setVisible(true);
            randomLockEnvelopeButton.setVisible(true);
            randomLockFilterButton.setVisible(true);
            randomLockSourceButton.setVisible(true);
            randomLockSampleButton.setVisible(true);
            randomLockFxButton.setVisible(true);
            randomLockOutputButton.setVisible(true);
            randomLockSequencerButton.setVisible(true);
            randomStatusLabel.setVisible(true);
            randomSectionLabel.setBounds(content.removeFromTop(28));
            auto actionRow = content.removeFromTop(48);
            recipeBox.setBounds(actionRow.removeFromLeft(210).reduced(4));
            randomScopeBox.setBounds(actionRow.removeFromLeft(112).reduced(4));
            generateButton.setBounds(actionRow.removeFromLeft(96).reduced(4));
            mutateButton.setBounds(actionRow.removeFromLeft(96).reduced(4));
            variationButton.setBounds(actionRow.removeFromLeft(82).reduced(4));
            wildMutateButton.setBounds(actionRow.removeFromLeft(72).reduced(4));
            undoRandomButton.setBounds(actionRow.removeFromLeft(82).reduced(4));
            redoRandomButton.setBounds(actionRow.removeFromLeft(82).reduced(4));
            auto lockRow = content.removeFromTop(42).withTrimmedTop(4);
            const auto lockButtonWidth = lockRow.getWidth() / 8;
            randomLockPitchButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockEnvelopeButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockFilterButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockSourceButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockSampleButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockFxButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockOutputButton.setBounds(lockRow.removeFromLeft(lockButtonWidth).reduced(4));
            randomLockSequencerButton.setBounds(lockRow.reduced(4));
            randomStatusLabel.setBounds(content.removeFromTop(30).reduced(4));
            content.removeFromTop(12);
            setSliderVisible(randomAmountSlider, randomAmountLabel, true);
            setSliderVisible(randomChaosSlider, randomChaosLabel, true);
            setSliderVisible(brightnessSlider, brightnessLabel, true);
            setSliderVisible(driveBiasSlider, driveBiasLabel, true);
            setSliderVisible(motionBiasSlider, motionBiasLabel, true);
            layoutKnobRow(content.removeFromTop(150), { &randomAmountSlider, &randomChaosSlider, &brightnessSlider, &driveBiasSlider, &motionBiasSlider });
            break;
        }

        case Panel::mod:
        {
            modSectionLabel.setVisible(true);
            modSourceLabel.setVisible(true);
            modMacroLabel.setVisible(true);
            modLfoLabel.setVisible(true);
            modEnvelopeLabel.setVisible(true);
            modMatrixLabel.setVisible(true);
            lfo1ShapeBox.setVisible(true);
            lfo1SyncRateBox.setVisible(true);
            lfoCurvePresetBox.setVisible(true);
            lfo1SyncButton.setVisible(true);
            lfo1RetriggerButton.setVisible(true);
            lfoCurveDisplay.setVisible(true);

            modSectionLabel.setBounds(content.removeFromTop(28));

            for (auto& label : modSourceRows)
                label.setVisible(true);

            for (size_t index = 0; index < modSlotRows.size(); ++index)
            {
                modSlotRows[index].setVisible(true);
                modSourceBoxes[index].setVisible(true);
                modDestinationBoxes[index].setVisible(true);
                modAmountSliders[index].setVisible(true);
                modAmountLabels[index].setVisible(false);
                modMatrixRows[index].setVisible(true);
                modSlotEnabledButtons[index].setVisible(true);
                modSlotDeleteButtons[index].setVisible(true);
            }
            modMatrixStatusLabel.setVisible(true);
            modInspectorLabel.setVisible(true);
            modInspectorDestinationBox.setVisible(true);
            modInspectorSourceBox.setVisible(true);
            modInspectorStatusLabel.setVisible(true);
            modInspectorAddButton.setVisible(true);
            modInspectorClearButton.setVisible(true);
            modMacroAssignLabel.setVisible(true);
            modMacroAssignStatusLabel.setVisible(true);
            modMacroAssignSourceBox.setVisible(true);
            modMacroAssignDestinationBox.setVisible(true);
            modMacroAssignAmountSlider.setVisible(true);
            modMacroAssignAddButton.setVisible(true);
            modMacroAssignReplaceButton.setVisible(true);
            modMacroAssignClearButton.setVisible(true);
            modMatrixSourceHeader.setVisible(true);
            modMatrixDestinationHeader.setVisible(true);
            modMatrixAmountHeader.setVisible(true);
            modMatrixSourceHeaderB.setVisible(true);
            modMatrixDestinationHeaderB.setVisible(true);
            modMatrixAmountHeaderB.setVisible(true);

            auto modContent = content.withTrimmedTop(8);
            auto topRow = modContent.removeFromTop(modTopRowHeight);
            auto sourceArea = topRow.removeFromLeft(300).reduced(18, 8);
            auto macroArea = topRow.reduced(18, 8);

            modSourceLabel.setBounds(sourceArea.removeFromTop(18));
            auto sourceListArea = sourceArea.withTrimmedTop(2);
            auto leftSources = sourceListArea.removeFromLeft(sourceListArea.getWidth() / 2);
            auto rightSources = sourceListArea;
            const auto sourceColumnRows = (modSourceRows.size() + 1) / 2;
            const auto sourceRowHeight = juce::jmax(10,
                                                    sourceListArea.getHeight()
                                                        / juce::jmax(1, static_cast<int>(sourceColumnRows)));
            for (size_t index = 0; index < modSourceRows.size(); ++index)
            {
                auto& column = index < sourceColumnRows ? leftSources : rightSources;
                modSourceRows[index].setBounds(column.removeFromTop(sourceRowHeight).reduced(3, 1));
            }

            auto macroHeader = macroArea.removeFromTop(18);
            modMacroLabel.setBounds(macroHeader.removeFromLeft(74));
            modMacroAssignStatusLabel.setBounds(macroHeader.reduced(3, 0));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroMotionSlider, macroMotionLabel, true);
            setSliderVisible(macroSpaceSlider, macroSpaceLabel, true);
            setSliderVisible(macroWeightSlider, macroWeightLabel, true);
            setSliderVisible(macroBounceSlider, macroBounceLabel, true);
            setSliderVisible(macroWarpSlider, macroWarpLabel, true);
            setSliderVisible(macroThrowSlider, macroThrowLabel, true);
            layoutKnobRow(macroArea.removeFromTop(42).withTrimmedTop(1), { &macroToneSlider, &macroDirtSlider, &macroMotionSlider, &macroSpaceSlider });
            layoutKnobRow(macroArea.removeFromTop(42).withTrimmedTop(1), { &macroWeightSlider, &macroBounceSlider, &macroWarpSlider, &macroThrowSlider });
            auto macroAssignRow = macroArea.removeFromTop(30).withTrimmedTop(4);
            modMacroAssignLabel.setBounds(macroAssignRow.removeFromLeft(48).withTrimmedTop(5));
            modMacroAssignSourceBox.setBounds(macroAssignRow.removeFromLeft(88).reduced(3, 3));
            modMacroAssignDestinationBox.setBounds(macroAssignRow.removeFromLeft(124).reduced(3, 3));
            modMacroAssignAmountSlider.setBounds(macroAssignRow.removeFromLeft(114).reduced(3, 4));
            modMacroAssignAddButton.setBounds(macroAssignRow.removeFromLeft(48).reduced(3, 3));
            modMacroAssignReplaceButton.setBounds(macroAssignRow.removeFromLeft(66).reduced(3, 3));
            modMacroAssignClearButton.setBounds(macroAssignRow.removeFromLeft(52).reduced(3, 3));

            modContent.removeFromTop(modPanelGap);
            auto generatorRow = modContent.removeFromTop(modGeneratorRowHeight);
            auto lfoArea = generatorRow.removeFromLeft(450).reduced(18, 8);
            auto envelopeArea = generatorRow.reduced(18, 8);

            modLfoLabel.setBounds(lfoArea.removeFromTop(18));
            auto lfoModeRow = lfoArea.removeFromTop(24);
            lfo1ShapeBox.setBounds(lfoModeRow.removeFromLeft(86).reduced(3, 4));
            lfo1SyncRateBox.setBounds(lfoModeRow.removeFromLeft(78).reduced(3, 4));
            lfo1SyncButton.setBounds(lfoModeRow.removeFromLeft(58).reduced(3, 4));
            lfo1RetriggerButton.setBounds(lfoModeRow.removeFromLeft(72).reduced(3, 4));
            lfoCurvePresetBox.setBounds(lfoModeRow.removeFromLeft(130).reduced(3, 4));
            lfoCurveDisplay.setBounds(lfoArea.removeFromTop(42).withTrimmedTop(2));

            setSliderVisible(lfo1RateSlider, lfo1RateLabel, true);
            setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, true);
            setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, true);
            layoutKnobRow(lfoArea.removeFromTop(50).withTrimmedTop(2), { &lfo1RateSlider, &lfo1DepthSlider, &lfo1PhaseSlider });

            modEnvelopeLabel.setBounds(envelopeArea.removeFromTop(18));
            setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, true);
            setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, true);
            setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, true);
            setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, true);
            setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, true);
            layoutKnobRow(envelopeArea.removeFromTop(78).withTrimmedTop(2), {
                &modEnv1AttackSlider,
                &modEnv1DecaySlider,
                &modEnv1SustainSlider,
                &modEnv1ReleaseSlider,
                &modEnv1DepthSlider
            });

            modContent.removeFromTop(modPanelGap);
            auto matrixArea = modContent.reduced(18, 0);
            auto matrixTitleRow = matrixArea.removeFromTop(28);
            modMatrixLabel.setBounds(matrixTitleRow.removeFromLeft(70).withTrimmedTop(5));
            modMatrixStatusLabel.setBounds(matrixTitleRow.removeFromLeft(150).reduced(3, 5));
            modInspectorStatusLabel.setBounds(matrixTitleRow.reduced(5, 5));

            auto inspectorRow = matrixArea.removeFromTop(30);
            modInspectorLabel.setBounds(inspectorRow.removeFromLeft(62).withTrimmedTop(6));
            modInspectorDestinationBox.setBounds(inspectorRow.removeFromLeft(150).reduced(3, 4));
            modInspectorSourceBox.setBounds(inspectorRow.removeFromLeft(138).reduced(3, 4));
            modInspectorAddButton.setBounds(inspectorRow.removeFromLeft(58).reduced(3, 4));
            modInspectorClearButton.setBounds(inspectorRow.removeFromLeft(66).reduced(3, 4));

            auto matrixHeaderRow = matrixArea.removeFromTop(18).reduced(3, 1);
            auto leftHeader = matrixHeaderRow.removeFromLeft((matrixHeaderRow.getWidth() - 10) / 2);
            matrixHeaderRow.removeFromLeft(10);
            auto rightHeader = matrixHeaderRow;
            auto placeHeader = [] (juce::Rectangle<int> header,
                                   juce::Label& source,
                                   juce::Label& destination,
                                   juce::Label& amount)
            {
                header.removeFromLeft(26);
                header.removeFromRight(52);
                source.setBounds(header.removeFromLeft(100).reduced(5, 0));
                destination.setBounds(header.removeFromLeft(138).reduced(5, 0));
                amount.setBounds(header.reduced(5, 0));
            };
            placeHeader(leftHeader, modMatrixSourceHeader, modMatrixDestinationHeader, modMatrixAmountHeader);
            placeHeader(rightHeader, modMatrixSourceHeaderB, modMatrixDestinationHeaderB, modMatrixAmountHeaderB);

            auto leftBank = matrixArea.removeFromLeft((matrixArea.getWidth() - 10) / 2);
            matrixArea.removeFromLeft(10);
            auto rightBank = matrixArea;
            auto placeRouteRow = [this] (size_t index, juce::Rectangle<int>& bank, int rowsRemaining)
            {
                const auto rowHeight = juce::jmax(22, bank.getHeight() / rowsRemaining);
                auto rowBounds = bank.removeFromTop(rowHeight).reduced(3, 1);
                modMatrixRows[index].setBounds(rowBounds);

                auto row = rowBounds.reduced(2, 2);
                modSlotRows[index].setBounds(row.removeFromLeft(26).reduced(2, 0));
                modSourceBoxes[index].setBounds(row.removeFromLeft(100).reduced(3, 0));
                modDestinationBoxes[index].setBounds(row.removeFromLeft(138).reduced(3, 0));
                auto actionArea = row.removeFromRight(52);
                modSlotEnabledButtons[index].setBounds(actionArea.removeFromLeft(31).reduced(1, 0));
                modSlotDeleteButtons[index].setBounds(actionArea.reduced(1, 0));
                modAmountSliders[index].setBounds(row.reduced(3, 0));
            };

            for (size_t index = 0; index < 4; ++index)
                placeRouteRow(index, leftBank, static_cast<int>(4 - index));

            for (size_t index = 4; index < modSlotRows.size(); ++index)
                placeRouteRow(index, rightBank, static_cast<int>(modSlotRows.size() - index));

            break;
        }

        case Panel::sample:
        {
            sampleSectionLabel.setVisible(true);
            sampleSourceLabel.setVisible(true);
            sampleChopLabel.setVisible(true);
            sampleShapeLabel.setVisible(true);
            loadSampleButton.setVisible(true);
            clearSampleButton.setVisible(true);
            randomCutButton.setVisible(true);
            ukgChopButton.setVisible(true);
            sampleEnabledButton.setVisible(true);
            sampleReverseButton.setVisible(true);
            sampleModeBox.setVisible(true);
            sampleSliceStyleBox.setVisible(true);
            sampleStutterEnabledButton.setVisible(true);
            sampleStutterRateBox.setVisible(true);
            sampleWaveformDisplay.setVisible(true);
            for (auto& button : sampleSliceButtons)
                button.setVisible(true);
            sampleNameLabel.setVisible(true);
            sampleSectionLabel.setBounds(content.removeFromTop(28));
            sampleSourceLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            auto actionRow = content.removeFromTop(38);
            loadSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            clearSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            randomCutButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            ukgChopButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            sampleEnabledButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleReverseButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleModeBox.setBounds(actionRow.removeFromLeft(112).reduced(4));
            auto stutterRow = content.removeFromTop(34).withTrimmedTop(2);
            sampleNameLabel.setBounds(stutterRow.removeFromLeft(330).reduced(8, 4));
            sampleSliceStyleBox.setBounds(stutterRow.removeFromLeft(142).reduced(4));
            sampleStutterEnabledButton.setBounds(stutterRow.removeFromLeft(96).reduced(4));
            sampleStutterRateBox.setBounds(stutterRow.removeFromLeft(108).reduced(4));
            sampleChopLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            sampleWaveformDisplay.setBounds(content.removeFromTop(100).reduced(4, 6));
            auto sliceRow = content.removeFromTop(34).withTrimmedTop(2);
            const auto sliceWidth = sliceRow.getWidth() / static_cast<int>(sampleSliceButtons.size());
            for (auto& button : sampleSliceButtons)
                button.setBounds(sliceRow.removeFromLeft(sliceWidth).reduced(4));
            auto cutRow = content.removeFromTop(42).withTrimmedTop(4);
            setSliderVisible(sampleStartSlider, sampleStartLabel, true);
            setSliderVisible(sampleEndSlider, sampleEndLabel, true);
            sampleStartSlider.setBounds(cutRow.removeFromLeft(cutRow.getWidth() / 2).reduced(48, 6));
            sampleEndSlider.setBounds(cutRow.reduced(48, 6));
            sampleShapeLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, true);
            setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, true);
            setSliderVisible(sampleGainSlider, sampleGainLabel, true);
            setSliderVisible(sampleMixSlider, sampleMixLabel, true);
            setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, true);
            layoutKnobRow(content.removeFromTop(78), { &sampleTransposeSlider, &sampleGainSlider, &sampleMixSlider });
            layoutKnobRow(content.removeFromTop(78).withTrimmedTop(4), { &samplePitchRampSlider, &sampleStutterRepeatsSlider });
            break;
        }

        case Panel::sequencer:
        {
            sequencerSectionLabel.setVisible(true);
            sequencerEnabledButton.setVisible(true);
            hostSyncStatusLabel.setVisible(true);
            rateEighthButton.setVisible(true);
            rateSixteenthButton.setVisible(true);
            rateThirtySecondButton.setVisible(true);
            sequencerGrooveBox.setVisible(true);
            sequencerScaleBox.setVisible(true);
            sequencerChordBox.setVisible(true);
            sequencerVoicingBox.setVisible(true);
            sequencerChordMemoryButton.setVisible(true);
            sequencerPatternBox.setVisible(true);
            sequencerGrooveTransformBox.setVisible(true);
            applyPatternButton.setVisible(true);
            copySequencerButton.setVisible(true);
            rotateSequencerLeftButton.setVisible(true);
            rotateSequencerRightButton.setVisible(true);
            exportSequencerMidiButton.setVisible(true);
            applyGrooveTransformButton.setVisible(true);
            randomSequencerButton.setVisible(true);
            mutateSequencerButton.setVisible(true);
            undoSequencerButton.setVisible(true);
            clearSequencerButton.setVisible(true);
            sequencerGrid.setVisible(true);
            sequencerSectionLabel.setBounds(content.removeFromTop(28));
            auto timingRow = content.removeFromTop(44);
            sequencerEnabledButton.setBounds(timingRow.removeFromLeft(62).reduced(4));
            auto rateRow = timingRow.removeFromLeft(150);
            const auto rateButtonWidth = rateRow.getWidth() / 3;
            rateEighthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateSixteenthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateThirtySecondButton.setBounds(rateRow.reduced(3, 4));
            sequencerGrooveBox.setBounds(timingRow.removeFromLeft(124).reduced(4));
            sequencerScaleBox.setBounds(timingRow.removeFromLeft(112).reduced(4));
            sequencerChordBox.setBounds(timingRow.removeFromLeft(112).reduced(4));
            sequencerVoicingBox.setBounds(timingRow.removeFromLeft(104).reduced(4));
            sequencerChordMemoryButton.setBounds(timingRow.removeFromLeft(92).reduced(4));
            hostSyncStatusLabel.setBounds(timingRow.reduced(4));
            auto patternRow = content.removeFromTop(42).withTrimmedTop(2);
            sequencerPatternBox.setBounds(patternRow.removeFromLeft(230).reduced(4));
            applyPatternButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            randomSequencerButton.setBounds(patternRow.removeFromLeft(108).reduced(4));
            mutateSequencerButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            undoSequencerButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            clearSequencerButton.setBounds(patternRow.removeFromLeft(82).reduced(4));
            auto utilityRow = content.removeFromTop(32).withTrimmedTop(2);
            copySequencerButton.setBounds(utilityRow.removeFromLeft(72).reduced(4));
            rotateSequencerLeftButton.setBounds(utilityRow.removeFromLeft(58).reduced(4));
            rotateSequencerRightButton.setBounds(utilityRow.removeFromLeft(58).reduced(4));
            exportSequencerMidiButton.setBounds(utilityRow.removeFromLeft(72).reduced(4));
            sequencerGrooveTransformBox.setBounds(utilityRow.removeFromLeft(184).reduced(4));
            applyGrooveTransformButton.setBounds(utilityRow.removeFromLeft(76).reduced(4));
            setSliderVisible(sequencerRootSlider, sequencerRootLabel, true);
            setSliderVisible(sequencerGateSlider, sequencerGateLabel, true);
            setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, true);
            setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, true);
            setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, true);
            setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, true);
            setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, true);
            setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, true);
            layoutKnobRow(content.removeFromTop(84).withTrimmedTop(6), {
                &sequencerRootSlider,
                &sequencerGateSlider,
                &sequencerSwingSlider,
                &sequencerChordStrumSlider,
                &sequencerAccentSlider,
                &sequencerOctaveSlider,
                &sequencerProbabilitySlider,
                &sequencerRandomSlider
            });
            sequencerGrid.setBounds(content.reduced(4, 12));
            break;
        }

        case Panel::effects:
        {
            futureSectionLabel.setVisible(true);
            futureSectionLabel.setBounds(content.removeFromTop(28));
            updateFxRackControls();

            auto actionRow = content.removeFromTop(44);
            fxAddBox.setVisible(true);
            fxMoveUpButton.setVisible(true);
            fxMoveDownButton.setVisible(true);
            fxResetOrderButton.setVisible(true);
            fxRemoveButton.setVisible(true);
            fxThrowDelayButton.setVisible(true);
            fxThrowSpaceButton.setVisible(true);
            fxThrowPumpButton.setVisible(true);
            fxThrowDryButton.setVisible(true);
            fxHoldDelayButton.setVisible(true);
            fxHoldSpaceButton.setVisible(true);
            fxHoldPumpButton.setVisible(true);
            fxMuteDropButton.setVisible(true);
            fxRackStatusLabel.setVisible(true);
            fxPresetBox.setVisible(true);
            fxApplyPresetButton.setVisible(true);
            hostSyncStatusLabel.setVisible(true);
            fxAddBox.setBounds(actionRow.removeFromLeft(160).reduced(4));
            fxMoveUpButton.setBounds(actionRow.removeFromLeft(52).reduced(4));
            fxMoveDownButton.setBounds(actionRow.removeFromLeft(58).reduced(4));
            fxResetOrderButton.setBounds(actionRow.removeFromLeft(72).reduced(4));
            fxRemoveButton.setBounds(actionRow.removeFromLeft(86).reduced(4));
            hostSyncStatusLabel.setBounds(actionRow.removeFromRight(126).reduced(4));
            fxRackStatusLabel.setBounds(actionRow.reduced(8, 4));

            auto performRow = content.removeFromTop(42).withTrimmedTop(2);
            fxThrowDelayButton.setBounds(performRow.removeFromLeft(102).reduced(4));
            fxThrowSpaceButton.setBounds(performRow.removeFromLeft(106).reduced(4));
            fxThrowPumpButton.setBounds(performRow.removeFromLeft(96).reduced(4));
            fxThrowDryButton.setBounds(performRow.removeFromLeft(88).reduced(4));
            fxHoldDelayButton.setBounds(performRow.removeFromLeft(84).reduced(4));
            fxHoldSpaceButton.setBounds(performRow.removeFromLeft(84).reduced(4));
            fxHoldPumpButton.setBounds(performRow.removeFromLeft(92).reduced(4));
            fxMuteDropButton.setBounds(performRow.removeFromLeft(90).reduced(4));

            content.removeFromTop(8);
            auto rackArea = content.removeFromLeft(260).reduced(18, 14);
            rackArea.removeFromTop(26);
            auto detailArea = content.reduced(24, 16);
            detailArea.removeFromTop(30);

            std::array<UI::FxRackRow*, 15> visibleFxSlots {};
            auto visibleFxSlotCount = 0;

            const auto moduleOrder = fxModuleOrder();
            for (const auto module : moduleOrder)
            {
                auto& slotButton = fxSlotButton(module);
                const auto isVisible = shouldShowFxModule(module);
                slotButton.setVisible(isVisible);

                if (isVisible)
                    visibleFxSlots[static_cast<size_t>(visibleFxSlotCount++)] = &slotButton;
            }

            const auto useTwoColumnRack = visibleFxSlotCount > 10;
            const auto slotGap = useTwoColumnRack ? 4 : 6;
            const auto columnGap = useTwoColumnRack ? 6 : 0;
            const auto rackColumnCount = useTwoColumnRack ? 2 : 1;
            const auto rackRowCount = visibleFxSlotCount > 0
                ? (visibleFxSlotCount + rackColumnCount - 1) / rackColumnCount
                : 0;
            const auto slotHeight = rackRowCount > 0
                ? juce::jlimit(24,
                               34,
                               (rackArea.getHeight() - ((rackRowCount - 1) * slotGap)) / rackRowCount)
                : 34;
            const auto slotWidth = (rackArea.getWidth() - columnGap) / rackColumnCount;

            for (auto index = 0; index < visibleFxSlotCount; ++index)
            {
                const auto column = useTwoColumnRack ? index / rackRowCount : 0;
                const auto row = useTwoColumnRack ? index % rackRowCount : index;
                const auto x = rackArea.getX() + (column * (slotWidth + columnGap));
                const auto y = rackArea.getY() + (row * (slotHeight + slotGap));
                visibleFxSlots[static_cast<size_t>(index)]->setBounds(x, y, slotWidth, slotHeight);
            }

            auto detailHeader = detailArea.removeFromTop(38);
            fxApplyPresetButton.setBounds(detailHeader.removeFromRight(62).reduced(3, 4));
            fxPresetBox.setBounds(detailHeader.removeFromRight(156).reduced(3, 4));
            auto controlsArea = detailArea.withTrimmedTop(16);

            switch (selectedFxModule)
            {
                case FxModule::tone:
                    fxToneEnabledButton.setVisible(true);
                    fxToneEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxToneTiltSlider, fxToneTiltLabel, true);
                    setSliderVisible(fxToneLowCutSlider, fxToneLowCutLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxToneTiltSlider, &fxToneLowCutSlider });
                    break;

                case FxModule::eq:
                    fxEqEnabledButton.setVisible(true);
                    fxEqEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxEqLowGainSlider, fxEqLowGainLabel, true);
                    setSliderVisible(fxEqMidGainSlider, fxEqMidGainLabel, true);
                    setSliderVisible(fxEqHighGainSlider, fxEqHighGainLabel, true);
                    setSliderVisible(fxEqTrimSlider, fxEqTrimLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxEqLowGainSlider,
                        &fxEqMidGainSlider,
                        &fxEqHighGainSlider,
                        &fxEqTrimSlider
                    });
                    break;

                case FxModule::distortion:
                    fxDistortionEnabledButton.setVisible(true);
                    fxDistortionEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxDistortionAmountSlider, fxDistortionAmountLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxDistortionAmountSlider });
                    break;

                case FxModule::bitcrush:
                    fxBitcrushEnabledButton.setVisible(true);
                    fxBitcrushEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxBitcrushBitsSlider, fxBitcrushBitsLabel, true);
                    setSliderVisible(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, true);
                    setSliderVisible(fxBitcrushMixSlider, fxBitcrushMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxBitcrushBitsSlider, &fxBitcrushDownsampleSlider, &fxBitcrushMixSlider });
                    break;

                case FxModule::pump:
                    fxPumpEnabledButton.setVisible(true);
                    fxPumpRateBox.setVisible(true);
                    fxPumpCurveBox.setVisible(true);
                    pumpCurveDisplay.setVisible(true);
                    fxPumpEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    fxPumpRateBox.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
                    fxPumpCurveBox.setBounds(detailHeader.removeFromLeft(118).reduced(3, 4));
                    pumpCurveDisplay.setBounds(controlsArea.removeFromTop(86).reduced(4, 2));
                    setSliderVisible(fxPumpDepthSlider, fxPumpDepthLabel, true);
                    setSliderVisible(fxPumpShapeSlider, fxPumpShapeLabel, true);
                    setSliderVisible(fxPumpPhaseSlider, fxPumpPhaseLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(128).withTrimmedTop(8), { &fxPumpDepthSlider, &fxPumpShapeSlider, &fxPumpPhaseSlider });
                    break;

                case FxModule::tremolo:
                    fxTremoloEnabledButton.setVisible(true);
                    fxTremoloRateBox.setVisible(true);
                    fxTremoloEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    fxTremoloRateBox.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxTremoloDepthSlider, fxTremoloDepthLabel, true);
                    setSliderVisible(fxTremoloPanSlider, fxTremoloPanLabel, true);
                    setSliderVisible(fxTremoloShapeSlider, fxTremoloShapeLabel, true);
                    setSliderVisible(fxTremoloPhaseSlider, fxTremoloPhaseLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxTremoloDepthSlider,
                        &fxTremoloPanSlider,
                        &fxTremoloShapeSlider,
                        &fxTremoloPhaseSlider
                    });
                    break;

                case FxModule::ring:
                    fxRingEnabledButton.setVisible(true);
                    fxRingEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxRingFrequencySlider, fxRingFrequencyLabel, true);
                    setSliderVisible(fxRingDepthSlider, fxRingDepthLabel, true);
                    setSliderVisible(fxRingMixSlider, fxRingMixLabel, true);
                    setSliderVisible(fxRingBiasSlider, fxRingBiasLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxRingFrequencySlider,
                        &fxRingDepthSlider,
                        &fxRingMixSlider,
                        &fxRingBiasSlider
                    });
                    break;

                case FxModule::comb:
                    fxCombEnabledButton.setVisible(true);
                    fxCombEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxCombFrequencySlider, fxCombFrequencyLabel, true);
                    setSliderVisible(fxCombFeedbackSlider, fxCombFeedbackLabel, true);
                    setSliderVisible(fxCombDampingSlider, fxCombDampingLabel, true);
                    setSliderVisible(fxCombMixSlider, fxCombMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxCombFrequencySlider,
                        &fxCombFeedbackSlider,
                        &fxCombDampingSlider,
                        &fxCombMixSlider
                    });
                    break;

                case FxModule::phaser:
                    fxPhaserEnabledButton.setVisible(true);
                    fxPhaserEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxPhaserRateSlider, fxPhaserRateLabel, true);
                    setSliderVisible(fxPhaserDepthSlider, fxPhaserDepthLabel, true);
                    setSliderVisible(fxPhaserMixSlider, fxPhaserMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxPhaserRateSlider, &fxPhaserDepthSlider, &fxPhaserMixSlider });
                    break;

                case FxModule::flanger:
                    fxFlangerEnabledButton.setVisible(true);
                    fxFlangerEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxFlangerRateSlider, fxFlangerRateLabel, true);
                    setSliderVisible(fxFlangerDepthSlider, fxFlangerDepthLabel, true);
                    setSliderVisible(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, true);
                    setSliderVisible(fxFlangerMixSlider, fxFlangerMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxFlangerRateSlider,
                        &fxFlangerDepthSlider,
                        &fxFlangerFeedbackSlider,
                        &fxFlangerMixSlider
                    });
                    break;

                case FxModule::chorus:
                    fxChorusEnabledButton.setVisible(true);
                    fxChorusEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxChorusRateSlider, fxChorusRateLabel, true);
                    setSliderVisible(fxChorusDepthSlider, fxChorusDepthLabel, true);
                    setSliderVisible(fxChorusMixSlider, fxChorusMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxChorusRateSlider, &fxChorusDepthSlider, &fxChorusMixSlider });
                    break;

                case FxModule::delay:
                    fxDelayEnabledButton.setVisible(true);
                    fxDelaySyncButton.setVisible(true);
                    fxDelayRateBox.setVisible(true);
                    fxDelayEnabledButton.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
                    fxDelaySyncButton.setBounds(detailHeader.removeFromLeft(72).reduced(3, 4));
                    fxDelayRateBox.setBounds(detailHeader.removeFromLeft(104).reduced(3, 4));
                    setSliderVisible(fxDelayTimeSlider, fxDelayTimeLabel, true);
                    setSliderVisible(fxDelayFeedbackSlider, fxDelayFeedbackLabel, true);
                    setSliderVisible(fxDelayMixSlider, fxDelayMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxDelayTimeSlider, &fxDelayFeedbackSlider, &fxDelayMixSlider });
                    break;

                case FxModule::reverb:
                    fxReverbEnabledButton.setVisible(true);
                    fxReverbEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxReverbSizeSlider, fxReverbSizeLabel, true);
                    setSliderVisible(fxReverbDampingSlider, fxReverbDampingLabel, true);
                    setSliderVisible(fxReverbMixSlider, fxReverbMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxReverbSizeSlider, &fxReverbDampingSlider, &fxReverbMixSlider });
                    break;

                case FxModule::width:
                    fxWidthEnabledButton.setVisible(true);
                    fxWidthEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxWidthAmountSlider, fxWidthAmountLabel, true);
                    setSliderVisible(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxWidthAmountSlider, &fxWidthMonoCutoffSlider });
                    break;

                case FxModule::guard:
                    fxGuardEnabledButton.setVisible(true);
                    fxGuardEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxGuardPushSlider, fxGuardPushLabel, true);
                    setSliderVisible(fxGuardCeilingSlider, fxGuardCeilingLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxGuardPushSlider, &fxGuardCeilingSlider });
                    break;
            }
            break;
        }

        case Panel::library:
        {
            librarySectionLabel.setVisible(true);
            presetNameEditor.setVisible(true);
            presetCategoryBox.setVisible(true);
            presetFilterBox.setVisible(true);
            presetTagBox.setVisible(true);
            presetSortBox.setVisible(true);
            presetRatingBox.setVisible(true);
            presetPackBox.setVisible(true);
            presetKeyBox.setVisible(true);
            presetBpmBox.setVisible(true);
            presetSearchEditor.setVisible(true);
            presetAuthorEditor.setVisible(true);
            savePresetButton.setVisible(true);
            presetBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            auditionPresetButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            refreshPresetsButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            presetBrowserHeaderLabel.setVisible(true);
            presetBrowserList.setVisible(true);
            librarySectionLabel.setBounds(content.removeFromTop(28));
            auto saveRow = content.removeFromTop(48);
            presetCategoryBox.setBounds(saveRow.removeFromLeft(172).reduced(4));
            presetNameEditor.setBounds(saveRow.removeFromLeft(276).reduced(4));
            savePresetButton.setBounds(saveRow.removeFromLeft(82).reduced(4));
            presetRatingBox.setBounds(saveRow.removeFromLeft(118).reduced(4));
            auto metadataRow = content.removeFromTop(44).withTrimmedTop(4);
            presetAuthorEditor.setBounds(metadataRow.removeFromLeft(150).reduced(4));
            presetPackBox.setBounds(metadataRow.removeFromLeft(190).reduced(4));
            presetKeyBox.setBounds(metadataRow.removeFromLeft(120).reduced(4));
            presetBpmBox.setBounds(metadataRow.removeFromLeft(118).reduced(4));
            auto filterRow = content.removeFromTop(46).withTrimmedTop(6);
            presetFilterBox.setBounds(filterRow.removeFromLeft(132).reduced(4));
            presetTagBox.setBounds(filterRow.removeFromLeft(150).reduced(4));
            presetSortBox.setBounds(filterRow.removeFromLeft(118).reduced(4));
            presetSearchEditor.setBounds(filterRow.removeFromLeft(260).reduced(4));
            refreshPresetsButton.setBounds(filterRow.removeFromLeft(90).reduced(4));
            auto loadRow = content.removeFromTop(46).withTrimmedTop(6);
            previousPresetButton.setBounds(loadRow.removeFromLeft(40).reduced(4));
            presetBox.setBounds(loadRow.removeFromLeft(318).reduced(4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(40).reduced(4));
            loadPresetButton.setBounds(loadRow.removeFromLeft(78).reduced(4));
            auditionPresetButton.setBounds(loadRow.removeFromLeft(88).reduced(4));
            favoritePresetButton.setBounds(loadRow.removeFromLeft(62).reduced(4));
            presetStatusLabel.setBounds(content.removeFromTop(36).reduced(6, 4));
            presetBrowserHeaderLabel.setBounds(content.removeFromTop(24).reduced(8, 2));
            presetBrowserList.setBounds(content.reduced(6, 4));
            break;
        }
    }
}

bool NateVSTAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& filePath : files)
    {
        const auto file = juce::File(filePath);
        const auto extension = file.getFileExtension().toLowerCase();
        if (extension == ".wav" || extension == ".aif" || extension == ".aiff")
            return true;
    }

    return false;
}

void NateVSTAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& filePath : files)
    {
        const auto file = juce::File(filePath);
        const auto extension = file.getFileExtension().toLowerCase();
        if (extension == ".wav" || extension == ".aif" || extension == ".aiff")
        {
            loadSampleFile(file);
            return;
        }
    }
}

void NateVSTAudioProcessorEditor::configureSlider(juce::Slider& slider,
                                                    juce::Label& label,
                                                    const juce::String& labelText,
                                                    const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setMouseDragSensitivity(rotaryDragSensitivityForParameter(parameterID));
    applyFineDragMode(slider, 0.32);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setTooltip(controlFeelTooltip(labelText));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureHorizontalSlider(juce::Slider& slider,
                                                              juce::Label& label,
                                                              const juce::String& labelText,
                                                              const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(150);
    applyFineDragMode(slider, 0.40);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
    slider.setTooltip(controlFeelTooltip(labelText));
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    label.attachToComponent(&slider, true);
    addAndMakeVisible(label);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureCompactHorizontalSlider(juce::Slider& slider,
                                                                    const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(110);
    applyFineDragMode(slider, 0.38);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 16);
    slider.setTooltip("Curve point: drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.");
    slider.setNumDecimalPlacesToDisplay(2);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
    addAndMakeVisible(label);
}

juce::Rectangle<int> NateVSTAudioProcessorEditor::layoutKnobRow(juce::Rectangle<int> area,
                                                                  std::initializer_list<juce::Component*> components)
{
    const auto count = static_cast<int>(components.size());
    if (count == 0)
        return area;

    const auto cellWidth = juce::jmax(1, area.getWidth() / count);
    const auto horizontalPadding = cellWidth < 64 ? 2 : 4;

    for (auto* component : components)
    {
        component->setVisible(true);
        component->setBounds(area.removeFromLeft(cellWidth).reduced(horizontalPadding, 0));
    }

    return area;
}

void NateVSTAudioProcessorEditor::chooseSampleFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load sample",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this] (const juce::FileChooser& chooser)
                             {
                                 const auto file = chooser.getResult();
                                 if (file.existsAsFile())
                                     loadSampleFile(file);
                             });
}

void NateVSTAudioProcessorEditor::exportSequencerMidiClip()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export sequencer MIDI",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Nate VST Sequence.mid"),
        "*.mid;*.midi");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [this] (const juce::FileChooser& chooser)
                             {
                                 auto file = chooser.getResult();
                                 if (file == juce::File{})
                                     return;

                                 if (! file.hasFileExtension(".mid;.midi"))
                                     file = file.withFileExtension(".mid");

                                 setRandomStatus(audioProcessor.exportSequencerMidiFile(file)
                                                     ? "MIDI exported"
                                                     : "MIDI export skipped");
                             });
}

void NateVSTAudioProcessorEditor::loadSampleFile(const juce::File& file)
{
    if (audioProcessor.loadSampleFile(file))
    {
        sampleWaveformKey.clear();
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
    }
}

void NateVSTAudioProcessorEditor::updateSampleNameLabel()
{
    const auto sampleName = audioProcessor.getLoadedSampleName();
    sampleNameLabel.setText(sampleName.isNotEmpty() ? sampleName : "No sample", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::selectSampleSlice(size_t sliceIndex)
{
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());
    if (sliceCount <= 0.0f)
        return;

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
    const auto start = static_cast<float>(safeIndex) / sliceCount;
    const auto end = static_cast<float>(safeIndex + 1) / sliceCount;
    const auto styleIndex = juce::jlimit(0, 4, sampleSliceStyleBox.getSelectedItemIndex());
    const auto slicePosition = static_cast<int>(safeIndex);
    const std::array<float, 8> pitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
    const std::array<float, 8> garagePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };
    const std::array<float, 8> garageRamp { 7.0f, 0.0f, -5.0f, 12.0f, 0.0f, -7.0f, 5.0f, -12.0f };

    setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::sampleStart, start);
    setPlainParameterValue(Parameters::ID::sampleEnd, end);
    setPlainParameterValue(Parameters::ID::samplePlaybackMode, 1.0f);

    switch (styleIndex)
    {
        case 1: // Pitch
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, pitchLadder[safeIndex]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -7.0f + static_cast<float>(slicePosition % 3));
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;

        case 2: // Reverse
            setPlainParameterValue(Parameters::ID::sampleReverse, (slicePosition % 2) == 0 ? 0.0f : 1.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, pitchLadder[static_cast<size_t>((slicePosition + 2) % 8)]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, (slicePosition % 3) == 0 ? -7.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -7.5f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;

        case 3: // Stutter
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, garagePitch[static_cast<size_t>((slicePosition + 1) % 8)] * 0.5f);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -8.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 1.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRate, static_cast<float>((slicePosition % 2) + 1));
            setPlainParameterValue(Parameters::ID::sampleStutterRepeats, static_cast<float>(2 + (slicePosition % 4)));
            break;

        case 4: // Garage
            setPlainParameterValue(Parameters::ID::sampleReverse, (slicePosition == 2 || slicePosition == 6) ? 1.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, garagePitch[safeIndex]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, garageRamp[safeIndex]);
            setPlainParameterValue(Parameters::ID::sampleGain, -8.5f + static_cast<float>(slicePosition % 4) * 0.8f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, (slicePosition == 3 || slicePosition == 7) ? 1.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRate, (slicePosition == 7) ? 2.0f : 1.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRepeats, (slicePosition == 7) ? 5.0f : 3.0f);
            break;

        case 0: // Clean
        default:
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, 0.0f);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -6.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;
    }

    const auto didAudition = audioProcessor.triggerSampleAudition();
    setRandomStatus("Slice " + juce::String(static_cast<int>(safeIndex + 1)) + " " + sampleSliceStyleBox.getText() + (didAudition ? " auditioned" : " selected"));
    updateSampleSliceButtons();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::updateSampleSliceButtons()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    const auto start = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleStart, 0.0f));
    const auto end = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleEnd, 1.0f));
    const auto orderedStart = juce::jmin(start, end);
    const auto orderedEnd = juce::jmax(start, end);
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());

    for (size_t index = 0; index < sampleSliceButtons.size(); ++index)
    {
        const auto sliceStart = static_cast<float>(index) / sliceCount;
        const auto sliceEnd = static_cast<float>(index + 1) / sliceCount;
        const auto isSelected = std::abs(orderedStart - sliceStart) < 0.005f
            && std::abs(orderedEnd - sliceEnd) < 0.005f;
        sampleSliceButtons[index].setToggleState(isSelected, juce::dontSendNotification);
    }
}

void NateVSTAudioProcessorEditor::updateSampleWaveformDisplay()
{
    const auto sampleName = audioProcessor.getLoadedSampleName();
    if (sampleName.isEmpty())
    {
        if (sampleWaveformKey.isNotEmpty())
        {
            sampleWaveformDisplay.setOverview({});
            sampleWaveformKey.clear();
        }

        sampleWaveformDisplay.setRange(0.0f, 1.0f);
        return;
    }

    if (sampleWaveformKey != sampleName)
    {
        sampleWaveformDisplay.setOverview(audioProcessor.createSamplePeakOverview(256));
        sampleWaveformKey = sampleName;
    }

    const auto start = readPlainParameterValue(Parameters::ID::sampleStart, 0.0f);
    const auto end = readPlainParameterValue(Parameters::ID::sampleEnd, 1.0f);
    sampleWaveformDisplay.setRange(start, end);
}

int NateVSTAudioProcessorEditor::selectedRandomMutationScope() const
{
    return juce::jmax(0, randomScopeBox.getSelectedId() - 1);
}

void NateVSTAudioProcessorEditor::setRandomStatus(const juce::String& action)
{
    const auto locks = audioProcessor.getActiveRandomizationLockSummary();
    const auto history = audioProcessor.getRandomHistorySummary();
    juce::StringArray details;
    if (history.isNotEmpty())
        details.add(history);
    if (randomScopeBox.getSelectedId() > 1)
        details.add("Scope: " + randomScopeBox.getText());
    details.add(locks.isNotEmpty() ? "Locked: " + locks : "No locks");
    randomStatusLabel.setText(action + " | " + details.joinIntoString(" | "), juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::shiftKeyboardOctave(int semitones)
{
    const auto currentLowestNote = pianoKeyboard.getLowestVisibleKey();
    const auto nextLowestNote = juce::jlimit(keyboardLowestNote,
                                            keyboardMaxLowestVisibleNote,
                                            currentLowestNote + semitones);

    pianoKeyboard.setLowestVisibleKey(nextLowestNote);
    updateKeyboardRangeLabel();
}

void NateVSTAudioProcessorEditor::updateKeyboardRangeLabel()
{
    const auto lowestVisibleNote = pianoKeyboard.getLowestVisibleKey();
    const auto noteName = juce::MidiMessage::getMidiNoteName(lowestVisibleNote, true, true, 3);

    if (keyboardRangeLabel.getText() != noteName)
        keyboardRangeLabel.setText(noteName, juce::dontSendNotification);

    keyboardOctaveDownButton.setEnabled(lowestVisibleNote > keyboardLowestNote);
    keyboardOctaveUpButton.setEnabled(lowestVisibleNote < keyboardMaxLowestVisibleNote);
}

void NateVSTAudioProcessorEditor::addFxModule(FxModule module)
{
    setPlainParameterValue(fxEnabledParameterID(module), 1.0f);
    selectedFxModule = module;
    fxAddBox.setSelectedId(0, juce::dontSendNotification);
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::removeSelectedFxModule()
{
    if (selectedFxModule == FxModule::guard)
    {
        fxRackStatusLabel.setText("Guard stays available as the output safety module", juce::dontSendNotification);
        return;
    }

    setPlainParameterValue(fxEnabledParameterID(selectedFxModule), 0.0f);

    selectedFxModule = FxModule::guard;

    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::selectFxModule(FxModule module)
{
    selectedFxModule = module;
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::moveSelectedFxModule(int direction)
{
    if (selectedFxModule == FxModule::guard)
    {
        fxRackStatusLabel.setText("Guard stays last as the output safety stage", juce::dontSendNotification);
        return;
    }

    auto order = fxModuleOrder();
    const auto position = fxOrderPosition(selectedFxModule) - 1;
    const auto lastMovablePosition = static_cast<int>(order.size()) - 2;

    if (position < 0)
        return;

    const auto step = direction < 0 ? -1 : 1;
    auto targetPosition = position + step;

    while (targetPosition >= 0
           && targetPosition <= lastMovablePosition
           && ! shouldShowFxModule(order[static_cast<size_t>(targetPosition)]))
    {
        targetPosition += step;
    }

    if (targetPosition < 0 || targetPosition > lastMovablePosition)
        return;

    std::swap(order[static_cast<size_t>(position)], order[static_cast<size_t>(targetPosition)]);
    setFxModuleOrder(order);
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::resetFxModuleOrder()
{
    setFxModuleOrder(fxDefaultModuleOrder());
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::applyDelayThrow()
{
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayRate, 2.0f);
    setPlainParameterValue(Parameters::ID::fxDelayTime, 0.31f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.58f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.42f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxReverbSize, 0.28f);
    setPlainParameterValue(Parameters::ID::fxReverbDamping, 0.50f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.14f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.18f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 140.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.08f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);

    selectedFxModule = FxModule::delay;
    resized();
    repaint();
    setFxRackStatusOverride("Delay throw armed | synced 1/8D, high feedback, club safety on");
}

void NateVSTAudioProcessorEditor::applySpaceThrow()
{
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayTime, 0.42f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.46f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.26f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxReverbSize, 0.72f);
    setPlainParameterValue(Parameters::ID::fxReverbDamping, 0.38f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.38f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.36f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 145.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.05f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);

    selectedFxModule = FxModule::reverb;
    resized();
    repaint();
    setFxRackStatusOverride("Space throw armed | wide delay into larger reverb");
}

void NateVSTAudioProcessorEditor::applyPumpDrop()
{
    setPlainParameterValue(Parameters::ID::fxPumpEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxPumpRate, 1.0f);
    setPlainParameterValue(Parameters::ID::fxPumpCurve, 2.0f);
    setPlainParameterValue(Parameters::ID::fxPumpDepth, 0.58f);
    setPlainParameterValue(Parameters::ID::fxPumpShape, 0.68f);
    setPlainParameterValue(Parameters::ID::fxPumpPhase, 0.08f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.08f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 135.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.10f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);
    fxPumpRateBox.setSelectedItemIndex(1, juce::dontSendNotification);
    fxPumpCurveBox.setSelectedItemIndex(2, juce::dontSendNotification);

    selectedFxModule = FxModule::pump;
    resized();
    repaint();
    setFxRackStatusOverride("Pump drop armed | 1/8 ducking with Guard safety");
}

void NateVSTAudioProcessorEditor::clearFxThrows()
{
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.18f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.0f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.0f);
    setPlainParameterValue(Parameters::ID::fxPumpEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxPumpDepth, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.04f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.92f);

    selectedFxModule = FxModule::guard;
    resized();
    repaint();
    setFxRackStatusOverride("Throws cleared | Guard remains on");
}

void NateVSTAudioProcessorEditor::beginMomentaryFxAction(MomentaryFxAction action)
{
    if (action == MomentaryFxAction::none || activeMomentaryFxAction != MomentaryFxAction::none)
        return;

    fxMomentarySnapshot = captureFxMomentarySnapshot();
    activeMomentaryFxAction = action;

    switch (action)
    {
        case MomentaryFxAction::delay:
            applyDelayThrow();
            setFxRackStatusOverride("Holding delay throw | release to restore");
            break;

        case MomentaryFxAction::space:
            applySpaceThrow();
            setFxRackStatusOverride("Holding space throw | release to restore");
            break;

        case MomentaryFxAction::pump:
            applyPumpDrop();
            setFxRackStatusOverride("Holding pump drop | release to restore");
            break;

        case MomentaryFxAction::mute:
            applyMomentaryMuteDrop();
            break;

        case MomentaryFxAction::none:
            break;
    }
}

void NateVSTAudioProcessorEditor::endMomentaryFxAction(MomentaryFxAction action)
{
    if (action == MomentaryFxAction::none || activeMomentaryFxAction != action)
        return;

    restoreFxMomentarySnapshot(fxMomentarySnapshot);
    fxMomentarySnapshot = {};
    activeMomentaryFxAction = MomentaryFxAction::none;
    resized();
    repaint();
    setFxRackStatusOverride("Momentary FX restored");
}

void NateVSTAudioProcessorEditor::applyMomentaryMuteDrop()
{
    setPlainParameterValue(Parameters::ID::outputGain, -24.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.92f);

    selectedFxModule = FxModule::guard;
    resized();
    repaint();
    setFxRackStatusOverride("Holding mute drop | release to restore");
}

void NateVSTAudioProcessorEditor::updateFxPresetBox(bool force)
{
    if (! force && fxPresetBoxModule == selectedFxModule && fxPresetBox.getNumItems() > 0)
        return;

    fxPresetBoxModule = selectedFxModule;
    fxPresetBox.clear(juce::dontSendNotification);

    auto addPreset = [this] (int presetId, const juce::String& name)
    {
        fxPresetBox.addItem(name, presetId);
    };

    switch (selectedFxModule)
    {
        case FxModule::tone:
            addPreset(1, "Garage Low Cut");
            addPreset(2, "Warm Top");
            addPreset(3, "Dark Warehouse");
            break;

        case FxModule::eq:
            addPreset(1, "Bass Tuck");
            addPreset(2, "Stab Presence");
            addPreset(3, "Club Trim");
            break;

        case FxModule::distortion:
            addPreset(1, "Controlled Grit");
            addPreset(2, "Rubber Bass");
            addPreset(3, "Warehouse Clip");
            break;

        case FxModule::bitcrush:
            addPreset(1, "Perc Dirt");
            addPreset(2, "Dub Edge");
            addPreset(3, "Digital Tight");
            break;

        case FxModule::pump:
            addPreset(1, "House Quarter");
            addPreset(2, "UKG Bounce");
            addPreset(3, "Tight Sixteenth");
            break;

        case FxModule::tremolo:
            addPreset(1, "Garage Skank");
            addPreset(2, "Wide Chop");
            addPreset(3, "Subtle Pulse");
            break;

        case FxModule::ring:
            addPreset(1, "Vocal Edge");
            addPreset(2, "Metal Blip");
            addPreset(3, "Dark Sidebands");
            break;

        case FxModule::comb:
            addPreset(1, "Garage Resonator");
            addPreset(2, "Perc Notch");
            addPreset(3, "Pluck Body");
            break;

        case FxModule::phaser:
            addPreset(1, "Light Sweep");
            addPreset(2, "Dub Swirl");
            addPreset(3, "Tech Pulse");
            break;

        case FxModule::flanger:
            addPreset(1, "Short Comb");
            addPreset(2, "Garage Drift");
            addPreset(3, "Metal Jet");
            break;

        case FxModule::chorus:
            addPreset(1, "Wide Organ");
            addPreset(2, "Tight Stab");
            addPreset(3, "Warm Pad");
            break;

        case FxModule::delay:
            addPreset(1, "Garage Throw");
            addPreset(2, "Short Slap");
            addPreset(3, "Dub Repeat");
            break;

        case FxModule::reverb:
            addPreset(1, "Short Room");
            addPreset(2, "Plate Wash");
            addPreset(3, "Dark Space");
            break;

        case FxModule::width:
            addPreset(1, "Mono-Safe Width");
            addPreset(2, "Wide Stab");
            addPreset(3, "Club Narrow");
            break;

        case FxModule::guard:
            addPreset(1, "Club Safety");
            addPreset(2, "Hot Push");
            addPreset(3, "Clean Ceiling");
            break;
    }

    fxPresetBox.setTextWhenNothingSelected("Module Preset");
    fxPresetBox.setSelectedId(0, juce::dontSendNotification);
    fxApplyPresetButton.setEnabled(fxPresetBox.getNumItems() > 0);
}

void NateVSTAudioProcessorEditor::applySelectedFxPreset()
{
    const auto presetId = fxPresetBox.getSelectedId();
    if (presetId <= 0)
        return;

    applyFxModulePreset(selectedFxModule, presetId);
}

void NateVSTAudioProcessorEditor::applyFxModulePreset(FxModule module, int presetId)
{
    const auto presetName = fxPresetBox.getText().isNotEmpty() ? fxPresetBox.getText() : juce::String("FX preset");

    auto set = [this] (const juce::String& parameterID, float value)
    {
        setPlainParameterValue(parameterID, value);
    };

    selectedFxModule = module;
    set(fxEnabledParameterID(module), 1.0f);

    switch (module)
    {
        case FxModule::tone:
            if (presetId == 1)
            {
                set(Parameters::ID::fxToneTilt, 0.08f);
                set(Parameters::ID::fxToneLowCut, 92.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxToneTilt, 0.22f);
                set(Parameters::ID::fxToneLowCut, 48.0f);
            }
            else
            {
                set(Parameters::ID::fxToneTilt, -0.30f);
                set(Parameters::ID::fxToneLowCut, 70.0f);
            }
            break;

        case FxModule::eq:
            if (presetId == 1)
            {
                set(Parameters::ID::fxEqLowGain, -2.6f);
                set(Parameters::ID::fxEqMidGain, 0.4f);
                set(Parameters::ID::fxEqHighGain, 1.2f);
                set(Parameters::ID::fxEqTrim, -0.8f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxEqLowGain, -1.2f);
                set(Parameters::ID::fxEqMidGain, 1.8f);
                set(Parameters::ID::fxEqHighGain, 2.4f);
                set(Parameters::ID::fxEqTrim, -1.2f);
            }
            else
            {
                set(Parameters::ID::fxEqLowGain, -0.8f);
                set(Parameters::ID::fxEqMidGain, -1.0f);
                set(Parameters::ID::fxEqHighGain, 0.8f);
                set(Parameters::ID::fxEqTrim, -1.8f);
            }
            break;

        case FxModule::distortion:
            set(Parameters::ID::fxDistortionAmount, presetId == 1 ? 0.22f : (presetId == 2 ? 0.38f : 0.56f));
            break;

        case FxModule::bitcrush:
            if (presetId == 1)
            {
                set(Parameters::ID::fxBitcrushBits, 10.0f);
                set(Parameters::ID::fxBitcrushDownsample, 2.0f);
                set(Parameters::ID::fxBitcrushMix, 0.18f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxBitcrushBits, 8.0f);
                set(Parameters::ID::fxBitcrushDownsample, 4.0f);
                set(Parameters::ID::fxBitcrushMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxBitcrushBits, 12.0f);
                set(Parameters::ID::fxBitcrushDownsample, 3.0f);
                set(Parameters::ID::fxBitcrushMix, 0.14f);
            }
            break;

        case FxModule::pump:
            if (presetId == 1)
            {
                set(Parameters::ID::fxPumpRate, 0.0f);
                set(Parameters::ID::fxPumpCurve, 0.0f);
                set(Parameters::ID::fxPumpDepth, 0.44f);
                set(Parameters::ID::fxPumpShape, 0.56f);
                set(Parameters::ID::fxPumpPhase, 0.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxPumpRate, 1.0f);
                set(Parameters::ID::fxPumpCurve, 2.0f);
                set(Parameters::ID::fxPumpDepth, 0.34f);
                set(Parameters::ID::fxPumpShape, 0.64f);
                set(Parameters::ID::fxPumpPhase, 0.08f);
            }
            else
            {
                set(Parameters::ID::fxPumpRate, 3.0f);
                set(Parameters::ID::fxPumpCurve, 4.0f);
                set(Parameters::ID::fxPumpDepth, 0.26f);
                set(Parameters::ID::fxPumpShape, 0.48f);
                set(Parameters::ID::fxPumpPhase, 0.0f);
            }
            break;

        case FxModule::tremolo:
            if (presetId == 1)
            {
                set(Parameters::ID::fxTremoloRate, 2.0f);
                set(Parameters::ID::fxTremoloDepth, 0.30f);
                set(Parameters::ID::fxTremoloPan, 0.46f);
                set(Parameters::ID::fxTremoloShape, 0.64f);
                set(Parameters::ID::fxTremoloPhase, 0.18f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxTremoloRate, 3.0f);
                set(Parameters::ID::fxTremoloDepth, 0.44f);
                set(Parameters::ID::fxTremoloPan, 0.78f);
                set(Parameters::ID::fxTremoloShape, 0.70f);
                set(Parameters::ID::fxTremoloPhase, 0.28f);
            }
            else
            {
                set(Parameters::ID::fxTremoloRate, 1.0f);
                set(Parameters::ID::fxTremoloDepth, 0.18f);
                set(Parameters::ID::fxTremoloPan, 0.20f);
                set(Parameters::ID::fxTremoloShape, 0.42f);
                set(Parameters::ID::fxTremoloPhase, 0.0f);
            }
            break;

        case FxModule::ring:
            if (presetId == 1)
            {
                set(Parameters::ID::fxRingFrequency, 92.0f);
                set(Parameters::ID::fxRingDepth, 0.18f);
                set(Parameters::ID::fxRingMix, 0.08f);
                set(Parameters::ID::fxRingBias, 0.60f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxRingFrequency, 420.0f);
                set(Parameters::ID::fxRingDepth, 0.34f);
                set(Parameters::ID::fxRingMix, 0.14f);
                set(Parameters::ID::fxRingBias, 0.42f);
            }
            else
            {
                set(Parameters::ID::fxRingFrequency, 58.0f);
                set(Parameters::ID::fxRingDepth, 0.26f);
                set(Parameters::ID::fxRingMix, 0.10f);
                set(Parameters::ID::fxRingBias, 0.34f);
            }
            break;

        case FxModule::comb:
            if (presetId == 1)
            {
                set(Parameters::ID::fxCombFrequency, 280.0f);
                set(Parameters::ID::fxCombFeedback, 0.20f);
                set(Parameters::ID::fxCombDamping, 0.56f);
                set(Parameters::ID::fxCombMix, 0.08f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxCombFrequency, 620.0f);
                set(Parameters::ID::fxCombFeedback, -0.24f);
                set(Parameters::ID::fxCombDamping, 0.42f);
                set(Parameters::ID::fxCombMix, 0.10f);
            }
            else
            {
                set(Parameters::ID::fxCombFrequency, 170.0f);
                set(Parameters::ID::fxCombFeedback, 0.32f);
                set(Parameters::ID::fxCombDamping, 0.48f);
                set(Parameters::ID::fxCombMix, 0.12f);
            }
            break;

        case FxModule::phaser:
            if (presetId == 1)
            {
                set(Parameters::ID::fxPhaserRate, 0.32f);
                set(Parameters::ID::fxPhaserDepth, 0.34f);
                set(Parameters::ID::fxPhaserMix, 0.16f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxPhaserRate, 0.18f);
                set(Parameters::ID::fxPhaserDepth, 0.56f);
                set(Parameters::ID::fxPhaserMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxPhaserRate, 0.78f);
                set(Parameters::ID::fxPhaserDepth, 0.28f);
                set(Parameters::ID::fxPhaserMix, 0.14f);
            }
            break;

        case FxModule::flanger:
            if (presetId == 1)
            {
                set(Parameters::ID::fxFlangerRate, 0.16f);
                set(Parameters::ID::fxFlangerDepth, 0.28f);
                set(Parameters::ID::fxFlangerFeedback, 0.24f);
                set(Parameters::ID::fxFlangerMix, 0.10f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxFlangerRate, 0.24f);
                set(Parameters::ID::fxFlangerDepth, 0.40f);
                set(Parameters::ID::fxFlangerFeedback, 0.12f);
                set(Parameters::ID::fxFlangerMix, 0.16f);
            }
            else
            {
                set(Parameters::ID::fxFlangerRate, 0.46f);
                set(Parameters::ID::fxFlangerDepth, 0.52f);
                set(Parameters::ID::fxFlangerFeedback, 0.42f);
                set(Parameters::ID::fxFlangerMix, 0.18f);
            }
            break;

        case FxModule::chorus:
            if (presetId == 1)
            {
                set(Parameters::ID::fxChorusRate, 0.28f);
                set(Parameters::ID::fxChorusDepth, 0.42f);
                set(Parameters::ID::fxChorusMix, 0.24f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxChorusRate, 0.42f);
                set(Parameters::ID::fxChorusDepth, 0.24f);
                set(Parameters::ID::fxChorusMix, 0.14f);
            }
            else
            {
                set(Parameters::ID::fxChorusRate, 0.18f);
                set(Parameters::ID::fxChorusDepth, 0.54f);
                set(Parameters::ID::fxChorusMix, 0.28f);
            }
            break;

        case FxModule::delay:
            if (presetId == 1)
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 2.0f);
                set(Parameters::ID::fxDelayTime, 0.31f);
                set(Parameters::ID::fxDelayFeedback, 0.54f);
                set(Parameters::ID::fxDelayMix, 0.34f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 4.0f);
                set(Parameters::ID::fxDelayTime, 0.11f);
                set(Parameters::ID::fxDelayFeedback, 0.20f);
                set(Parameters::ID::fxDelayMix, 0.16f);
            }
            else
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 0.0f);
                set(Parameters::ID::fxDelayTime, 0.42f);
                set(Parameters::ID::fxDelayFeedback, 0.62f);
                set(Parameters::ID::fxDelayMix, 0.28f);
            }
            break;

        case FxModule::reverb:
            if (presetId == 1)
            {
                set(Parameters::ID::fxReverbSize, 0.24f);
                set(Parameters::ID::fxReverbDamping, 0.62f);
                set(Parameters::ID::fxReverbMix, 0.12f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxReverbSize, 0.54f);
                set(Parameters::ID::fxReverbDamping, 0.42f);
                set(Parameters::ID::fxReverbMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxReverbSize, 0.74f);
                set(Parameters::ID::fxReverbDamping, 0.28f);
                set(Parameters::ID::fxReverbMix, 0.30f);
            }
            break;

        case FxModule::width:
            if (presetId == 1)
            {
                set(Parameters::ID::fxWidthAmount, 1.12f);
                set(Parameters::ID::fxWidthMonoCutoff, 145.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxWidthAmount, 1.42f);
                set(Parameters::ID::fxWidthMonoCutoff, 135.0f);
            }
            else
            {
                set(Parameters::ID::fxWidthAmount, 0.92f);
                set(Parameters::ID::fxWidthMonoCutoff, 175.0f);
            }
            break;

        case FxModule::guard:
            if (presetId == 1)
            {
                set(Parameters::ID::fxGuardPush, 0.06f);
                set(Parameters::ID::fxGuardCeiling, 0.90f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxGuardPush, 0.16f);
                set(Parameters::ID::fxGuardCeiling, 0.88f);
            }
            else
            {
                set(Parameters::ID::fxGuardPush, 0.02f);
                set(Parameters::ID::fxGuardCeiling, 0.94f);
            }
            break;
    }

    updateFxRackControls();
    resized();
    repaint();
    setFxRackStatusOverride(presetName + " loaded | " + fxModuleName(module));
}

NateVSTAudioProcessorEditor::FxMomentarySnapshot NateVSTAudioProcessorEditor::captureFxMomentarySnapshot() const
{
    FxMomentarySnapshot snapshot;
    snapshot.selectedModule = selectedFxModule;
    snapshot.valid = true;

    for (size_t index = 0; index < momentaryFxParameterIDs.size(); ++index)
        snapshot.values[index] = readPlainParameterValue(momentaryFxParameterIDs[index], 0.0f);

    return snapshot;
}

void NateVSTAudioProcessorEditor::restoreFxMomentarySnapshot(const FxMomentarySnapshot& snapshot)
{
    if (! snapshot.valid)
        return;

    for (size_t index = 0; index < momentaryFxParameterIDs.size(); ++index)
        setPlainParameterValue(momentaryFxParameterIDs[index], snapshot.values[index]);

    fxDelayRateBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f)), juce::dontSendNotification);
    fxPumpRateBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpRate, 0.0f)), juce::dontSendNotification);
    fxPumpCurveBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)), juce::dontSendNotification);
    selectedFxModule = snapshot.selectedModule;
}

void NateVSTAudioProcessorEditor::setFxRackStatusOverride(const juce::String& message)
{
    fxRackStatusOverride = message;
    fxRackStatusOverrideUntilMs = juce::Time::getMillisecondCounterHiRes() + fxRackStatusOverrideMs;
    fxRackStatusLabel.setText(fxRackStatusOverride, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateFxRackControls()
{
    if (! shouldShowFxModule(selectedFxModule))
        selectedFxModule = FxModule::guard;

    const auto moduleOrder = fxModuleOrder();
    for (const auto module : moduleOrder)
    {
        auto& button = fxSlotButton(module);
        const auto isEnabled = isFxModuleEnabled(module);
        button.setState(fxModuleName(module),
                        fxModuleSummary(module),
                        fxOrderPosition(module),
                        isEnabled,
                        module == selectedFxModule,
                        module == FxModule::guard);
    }

    if (fxRackStatusOverride.isNotEmpty()
        && juce::Time::getMillisecondCounterHiRes() < fxRackStatusOverrideUntilMs)
    {
        fxRackStatusLabel.setText(fxRackStatusOverride, juce::dontSendNotification);
    }
    else
    {
        fxRackStatusOverride.clear();
        fxRackStatusLabel.setText("#" + juce::String(fxOrderPosition(selectedFxModule)).paddedLeft('0', 2)
                                      + " " + fxModuleName(selectedFxModule)
                                      + " | " + fxModuleSummary(selectedFxModule),
                                  juce::dontSendNotification);
    }
    const auto selectedPosition = fxOrderPosition(selectedFxModule);
    const auto canMoveSelected = selectedFxModule != FxModule::guard;
    const auto hasVisibleMoveTarget = [&] (int direction)
    {
        if (! canMoveSelected)
            return false;

        const auto position = selectedPosition - 1;
        const auto lastMovablePosition = static_cast<int>(moduleOrder.size()) - 2;
        const auto step = direction < 0 ? -1 : 1;
        auto targetPosition = position + step;

        while (targetPosition >= 0 && targetPosition <= lastMovablePosition)
        {
            if (shouldShowFxModule(moduleOrder[static_cast<size_t>(targetPosition)]))
                return true;

            targetPosition += step;
        }

        return false;
    };

    updateFxPresetBox();
    fxMoveUpButton.setEnabled(hasVisibleMoveTarget(-1));
    fxMoveDownButton.setEnabled(hasVisibleMoveTarget(1));
    fxRemoveButton.setEnabled(selectedFxModule != FxModule::guard);
}

std::array<NateVSTAudioProcessorEditor::FxModule, 15> NateVSTAudioProcessorEditor::fxDefaultModuleOrder() const
{
    return {
        FxModule::tone,
        FxModule::eq,
        FxModule::distortion,
        FxModule::bitcrush,
        FxModule::pump,
        FxModule::tremolo,
        FxModule::ring,
        FxModule::comb,
        FxModule::phaser,
        FxModule::flanger,
        FxModule::chorus,
        FxModule::delay,
        FxModule::reverb,
        FxModule::width,
        FxModule::guard
    };
}

std::array<NateVSTAudioProcessorEditor::FxModule, 15> NateVSTAudioProcessorEditor::fxModuleOrder() const
{
    auto order = fxDefaultModuleOrder();
    std::array<bool, 15> used {};
    auto writeIndex = size_t { 0 };

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::fxOrder.size(); ++slotIndex)
    {
        const auto fallback = static_cast<float>(slotIndex);
        auto moduleIndex = static_cast<int>(slotIndex);
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::fxOrder[slotIndex]))
            moduleIndex = static_cast<int>(std::round(value->load()));
        else
            moduleIndex = static_cast<int>(fallback);

        moduleIndex = juce::jlimit(0, 14, moduleIndex);
        const auto module = fxModuleFromIndex(moduleIndex);
        if (module == FxModule::guard || used[static_cast<size_t>(moduleIndex)])
            continue;

        order[writeIndex++] = module;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    for (const auto module : fxDefaultModuleOrder())
    {
        const auto moduleIndex = fxModuleIndex(module);
        if (module == FxModule::guard || used[static_cast<size_t>(moduleIndex)])
            continue;

        order[writeIndex++] = module;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    while (writeIndex < order.size() - 1)
        order[writeIndex++] = FxModule::guard;

    order.back() = FxModule::guard;
    return order;
}

void NateVSTAudioProcessorEditor::setFxModuleOrder(const std::array<FxModule, 15>& order)
{
    auto normalised = order;
    normalised.back() = FxModule::guard;

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::fxOrder.size(); ++slotIndex)
        setPlainParameterValue(Parameters::ID::fxOrder[slotIndex], static_cast<float>(fxModuleIndex(normalised[slotIndex])));
}

int NateVSTAudioProcessorEditor::fxOrderPosition(FxModule module) const
{
    const auto order = fxModuleOrder();
    for (size_t index = 0; index < order.size(); ++index)
        if (order[index] == module)
            return static_cast<int>(index + 1);

    return 0;
}

int NateVSTAudioProcessorEditor::fxModuleIndex(FxModule module) const
{
    return static_cast<int>(module);
}

NateVSTAudioProcessorEditor::FxModule NateVSTAudioProcessorEditor::fxModuleFromIndex(int index) const
{
    switch (juce::jlimit(0, 14, index))
    {
        case 0: return FxModule::tone;
        case 1: return FxModule::eq;
        case 2: return FxModule::distortion;
        case 3: return FxModule::bitcrush;
        case 4: return FxModule::pump;
        case 5: return FxModule::tremolo;
        case 6: return FxModule::ring;
        case 7: return FxModule::comb;
        case 8: return FxModule::phaser;
        case 9: return FxModule::flanger;
        case 10: return FxModule::chorus;
        case 11: return FxModule::delay;
        case 12: return FxModule::reverb;
        case 13: return FxModule::width;
        case 14:
        default: return FxModule::guard;
    }
}

bool NateVSTAudioProcessorEditor::isFxModuleEnabled(FxModule module) const
{
    if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(fxEnabledParameterID(module)))
        return value->load() >= 0.5f;

    return false;
}

bool NateVSTAudioProcessorEditor::shouldShowFxModule(FxModule module) const
{
    return module == FxModule::guard || module == selectedFxModule || isFxModuleEnabled(module);
}

juce::String NateVSTAudioProcessorEditor::fxEnabledParameterID(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return Parameters::ID::fxToneEnabled;
        case FxModule::eq: return Parameters::ID::fxEqEnabled;
        case FxModule::distortion: return Parameters::ID::fxDistortionEnabled;
        case FxModule::bitcrush: return Parameters::ID::fxBitcrushEnabled;
        case FxModule::pump: return Parameters::ID::fxPumpEnabled;
        case FxModule::tremolo: return Parameters::ID::fxTremoloEnabled;
        case FxModule::ring: return Parameters::ID::fxRingEnabled;
        case FxModule::comb: return Parameters::ID::fxCombEnabled;
        case FxModule::phaser: return Parameters::ID::fxPhaserEnabled;
        case FxModule::flanger: return Parameters::ID::fxFlangerEnabled;
        case FxModule::chorus: return Parameters::ID::fxChorusEnabled;
        case FxModule::delay: return Parameters::ID::fxDelayEnabled;
        case FxModule::reverb: return Parameters::ID::fxReverbEnabled;
        case FxModule::width: return Parameters::ID::fxWidthEnabled;
        case FxModule::guard: return Parameters::ID::fxGuardEnabled;
    }

    return {};
}

juce::String NateVSTAudioProcessorEditor::fxModuleName(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return "Tone";
        case FxModule::eq: return "EQ";
        case FxModule::distortion: return "Drive";
        case FxModule::bitcrush: return "Crush";
        case FxModule::pump: return "Pump";
        case FxModule::tremolo: return "Tremolo";
        case FxModule::ring: return "Ring Mod";
        case FxModule::comb: return "Comb";
        case FxModule::phaser: return "Phaser";
        case FxModule::flanger: return "Flanger";
        case FxModule::chorus: return "Chorus";
        case FxModule::delay: return "Delay";
        case FxModule::reverb: return "Reverb";
        case FxModule::width: return "Width";
        case FxModule::guard: return "Guard";
    }

    return {};
}

juce::String NateVSTAudioProcessorEditor::fxModuleSummary(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return "tilt and low cut";
        case FxModule::eq: return "low mid high trim";
        case FxModule::distortion: return "saturation amount";
        case FxModule::bitcrush: return "bits downsample mix";
        case FxModule::pump:
        {
            const auto choices = Parameters::pumpCurveChoices();
            const auto curveIndex = juce::jlimit(0,
                                                 choices.size() - 1,
                                                 juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)));
            return "sync " + choices[curveIndex] + " duck";
        }
        case FxModule::tremolo: return "sync trem pan";
        case FxModule::ring: return "metallic sidebands";
        case FxModule::comb: return "tuned resonance";
        case FxModule::phaser: return "rate depth mix";
        case FxModule::flanger: return "short delay feedback";
        case FxModule::chorus: return "rate depth mix";
        case FxModule::delay:
        {
            if (readPlainParameterValue(Parameters::ID::fxDelaySync, 0.0f) >= 0.5f)
            {
                const auto choices = Parameters::delayRateChoices();
                const auto rateIndex = juce::jlimit(0,
                                                     choices.size() - 1,
                                                     juce::roundToInt(readPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f)));
                return "sync " + choices[rateIndex] + " feedback mix";
            }

            return "time feedback mix";
        }
        case FxModule::reverb: return "size damping mix";
        case FxModule::width: return "mono bass width";
        case FxModule::guard: return "push and ceiling";
    }

    return {};
}

UI::FxRackRow& NateVSTAudioProcessorEditor::fxSlotButton(FxModule module)
{
    switch (module)
    {
        case FxModule::tone: return fxToneSlotButton;
        case FxModule::eq: return fxEqSlotButton;
        case FxModule::distortion: return fxDistortionSlotButton;
        case FxModule::bitcrush: return fxBitcrushSlotButton;
        case FxModule::pump: return fxPumpSlotButton;
        case FxModule::tremolo: return fxTremoloSlotButton;
        case FxModule::ring: return fxRingSlotButton;
        case FxModule::comb: return fxCombSlotButton;
        case FxModule::phaser: return fxPhaserSlotButton;
        case FxModule::flanger: return fxFlangerSlotButton;
        case FxModule::chorus: return fxChorusSlotButton;
        case FxModule::delay: return fxDelaySlotButton;
        case FxModule::reverb: return fxReverbSlotButton;
        case FxModule::width: return fxWidthSlotButton;
        case FxModule::guard: return fxGuardSlotButton;
    }

    return fxGuardSlotButton;
}

float NateVSTAudioProcessorEditor::readPlainParameterValue(const juce::String& parameterID, float fallback) const
{
    if (const auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

void NateVSTAudioProcessorEditor::setPlainParameterValue(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}

int NateVSTAudioProcessorEditor::selectedMacroAssignmentSourceIndex() const
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto selectedSource = modMacroAssignSourceBox.getSelectedId() - 1;
    return juce::jlimit(firstMacroModSourceIndex,
                        juce::jmin(lastMacroModSourceIndex, sourceChoices.size() - 1),
                        selectedSource);
}

int NateVSTAudioProcessorEditor::selectedMacroAssignmentDestinationIndex() const
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = modMacroAssignDestinationBox.getSelectedId() - 1;
    return juce::jlimit(1, destinationChoices.size() - 1, selectedDestination);
}

void NateVSTAudioProcessorEditor::setActivePanel(Panel panel)
{
    activePanel = panel;
    updatePanelVisibility();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::updatePanelVisibility()
{
    hidePanelComponents();
    updateTabButtons();
}

void NateVSTAudioProcessorEditor::updateTabButtons()
{
    homeTabButton.setToggleState(activePanel == Panel::home, juce::dontSendNotification);
    synthTabButton.setToggleState(activePanel == Panel::synth, juce::dontSendNotification);
    labTabButton.setToggleState(activePanel == Panel::lab, juce::dontSendNotification);
    modTabButton.setToggleState(activePanel == Panel::mod, juce::dontSendNotification);
    sampleTabButton.setToggleState(activePanel == Panel::sample, juce::dontSendNotification);
    sequencerTabButton.setToggleState(activePanel == Panel::sequencer, juce::dontSendNotification);
    effectsTabButton.setToggleState(activePanel == Panel::effects, juce::dontSendNotification);
    libraryTabButton.setToggleState(activePanel == Panel::library, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::hidePanelComponents()
{
    auto hide = [] (std::initializer_list<juce::Component*> components)
    {
        for (auto* component : components)
            component->setVisible(false);
    };

    hide({
        &homeSectionLabel, &homeEngineLabel, &homeShapeLabel, &homeLabLabel, &homeLibraryLabel,
        &synthSectionLabel, &synthSourceLabel, &synthVoiceLabel, &synthFilterLabel, &synthAmpLabel,
        &randomSectionLabel, &modSectionLabel, &modSourceLabel, &modMacroLabel, &modLfoLabel, &modEnvelopeLabel, &modMatrixLabel,
        &modMatrixStatusLabel, &modInspectorLabel, &modInspectorStatusLabel, &modMatrixSourceHeader, &modMatrixDestinationHeader, &modMatrixAmountHeader,
        &modMatrixSourceHeaderB, &modMatrixDestinationHeaderB, &modMatrixAmountHeaderB, &modMacroAssignLabel, &modMacroAssignStatusLabel,
        &sampleSectionLabel, &sampleSourceLabel, &sampleChopLabel, &sampleShapeLabel, &sequencerSectionLabel,
        &hostSyncStatusLabel, &futureSectionLabel, &librarySectionLabel, &sampleNameLabel, &presetStatusLabel, &presetBrowserHeaderLabel, &randomStatusLabel, &performanceStatusLabel,
        &waveformBox, &osc2WaveBox, &filterModeBox, &filterCharacterBox, &filterSlopeBox, &recipeBox, &randomScopeBox, &sequencerRateBox, &sequencerGrooveBox, &sequencerScaleBox, &sequencerChordBox, &sequencerVoicingBox, &sequencerPatternBox, &sequencerGrooveTransformBox, &sampleModeBox, &sampleSliceStyleBox, &sampleStutterRateBox, &presetBox, &presetCategoryBox,
        &presetFilterBox, &presetTagBox, &presetSortBox, &presetRatingBox, &presetPackBox, &presetKeyBox, &presetBpmBox, &fxAddBox, &fxPresetBox, &fxDelayRateBox, &fxPumpRateBox, &fxPumpCurveBox, &fxTremoloRateBox, &modInspectorDestinationBox, &modInspectorSourceBox, &modMacroAssignSourceBox, &modMacroAssignDestinationBox, &lfo1ShapeBox, &lfo1SyncRateBox, &lfoCurvePresetBox,
        &monoButton, &sampleEnabledButton, &sampleReverseButton, &sampleStutterEnabledButton, &sequencerEnabledButton, &sequencerChordMemoryButton,
        &fxDistortionEnabledButton, &fxBitcrushEnabledButton, &fxPumpEnabledButton, &fxTremoloEnabledButton, &fxRingEnabledButton, &fxCombEnabledButton, &fxChorusEnabledButton, &fxDelayEnabledButton, &fxDelaySyncButton, &fxReverbEnabledButton, &fxWidthEnabledButton,
        &fxToneEnabledButton, &fxEqEnabledButton, &fxPhaserEnabledButton, &fxGuardEnabledButton,
        &fxFlangerEnabledButton,
        &randomLockPitchButton, &randomLockEnvelopeButton, &randomLockFilterButton, &randomLockSourceButton,
        &randomLockSampleButton, &randomLockFxButton, &randomLockOutputButton, &randomLockSequencerButton,
        &lfo1SyncButton, &lfo1RetriggerButton,
        &generateButton, &mutateButton, &variationButton, &wildMutateButton, &undoRandomButton, &redoRandomButton,
        &recallSnapshotAButton, &captureSnapshotAButton, &recallSnapshotBButton, &captureSnapshotBButton,
        &loadSampleButton, &clearSampleButton,
        &randomCutButton, &ukgChopButton, &randomSequencerButton, &mutateSequencerButton, &undoSequencerButton, &clearSequencerButton,
        &bassPatternButton, &stabPatternButton, &ukgPatternButton, &applyPatternButton, &copySequencerButton,
        &rotateSequencerLeftButton, &rotateSequencerRightButton, &exportSequencerMidiButton, &applyGrooveTransformButton,
        &sineWaveButton, &sawWaveButton, &squareWaveButton, &triangleWaveButton,
        &osc2SineWaveButton, &osc2SawWaveButton, &osc2SquareWaveButton, &osc2TriangleWaveButton,
        &lowpassFilterButton, &bandpassFilterButton, &highpassFilterButton,
        &rateEighthButton, &rateSixteenthButton, &rateThirtySecondButton,
        &previousPresetButton, &nextPresetButton,
        &savePresetButton, &loadPresetButton, &auditionPresetButton, &refreshPresetsButton, &favoritePresetButton,
        &fxMoveUpButton, &fxMoveDownButton, &fxResetOrderButton,
        &fxThrowDelayButton, &fxThrowSpaceButton, &fxThrowPumpButton, &fxThrowDryButton,
        &fxHoldDelayButton, &fxHoldSpaceButton, &fxHoldPumpButton, &fxMuteDropButton,
        &fxApplyPresetButton, &modInspectorAddButton, &modInspectorClearButton, &modMacroAssignAddButton, &modMacroAssignReplaceButton, &modMacroAssignClearButton,
        &fxRemoveButton, &fxToneSlotButton, &fxEqSlotButton, &fxDistortionSlotButton, &fxBitcrushSlotButton, &fxPumpSlotButton, &fxTremoloSlotButton, &fxRingSlotButton, &fxCombSlotButton, &fxPhaserSlotButton, &fxFlangerSlotButton, &fxChorusSlotButton,
        &fxDelaySlotButton, &fxReverbSlotButton, &fxWidthSlotButton, &fxGuardSlotButton,
        &presetNameEditor, &presetSearchEditor, &presetAuthorEditor, &presetBrowserList, &fxRackStatusLabel,
        &lowEndAssistant, &performanceXYPad, &sampleWaveformDisplay, &lfoCurveDisplay, &pumpCurveDisplay, &sequencerGrid
    });

    for (auto& slider : lfoCurveSliders)
        slider.setVisible(false);

    for (auto& button : sampleSliceButtons)
        button.setVisible(false);

    for (auto& label : modSourceRows)
        label.setVisible(false);

    for (size_t index = 0; index < modSlotRows.size(); ++index)
    {
        modMatrixRows[index].setVisible(false);
        modSlotRows[index].setVisible(false);
        modSourceBoxes[index].setVisible(false);
        modDestinationBoxes[index].setVisible(false);
        modSlotEnabledButtons[index].setVisible(false);
        modSlotDeleteButtons[index].setVisible(false);
        setSliderVisible(modAmountSliders[index], modAmountLabels[index], false);
    }

    setSliderVisible(octaveSlider, octaveLabel, false);
    setSliderVisible(tuneSlider, tuneLabel, false);
    setSliderVisible(osc1LevelSlider, osc1LevelLabel, false);
    setSliderVisible(osc2OctaveSlider, osc2OctaveLabel, false);
    setSliderVisible(osc2TuneSlider, osc2TuneLabel, false);
    setSliderVisible(osc2LevelSlider, osc2LevelLabel, false);
    setSliderVisible(subLevelSlider, subLevelLabel, false);
    setSliderVisible(noiseLevelSlider, noiseLevelLabel, false);
    setSliderVisible(oscWarpSlider, oscWarpLabel, false);
    setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, false);
    setSliderVisible(unisonDetuneSlider, unisonDetuneLabel, false);
    setSliderVisible(unisonBlendSlider, unisonBlendLabel, false);
    setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, false);
    setSliderVisible(glideSlider, glideLabel, false);
    setSliderVisible(macroToneSlider, macroToneLabel, false);
    setSliderVisible(macroDirtSlider, macroDirtLabel, false);
    setSliderVisible(macroMotionSlider, macroMotionLabel, false);
    setSliderVisible(macroSpaceSlider, macroSpaceLabel, false);
    setSliderVisible(macroWeightSlider, macroWeightLabel, false);
    setSliderVisible(macroBounceSlider, macroBounceLabel, false);
    setSliderVisible(macroWarpSlider, macroWarpLabel, false);
    setSliderVisible(macroThrowSlider, macroThrowLabel, false);
    modMacroAssignAmountSlider.setVisible(false);
    setSliderVisible(lfo1RateSlider, lfo1RateLabel, false);
    setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, false);
    setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, false);
    setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, false);
    setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, false);
    setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, false);
    setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, false);
    setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, false);
    setSliderVisible(attackSlider, attackLabel, false);
    setSliderVisible(decaySlider, decayLabel, false);
    setSliderVisible(sustainSlider, sustainLabel, false);
    setSliderVisible(releaseSlider, releaseLabel, false);
    setSliderVisible(cutoffSlider, cutoffLabel, false);
    setSliderVisible(resonanceSlider, resonanceLabel, false);
    setSliderVisible(filterEnvSlider, filterEnvLabel, false);
    setSliderVisible(driveSlider, driveLabel, false);
    setSliderVisible(outputSlider, outputLabel, false);
    setSliderVisible(randomAmountSlider, randomAmountLabel, false);
    setSliderVisible(randomChaosSlider, randomChaosLabel, false);
    setSliderVisible(brightnessSlider, brightnessLabel, false);
    setSliderVisible(driveBiasSlider, driveBiasLabel, false);
    setSliderVisible(motionBiasSlider, motionBiasLabel, false);
    setSliderVisible(sampleStartSlider, sampleStartLabel, false);
    setSliderVisible(sampleEndSlider, sampleEndLabel, false);
    setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, false);
    setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, false);
    setSliderVisible(sampleGainSlider, sampleGainLabel, false);
    setSliderVisible(sampleMixSlider, sampleMixLabel, false);
    setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, false);
    setSliderVisible(sequencerRootSlider, sequencerRootLabel, false);
    setSliderVisible(sequencerGateSlider, sequencerGateLabel, false);
    setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, false);
    setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, false);
    setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, false);
    setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, false);
    setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, false);
    setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, false);
    setSliderVisible(fxDistortionAmountSlider, fxDistortionAmountLabel, false);
    setSliderVisible(fxBitcrushBitsSlider, fxBitcrushBitsLabel, false);
    setSliderVisible(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, false);
    setSliderVisible(fxBitcrushMixSlider, fxBitcrushMixLabel, false);
    setSliderVisible(fxPumpDepthSlider, fxPumpDepthLabel, false);
    setSliderVisible(fxPumpShapeSlider, fxPumpShapeLabel, false);
    setSliderVisible(fxPumpPhaseSlider, fxPumpPhaseLabel, false);
    setSliderVisible(fxTremoloDepthSlider, fxTremoloDepthLabel, false);
    setSliderVisible(fxTremoloPanSlider, fxTremoloPanLabel, false);
    setSliderVisible(fxTremoloShapeSlider, fxTremoloShapeLabel, false);
    setSliderVisible(fxTremoloPhaseSlider, fxTremoloPhaseLabel, false);
    setSliderVisible(fxRingFrequencySlider, fxRingFrequencyLabel, false);
    setSliderVisible(fxRingDepthSlider, fxRingDepthLabel, false);
    setSliderVisible(fxRingMixSlider, fxRingMixLabel, false);
    setSliderVisible(fxRingBiasSlider, fxRingBiasLabel, false);
    setSliderVisible(fxCombFrequencySlider, fxCombFrequencyLabel, false);
    setSliderVisible(fxCombFeedbackSlider, fxCombFeedbackLabel, false);
    setSliderVisible(fxCombDampingSlider, fxCombDampingLabel, false);
    setSliderVisible(fxCombMixSlider, fxCombMixLabel, false);
    setSliderVisible(fxChorusRateSlider, fxChorusRateLabel, false);
    setSliderVisible(fxChorusDepthSlider, fxChorusDepthLabel, false);
    setSliderVisible(fxChorusMixSlider, fxChorusMixLabel, false);
    setSliderVisible(fxDelayTimeSlider, fxDelayTimeLabel, false);
    setSliderVisible(fxDelayFeedbackSlider, fxDelayFeedbackLabel, false);
    setSliderVisible(fxDelayMixSlider, fxDelayMixLabel, false);
    setSliderVisible(fxReverbSizeSlider, fxReverbSizeLabel, false);
    setSliderVisible(fxReverbDampingSlider, fxReverbDampingLabel, false);
    setSliderVisible(fxReverbMixSlider, fxReverbMixLabel, false);
    setSliderVisible(fxWidthAmountSlider, fxWidthAmountLabel, false);
    setSliderVisible(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, false);
    setSliderVisible(fxToneTiltSlider, fxToneTiltLabel, false);
    setSliderVisible(fxToneLowCutSlider, fxToneLowCutLabel, false);
    setSliderVisible(fxEqLowGainSlider, fxEqLowGainLabel, false);
    setSliderVisible(fxEqMidGainSlider, fxEqMidGainLabel, false);
    setSliderVisible(fxEqHighGainSlider, fxEqHighGainLabel, false);
    setSliderVisible(fxEqTrimSlider, fxEqTrimLabel, false);
    setSliderVisible(fxPhaserRateSlider, fxPhaserRateLabel, false);
    setSliderVisible(fxPhaserDepthSlider, fxPhaserDepthLabel, false);
    setSliderVisible(fxPhaserMixSlider, fxPhaserMixLabel, false);
    setSliderVisible(fxFlangerRateSlider, fxFlangerRateLabel, false);
    setSliderVisible(fxFlangerDepthSlider, fxFlangerDepthLabel, false);
    setSliderVisible(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, false);
    setSliderVisible(fxFlangerMixSlider, fxFlangerMixLabel, false);
    setSliderVisible(fxGuardPushSlider, fxGuardPushLabel, false);
    setSliderVisible(fxGuardCeilingSlider, fxGuardCeilingLabel, false);
}

void NateVSTAudioProcessorEditor::setSliderVisible(juce::Slider& slider, juce::Label& label, bool shouldBeVisible)
{
    slider.setVisible(shouldBeVisible);
    label.setVisible(shouldBeVisible);
}

void NateVSTAudioProcessorEditor::setChoiceParameter(const juce::String& parameterID, int choiceIndex)
{
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(choiceIndex)));
        parameter->endChangeGesture();
    }

    if (parameterID == Parameters::ID::oscWave)
        waveformBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::osc2Wave)
        osc2WaveBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::filterMode)
        filterModeBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::sequencerRate)
        sequencerRateBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);

    updateSegmentedSelectors();
}

void NateVSTAudioProcessorEditor::updateSegmentedSelectors()
{
    auto getChoiceIndex = [this] (const juce::ComboBox& sourceBox, const juce::String& parameterID, int fallback)
    {
        const auto selectedIndex = sourceBox.getSelectedItemIndex();
        if (selectedIndex >= 0)
            return selectedIndex;

        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return static_cast<int>(value->load());

        return fallback;
    };

    const auto waveformIndex = getChoiceIndex(waveformBox, Parameters::ID::oscWave, 1);
    sineWaveButton.setToggleState(waveformIndex == 0, juce::dontSendNotification);
    sawWaveButton.setToggleState(waveformIndex == 1, juce::dontSendNotification);
    squareWaveButton.setToggleState(waveformIndex == 2, juce::dontSendNotification);
    triangleWaveButton.setToggleState(waveformIndex == 3, juce::dontSendNotification);

    const auto osc2WaveformIndex = getChoiceIndex(osc2WaveBox, Parameters::ID::osc2Wave, 1);
    osc2SineWaveButton.setToggleState(osc2WaveformIndex == 0, juce::dontSendNotification);
    osc2SawWaveButton.setToggleState(osc2WaveformIndex == 1, juce::dontSendNotification);
    osc2SquareWaveButton.setToggleState(osc2WaveformIndex == 2, juce::dontSendNotification);
    osc2TriangleWaveButton.setToggleState(osc2WaveformIndex == 3, juce::dontSendNotification);

    const auto filterModeIndex = getChoiceIndex(filterModeBox, Parameters::ID::filterMode, 0);
    lowpassFilterButton.setToggleState(filterModeIndex == 0, juce::dontSendNotification);
    bandpassFilterButton.setToggleState(filterModeIndex == 1, juce::dontSendNotification);
    highpassFilterButton.setToggleState(filterModeIndex == 2, juce::dontSendNotification);

    const auto rateIndex = getChoiceIndex(sequencerRateBox, Parameters::ID::sequencerRate, 1);
    rateEighthButton.setToggleState(rateIndex == 0, juce::dontSendNotification);
    rateSixteenthButton.setToggleState(rateIndex == 1, juce::dontSendNotification);
    rateThirtySecondButton.setToggleState(rateIndex == 2, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateLfoCurveDisplay()
{
    std::array<float, 8> values {};
    for (size_t index = 0; index < values.size(); ++index)
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::lfo1Curve[index]))
            values[index] = value->load();

    auto shapeIndex = 0;
    if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::lfo1Shape))
        shapeIndex = static_cast<int>(value->load() + 0.5f);

    lfoCurveDisplay.setValues(values, shapeIndex == 5);

    auto phase = readPlainParameterValue(Parameters::ID::lfo1Phase, 0.0f);
    const auto syncEnabled = readPlainParameterValue(Parameters::ID::lfo1Sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo1SyncRate, 1.0f));
    const auto hostStatus = audioProcessor.getHostSyncStatus();

    if (syncEnabled && hostStatus.ppqAvailable)
    {
        phase += static_cast<float>(std::fmod(hostStatus.ppqPosition * lfoCyclesPerBeatForUi(syncRateIndex), 1.0));
    }
    else
    {
        const auto bpm = juce::jlimit(20.0, 300.0, hostStatus.bpm);
        const auto rateHz = syncEnabled
            ? static_cast<float>((bpm / 60.0) * lfoCyclesPerBeatForUi(syncRateIndex))
            : readPlainParameterValue(Parameters::ID::lfo1Rate, 1.0f);
        phase += static_cast<float>(std::fmod((juce::Time::getMillisecondCounterHiRes() * 0.001) * rateHz, 1.0));
    }

    phase = std::fmod(phase + 1.0f, 1.0f);
    lfoCurveDisplay.setPhase(phase, shapeIndex == 5);
}

void NateVSTAudioProcessorEditor::applyLfoCurvePreset(int presetId)
{
    const auto values = lfoCurvePresetValues(presetId);
    setPlainParameterValue(Parameters::ID::lfo1Shape, 5.0f);

    for (size_t index = 0; index < values.size(); ++index)
        setPlainParameterValue(Parameters::ID::lfo1Curve[index], values[index]);

    updateLfoCurveDisplay();
    updateModDestinationIndicators();
    modMatrixStatusLabel.setText("Loaded LFO curve: " + lfoCurvePresetBox.getText(),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updatePumpCurveDisplay()
{
    std::array<float, 8> customCurve {};
    for (size_t index = 0; index < customCurve.size(); ++index)
        customCurve[index] = readPlainParameterValue(Parameters::ID::fxPumpCustomCurve[index], 0.0f);

    const auto bounce = readPlainParameterValue(Parameters::ID::macroBounce, 0.0f);
    const auto moduleEnabled = readPlainParameterValue(Parameters::ID::fxPumpEnabled, 0.0f) >= 0.5f;

    pumpCurveDisplay.setState(
        juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)),
        juce::jlimit(0.0f, 1.0f, (moduleEnabled ? readPlainParameterValue(Parameters::ID::fxPumpDepth, 0.35f) : 0.0f) + (bounce * 0.5f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::fxPumpShape, 0.45f) + (bounce * 0.16f)),
        readPlainParameterValue(Parameters::ID::fxPumpPhase, 0.0f),
        juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpRate, 0.0f)),
        moduleEnabled || bounce > 0.001f,
        customCurve);
}

void NateVSTAudioProcessorEditor::updateHostSyncStatus()
{
    const auto status = audioProcessor.getHostSyncStatus();
    const auto bpm = juce::roundToInt(juce::jlimit(20.0, 300.0, status.bpm));
    juce::String text;
    juce::String tooltip;
    auto textColour = juce::Colour(0xff7d8b90);
    auto background = juce::Colour(0x22141a1d);
    auto outline = juce::Colour(0xff263035);

    if (status.positionAvailable && status.ppqAvailable && status.playing)
    {
        text = "LOCK " + juce::String(bpm) + " | PLAY";
        tooltip = "Host BPM and PPQ are available. SEQ, Pump, Tremolo, and synced Delay can follow Ableton transport phase. PPQ "
            + juce::String(status.ppqPosition, 2);
        textColour = juce::Colour(0xff8ee6c9);
        background = juce::Colour(0x243bcfa7);
        outline = juce::Colour(0xff3bcfa7);
    }
    else if (status.positionAvailable)
    {
        text = "HOST " + juce::String(bpm) + (status.playing ? " | NO PPQ" : " | STOP");
        tooltip = status.playing
            ? "Host tempo is available, but PPQ phase is not. Tempo-synced movement uses internal phase fallback."
            : "Host tempo is available and transport is stopped. The sequencer waits; tempo-synced FX can audition from internal phase.";
        textColour = juce::Colour(0xffffc29a);
        background = juce::Colour(0x22ff8a4d);
        outline = juce::Colour(0xff705846);
    }
    else
    {
        text = "INT " + juce::String(bpm) + " | FREE";
        tooltip = "No host tempo or transport position has reached the audio engine yet. Nate VST is using its internal fallback tempo.";
    }

    hostSyncStatusLabel.setText(text, juce::dontSendNotification);
    hostSyncStatusLabel.setTooltip(tooltip);
    hostSyncStatusLabel.setColour(juce::Label::textColourId, textColour);
    hostSyncStatusLabel.setColour(juce::Label::backgroundColourId, background);
    hostSyncStatusLabel.setColour(juce::Label::outlineColourId, outline);
}

void NateVSTAudioProcessorEditor::updateModMatrixRows()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    std::vector<int> sourceRouteCounts(static_cast<size_t>(sourceChoices.size()), 0);
    std::vector<float> sourceDepths(static_cast<size_t>(sourceChoices.size()), 0.0f);
    auto activeRouteCount = 0;
    juce::String firstActiveRoute;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto choiceName = [] (const juce::StringArray& choices, int index)
    {
        if (juce::isPositiveAndBelow(index, choices.size()))
            return choices[index];

        return juce::String("Off");
    };

    for (size_t index = 0; index < modMatrixRows.size(); ++index)
    {
        const auto sourceIndex = juce::jlimit(0,
                                             sourceChoices.size() - 1,
                                             static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f))));
        const auto destinationIndex = juce::jlimit(0,
                                                  destinationChoices.size() - 1,
                                                  static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f))));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;
        const auto sourceText = choiceName(sourceChoices, sourceIndex);
        const auto destinationText = choiceName(destinationChoices, destinationIndex);
        const auto isConfiguredRoute = sourceIndex > 0 && destinationIndex > 0 && std::abs(amount) > 0.001f;
        const auto isActiveRoute = enabled && isConfiguredRoute;

        modMatrixRows[index].setState(static_cast<int>(index + 1), sourceText, destinationText, amount, enabled);
        modSlotEnabledButtons[index].setButtonText(enabled ? "On" : "Off");
        modSlotEnabledButtons[index].setEnabled(isConfiguredRoute);
        modSlotDeleteButtons[index].setEnabled(isConfiguredRoute);

        if (isActiveRoute)
        {
            ++activeRouteCount;
            if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            {
                sourceRouteCounts[static_cast<size_t>(sourceIndex)] += 1;
                sourceDepths[static_cast<size_t>(sourceIndex)] = juce::jlimit(-1.0f,
                                                                              1.0f,
                                                                              sourceDepths[static_cast<size_t>(sourceIndex)] + amount);
            }

            if (firstActiveRoute.isEmpty())
            {
                const auto percent = juce::roundToInt(amount * 100.0f);
                firstActiveRoute = sourceText + " -> " + destinationText
                    + " " + (percent >= 0 ? "+" : "") + juce::String(percent);
            }
        }
    }

    const auto statusText = activeRouteCount == 0
        ? juce::String("No active routes")
        : juce::String(activeRouteCount) + " active | " + firstActiveRoute;
    modMatrixStatusLabel.setText(statusText, juce::dontSendNotification);

    for (size_t rowIndex = 0; rowIndex < modSourceRows.size(); ++rowIndex)
    {
        const auto sourceIndex = rowIndex + 1;
        const auto sourceChoiceIndex = static_cast<int>(sourceIndex);
        const auto routeCount = juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
            ? sourceRouteCounts[sourceIndex]
            : 0;
        const auto depth = juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
            ? sourceDepths[sourceIndex]
            : 0.0f;
        auto text = modSourceSummaryText(rowIndex);

        if (routeCount > 0)
        {
            const auto percent = juce::roundToInt(depth * 100.0f);
            text += " | " + juce::String(routeCount) + " route" + (routeCount == 1 ? "" : "s")
                + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%";
        }

        auto& label = modSourceRows[rowIndex];
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId,
                        routeCount > 0
                            ? (depth < 0.0f ? juce::Colour(0xffffc29a) : juce::Colour(0xffd9fff1))
                            : juce::Colour(0xffc7d7d4));
        label.setColour(juce::Label::backgroundColourId,
                        routeCount > 0
                            ? (depth < 0.0f ? juce::Colour(0x22ff8a4d) : juce::Colour(0x223bcfa7))
                            : juce::Colours::transparentBlack);
        label.setTooltip(routeCount > 0
                             ? sourceChoices[sourceChoiceIndex] + ": " + juce::String(routeCount)
                                + " active route" + (routeCount == 1 ? "" : "s")
                             : juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
                                 ? sourceChoices[sourceChoiceIndex] + ": no active routes"
                                 : juce::String("No source"));
    }
}

void NateVSTAudioProcessorEditor::updateModInspectorStatus()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = juce::jlimit(1,
                                                 destinationChoices.size() - 1,
                                                 modInspectorDestinationBox.getSelectedId() - 1);
    const auto destinationName = destinationChoices[selectedDestination];
    juce::StringArray routes;
    auto summedDepth = 0.0f;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::jlimit(0,
                                             sourceChoices.size() - 1,
                                             static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f))));
        const auto destinationIndex = juce::jlimit(0,
                                                  destinationChoices.size() - 1,
                                                  static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f))));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (sourceIndex <= 0 || destinationIndex != selectedDestination || std::abs(amount) <= 0.001f)
            continue;

        if (enabled)
            summedDepth += amount;

        const auto percent = juce::roundToInt(amount * 100.0f);
        routes.add("S" + juce::String(static_cast<int>(index + 1)) + " "
                   + sourceChoices[sourceIndex] + " "
                   + (percent >= 0 ? "+" : "") + juce::String(percent) + "%"
                   + (enabled ? "" : " off"));
    }

    if (routes.isEmpty())
    {
        modInspectorStatusLabel.setText(destinationName + ": no active routes", juce::dontSendNotification);
        modInspectorStatusLabel.setTooltip("No modulation routes currently target " + destinationName);
        modInspectorClearButton.setEnabled(false);
        return;
    }

    const auto summedPercent = juce::roundToInt(juce::jlimit(-1.0f, 1.0f, summedDepth) * 100.0f);
    const auto summary = destinationName + " | " + routes.joinIntoString(" | ")
        + " | Sum " + (summedPercent >= 0 ? "+" : "") + juce::String(summedPercent) + "%";
    modInspectorStatusLabel.setText(summary, juce::dontSendNotification);
    modInspectorStatusLabel.setTooltip(summary);
    modInspectorClearButton.setEnabled(true);
}

void NateVSTAudioProcessorEditor::updateMacroAssignmentEditorStatus()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    const auto destinationIndex = selectedMacroAssignmentDestinationIndex();
    const auto sourceName = sourceChoices[sourceIndex];
    juce::StringArray routes;
    auto selectedRouteExists = false;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (currentSource != sourceIndex
            || currentDestination <= 0
            || ! juce::isPositiveAndBelow(currentDestination, destinationChoices.size())
            || std::abs(amount) <= 0.001f)
        {
            continue;
        }

        if (currentDestination == destinationIndex)
            selectedRouteExists = true;

        const auto percent = juce::roundToInt(amount * 100.0f);
        routes.add(destinationChoices[currentDestination]
                   + " " + (percent >= 0 ? "+" : "") + juce::String(percent)
                   + (enabled ? "" : " off"));
    }

    const auto targetAmount = juce::roundToInt(modMacroAssignAmountSlider.getValue());
    const auto targetText = destinationChoices[destinationIndex]
        + " " + (targetAmount >= 0 ? "+" : "") + juce::String(targetAmount) + "%";
    const auto summary = routes.isEmpty()
        ? sourceName + ": no assignments | target " + targetText
        : sourceName + ": " + routes.joinIntoString(", ");

    modMacroAssignStatusLabel.setText(summary, juce::dontSendNotification);
    modMacroAssignStatusLabel.setTooltip(summary + " | Add updates a matching route, Replace keeps only the target route for this macro");
    modMacroAssignAddButton.setButtonText(selectedRouteExists ? "Update" : "Add");
    modMacroAssignClearButton.setEnabled(! routes.isEmpty());
}

void NateVSTAudioProcessorEditor::setModInspectorDestination(int destinationIndex)
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    destinationIndex = juce::jlimit(1, destinationChoices.size() - 1, destinationIndex);
    modInspectorDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);
    updateModInspectorStatus();
}

void NateVSTAudioProcessorEditor::addInspectedModRoute()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    auto sourceIndex = juce::jlimit(1,
                                    sourceChoices.size() - 1,
                                    modInspectorSourceBox.getSelectedId() - 1);
    const auto destinationIndex = juce::jlimit(1,
                                              destinationChoices.size() - 1,
                                              modInspectorDestinationBox.getSelectedId() - 1);
    if (destinationUsesGlobalModulationSources(destinationIndex) && (sourceIndex == 2 || sourceIndex == 3))
    {
        sourceIndex = 1;
        modInspectorSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
    }

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto targetSlot = -1;
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f)));
        const auto currentDestination = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        const auto currentAmount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);

        if (currentSource <= 0 || currentDestination <= 0 || std::abs(currentAmount) <= 0.001f)
        {
            targetSlot = static_cast<int>(index);
            break;
        }
    }

    if (targetSlot < 0)
    {
        modMatrixStatusLabel.setText("No free modulation slot", juce::dontSendNotification);
        return;
    }

    auto defaultAmount = 0.28f;
    if (destinationUsesGlobalModulationSources(destinationIndex))
        defaultAmount = 0.24f;
    else if (sourceIndex == 1 || sourceIndex == 2)
        defaultAmount = 0.35f;
    else if (sourceIndex == 3)
        defaultAmount = 0.20f;
    else if (sourceIndex >= 4)
        defaultAmount = 0.30f;

    const auto slotIndex = static_cast<size_t>(targetSlot);
    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], static_cast<float>(sourceIndex));
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], static_cast<float>(destinationIndex));
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], defaultAmount);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto percent = juce::roundToInt(defaultAmount * 100.0f);
    modMatrixStatusLabel.setText("Added S" + juce::String(targetSlot + 1) + " "
                                     + sourceChoices[sourceIndex] + " -> " + destinationChoices[destinationIndex]
                                     + " +" + juce::String(percent) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::addMacroAssignment(bool replaceExisting)
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    const auto destinationIndex = selectedMacroAssignmentDestinationIndex();
    auto amount = static_cast<float>(modMacroAssignAmountSlider.getValue() / 100.0);

    if (std::abs(amount) < 0.001f)
        amount = 0.01f;

    auto targetSlot = -1;
    auto firstEmptySlot = -1;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto currentAmount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto isEmpty = currentSource <= 0 || currentDestination <= 0 || std::abs(currentAmount) <= 0.001f;

        if (! replaceExisting && currentSource == sourceIndex && currentDestination == destinationIndex)
            targetSlot = static_cast<int>(index);

        if (replaceExisting && currentSource == sourceIndex)
        {
            if (targetSlot < 0)
                targetSlot = static_cast<int>(index);

            setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
            continue;
        }

        if (firstEmptySlot < 0 && isEmpty)
            firstEmptySlot = static_cast<int>(index);
    }

    if (targetSlot < 0)
        targetSlot = firstEmptySlot;

    if (targetSlot < 0)
    {
        modMatrixStatusLabel.setText("No free modulation slot for " + sourceChoices[sourceIndex], juce::dontSendNotification);
        updateMacroAssignmentEditorStatus();
        return;
    }

    const auto slotIndex = static_cast<size_t>(targetSlot);
    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], static_cast<float>(sourceIndex));
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], static_cast<float>(destinationIndex));
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], amount);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);

    modInspectorSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
    modInspectorDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto percent = juce::roundToInt(amount * 100.0f);
    modMatrixStatusLabel.setText(juce::String(replaceExisting ? "Replaced " : "Assigned ")
                                     + sourceChoices[sourceIndex] + " -> " + destinationChoices[destinationIndex]
                                     + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::clearSelectedMacroAssignments()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    auto clearedCount = 0;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        if (currentSource != sourceIndex)
            continue;

        setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
        ++clearedCount;
    }

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    modMatrixStatusLabel.setText(clearedCount > 0
                                     ? "Cleared " + juce::String(clearedCount) + " " + sourceChoices[sourceIndex] + " route"
                                        + (clearedCount == 1 ? "" : "s")
                                     : sourceChoices[sourceIndex] + " had no routes",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::deleteModRoute(size_t slotIndex)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    modMatrixStatusLabel.setText("Deleted S" + juce::String(static_cast<int>(slotIndex + 1)),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::clearInspectedModRoutes()
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = juce::jlimit(1,
                                                 destinationChoices.size() - 1,
                                                 modInspectorDestinationBox.getSelectedId() - 1);
    auto clearedCount = 0;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto destinationIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        if (destinationIndex != selectedDestination)
            continue;

        setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
        ++clearedCount;
    }

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    if (clearedCount > 0)
        modMatrixStatusLabel.setText("Cleared " + juce::String(clearedCount)
                                         + " route" + (clearedCount == 1 ? "" : "s")
                                         + " to " + destinationChoices[selectedDestination],
                                     juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateModDestinationIndicators()
{
    std::array<float, 18> destinationDepths {};
    std::array<int, 18> destinationRouteCounts {};
    std::array<juce::StringArray, 18> destinationSources {};
    const auto sourceChoices = Parameters::modulationSourceChoices();

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f)));
        const auto destinationIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0 || destinationIndex <= 0 || destinationIndex >= static_cast<int>(destinationDepths.size()))
            continue;

        destinationDepths[static_cast<size_t>(destinationIndex)] += amount;
        destinationRouteCounts[static_cast<size_t>(destinationIndex)] += 1;
        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            destinationSources[static_cast<size_t>(destinationIndex)].addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    auto setIndicator = [] (juce::Slider& slider, float amount, int routeCount, const juce::StringArray& sources)
    {
        amount = juce::jlimit(-1.0f, 1.0f, amount);

        auto& properties = slider.getProperties();
        const auto previous = static_cast<float>(static_cast<double>(properties.getWithDefault("modAmount", 0.0)));
        const auto previousCount = static_cast<int>(properties.getWithDefault("modRouteCount", 0));
        const auto previousSources = properties.getWithDefault("modSourceSummary", {}).toString();
        const auto sourceSummary = sources.joinIntoString(", ");
        if (std::abs(previous - amount) < 0.001f
            && previousCount == routeCount
            && previousSources == sourceSummary)
            return;

        properties.set("modAmount", amount);
        properties.set("modRouteCount", routeCount);
        properties.set("modSourceSummary", sourceSummary);
        slider.setTooltip(routeCount > 0
                              ? "Modulated by " + sourceSummary
                                  + " | Sum " + (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%"
                              : juce::String {});
        slider.repaint();
    };

    setIndicator(cutoffSlider, destinationDepths[1], destinationRouteCounts[1], destinationSources[1]);
    setIndicator(resonanceSlider, destinationDepths[2], destinationRouteCounts[2], destinationSources[2]);
    setIndicator(filterEnvSlider, destinationDepths[3], destinationRouteCounts[3], destinationSources[3]);
    setIndicator(driveSlider, destinationDepths[4], destinationRouteCounts[4], destinationSources[4]);
    setIndicator(osc2TuneSlider, destinationDepths[5], destinationRouteCounts[5], destinationSources[5]);
    setIndicator(osc2LevelSlider, destinationDepths[6], destinationRouteCounts[6], destinationSources[6]);
    setIndicator(fxPumpDepthSlider, destinationDepths[7], destinationRouteCounts[7], destinationSources[7]);
    setIndicator(fxDelayMixSlider, destinationDepths[8], destinationRouteCounts[8], destinationSources[8]);
    setIndicator(fxReverbMixSlider, destinationDepths[9], destinationRouteCounts[9], destinationSources[9]);
    setIndicator(fxWidthAmountSlider, destinationDepths[10], destinationRouteCounts[10], destinationSources[10]);
    setIndicator(fxDistortionAmountSlider, destinationDepths[11], destinationRouteCounts[11], destinationSources[11]);
    setIndicator(sampleStartSlider, destinationDepths[12], destinationRouteCounts[12], destinationSources[12]);
    setIndicator(sampleMixSlider, destinationDepths[13], destinationRouteCounts[13], destinationSources[13]);
    setIndicator(sampleTransposeSlider, destinationDepths[14], destinationRouteCounts[14], destinationSources[14]);
    setIndicator(samplePitchRampSlider, destinationDepths[15], destinationRouteCounts[15], destinationSources[15]);
    setIndicator(sampleStutterRepeatsSlider, destinationDepths[16], destinationRouteCounts[16], destinationSources[16]);
    setIndicator(oscWarpSlider, destinationDepths[17], destinationRouteCounts[17], destinationSources[17]);
}

void NateVSTAudioProcessorEditor::updateOutputMeter()
{
    auto peakLeft = 0.0f;
    auto peakRight = 0.0f;
    auto rmsLeft = 0.0f;
    auto rmsRight = 0.0f;
    audioProcessor.getOutputMeterLevels(peakLeft, peakRight, rmsLeft, rmsRight);

    displayedPeakLeft = smoothMeterValue(displayedPeakLeft, peakLeft);
    displayedPeakRight = smoothMeterValue(displayedPeakRight, peakRight);
    displayedRmsLeft = smoothMeterValue(displayedRmsLeft, rmsLeft);
    displayedRmsRight = smoothMeterValue(displayedRmsRight, rmsRight);
    outputMeter.setLevels(displayedPeakLeft, displayedPeakRight, displayedRmsLeft, displayedRmsRight);
}

void NateVSTAudioProcessorEditor::updateLowEndAssistant()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto subRms = 0.0f;
    auto lowStereoRisk = 0.0f;
    auto outputPeak = 0.0f;
    audioProcessor.getLowEndMeterLevels(subRms, lowStereoRisk, outputPeak);

    const auto rootNote = juce::jlimit(0, 127, juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 36.0f)));
    const auto rootName = juce::MidiMessage::getMidiNoteName(rootNote, true, true, 3);
    const auto rootHz = juce::MidiMessage::getMidiNoteInHertz(rootNote);
    const auto unisonVoiceCount = juce::jlimit(1, 8, juce::roundToInt(readParameter(Parameters::ID::unisonVoices, 1.0f)));
    const auto unisonSpreadAmount = readParameter(Parameters::ID::unisonSpread, 0.0f);

    UI::LowEndAssistant::State state;
    state.rootText = rootName + " " + juce::String(static_cast<int>(std::round(rootHz))) + "Hz";
    state.subRms = subRms;
    state.lowStereoRisk = lowStereoRisk;
    state.outputPeak = outputPeak;
    state.monoCrossoverHz = readParameter(Parameters::ID::fxWidthMonoCutoff, 120.0f);
    state.monoEnabled = readParameter(Parameters::ID::monoMode, 0.0f) >= 0.5f;
    state.widthEnabled = readParameter(Parameters::ID::fxWidthEnabled, 0.0f) >= 0.5f;
    state.guardEnabled = readParameter(Parameters::ID::fxGuardEnabled, 0.0f) >= 0.5f;

    if (! state.monoEnabled && ! state.widthEnabled && lowStereoRisk >= 0.28f)
    {
        state.phaseText = "LOW SIDE";
        state.guidanceText = "Mono lows before club play";
        state.guidanceLevel = 2;
    }
    else if (! state.monoEnabled && ! state.widthEnabled && lowStereoRisk >= 0.16f)
    {
        state.phaseText = "LOW SIDE";
        state.guidanceText = "Width off: watch bass sides";
        state.guidanceLevel = 1;
    }
    else if (rootHz < 40.0)
    {
        state.phaseText = "ROOT LOW";
        state.guidanceText = "Raise root for club subs";
        state.guidanceLevel = 1;
    }
    else if (rootHz > 82.0)
    {
        state.phaseText = "ROOT HIGH";
        state.guidanceText = "Root above sub sweet spot";
        state.guidanceLevel = 1;
    }
    else if (! state.guardEnabled && outputPeak >= 0.9f)
    {
        state.phaseText = "HEADROOM";
        state.guidanceText = "Guard off near ceiling";
        state.guidanceLevel = 1;
    }
    else if (state.monoEnabled && unisonVoiceCount > 1 && unisonSpreadAmount > 0.05f)
    {
        state.phaseText = "SPREAD LOCK";
        state.guidanceText = "Mono is collapsing spread";
        state.guidanceLevel = 0;
    }
    else
    {
        state.phaseText = "RESET OK";
        state.guidanceText = "Phase resets on note start";
        state.guidanceLevel = 0;
    }

    lowEndAssistant.setState(state);
}

void NateVSTAudioProcessorEditor::updatePerformanceSnapshotButtons()
{
    const auto hasSnapshotA = audioProcessor.hasPerformanceSnapshot(0);
    const auto hasSnapshotB = audioProcessor.hasPerformanceSnapshot(1);

    recallSnapshotAButton.setEnabled(hasSnapshotA);
    recallSnapshotBButton.setEnabled(hasSnapshotB);
    performanceStatusLabel.setText(juce::String(hasSnapshotA ? "A ready" : "A empty")
                                       + " | "
                                       + juce::String(hasSnapshotB ? "B ready" : "B empty"),
                                   juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updatePerformanceXYPad()
{
    auto readParameter = [this] (const juce::String& parameterID)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return 0.0f;
    };

    performanceXYPad.setValues(readParameter(Parameters::ID::macroMotion),
                               readParameter(Parameters::ID::macroSpace));
}

void NateVSTAudioProcessorEditor::updateSequencerGridContext()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    const auto root = juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 36.0f));
    const auto octaveOffset = juce::roundToInt(readParameter(Parameters::ID::sequencerOctave, 0.0f)) * 12;
    const auto scaleMode = juce::roundToInt(readParameter(Parameters::ID::sequencerScale, 0.0f));
    sequencerGrid.setRootNote(juce::jlimit(0, 127, root + octaveOffset));
    sequencerGrid.setScaleMode(scaleMode);
}

void NateVSTAudioProcessorEditor::timerCallback()
{
    if (activePresetAuditionNote >= 0
        && juce::Time::getMillisecondCounterHiRes() >= presetAuditionNoteOffMs)
    {
        releasePresetAuditionNote();
    }

    updateSegmentedSelectors();
    updateLfoCurveDisplay();
    updatePumpCurveDisplay();
    updateHostSyncStatus();
    updateModMatrixRows();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();
    updateModDestinationIndicators();
    updateOutputMeter();
    updateLowEndAssistant();
    updatePerformanceSnapshotButtons();
    updatePerformanceXYPad();
    updateSequencerGridContext();
    updateSampleSliceButtons();
    updateSampleWaveformDisplay();
    updateKeyboardRangeLabel();
    updateFxRackControls();
}

int NateVSTAudioProcessorEditor::getNumRows()
{
    return static_cast<int>(visiblePresetBrowserPresets.size());
}

void NateVSTAudioProcessorEditor::paintListBoxItem(int rowNumber,
                                                    juce::Graphics& g,
                                                    int width,
                                                    int height,
                                                    bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return;

    const auto& preset = visiblePresetBrowserPresets[static_cast<size_t>(rowNumber)];
    const auto rowFill = rowIsSelected
        ? juce::Colour(0xff21342f)
        : (rowNumber % 2 == 0 ? juce::Colour(0xff101619) : juce::Colour(0xff121b1e));

    g.fillAll(rowFill);
    g.setColour(rowIsSelected ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff253036));
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));

    auto row = juce::Rectangle<int>(0, 0, width, height).reduced(8, 3);
    auto drawCell = [&g] (juce::Rectangle<int> area,
                          const juce::String& text,
                          juce::Colour colour,
                          juce::Justification justification = juce::Justification::centredLeft)
    {
        g.setColour(colour);
        g.drawFittedText(text, area.reduced(3, 0), justification, 1);
    };

    const auto categoryText = preset.folder.isNotEmpty() ? preset.folder : preset.category;
    const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + "/5" : juce::String("-");
    const auto sourcePrefix = preset.isFavorite ? juce::String("F ") : juce::String();
    const auto nameColour = preset.isFavorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4);

    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    drawCell(row.removeFromLeft(218), sourcePrefix + preset.name, nameColour);
    drawCell(row.removeFromLeft(112), categoryText, juce::Colour(0xffb7c5c7));
    drawCell(row.removeFromLeft(126), preset.pack, juce::Colour(0xffa8b6b8));
    drawCell(row.removeFromLeft(76), preset.key, juce::Colour(0xffa8b6b8));
    drawCell(row.removeFromLeft(64), formatPresetBpm(preset.bpm), juce::Colour(0xffa8b6b8));
    drawCell(row.removeFromLeft(54), ratingText, juce::Colour(0xffd6e0dc), juce::Justification::centred);
    drawCell(row, preset.macroSummary, preset.macroIntensity >= 0.30f ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff8c9a9d));
}

juce::String NateVSTAudioProcessorEditor::getNameForRow(int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return {};

    return visiblePresetBrowserPresets[static_cast<size_t>(rowNumber)].name;
}

void NateVSTAudioProcessorEditor::selectedRowsChanged(int lastRowSelected)
{
    if (ignorePresetBrowserSelection
        || lastRowSelected < 0
        || lastRowSelected >= getNumRows())
    {
        return;
    }

    const auto& preset = visiblePresetBrowserPresets[static_cast<size_t>(lastRowSelected)];
    presetBox.setSelectedItemIndex(lastRowSelected, juce::dontSendNotification);
    presetNameEditor.setText(preset.name, juce::dontSendNotification);
    updateFavoritePresetButton();
    presetStatusLabel.setText("Selected " + preset.name + " | " + preset.pack + " | "
                                  + preset.key + " | " + formatPresetBpm(preset.bpm)
                                  + " | " + presetMacroPreviewText(preset),
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= getNumRows())
        return;

    selectedRowsChanged(row);
    loadSelectedPreset();
}

void NateVSTAudioProcessorEditor::refreshPresetList()
{
    const auto previousSelection = presetBox.getText();
    presetBox.clear(juce::dontSendNotification);
    visiblePresetBrowserPresets.clear();

    const auto library = audioProcessor.getPresetLibrary();
    const auto recentNames = audioProcessor.getRecentPresetNames();
    auto filter = presetFilterBox.getText().trim();
    if (filter.isEmpty())
        filter = "All";

    auto tagFilter = presetTagBox.getText().trim();
    if (tagFilter.isEmpty())
        tagFilter = "All Tags";

    auto sortMode = presetSortBox.getText().trim();
    if (sortMode.isEmpty())
        sortMode = "Name";

    const auto searchText = presetSearchEditor.getText().trim();
    juce::StringArray searchTerms;
    searchTerms.addTokens(searchText, " ", "\"");
    searchTerms.removeEmptyStrings();

    auto matchesSearch = [&searchTerms] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        if (searchTerms.isEmpty())
            return true;

        const auto sourceText = preset.isFactory ? juce::String("Factory") : juce::String("User");
        const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + " Star" : juce::String("Unrated");
        const auto bpmText = formatPresetBpm(preset.bpm);
        const auto searchable = preset.name + " " + preset.category + " " + preset.source + " " + preset.tags + " "
            + preset.folder + " " + sourceText + " " + ratingText + " " + preset.author + " "
            + preset.pack + " " + preset.key + " " + bpmText + " " + preset.macroSummary + " Macro Macros Performance"
            + (preset.isFavorite ? " Favorite" : "");

        for (const auto& term : searchTerms)
            if (! searchable.containsIgnoreCase(term))
                return false;

        return true;
    };

    auto matchesTag = [&tagFilter] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        if (tagFilter == "All Tags")
            return true;

        return preset.tags.containsIgnoreCase(tagFilter)
            || preset.category.equalsIgnoreCase(tagFilter)
            || preset.pack.equalsIgnoreCase(tagFilter)
            || preset.key.equalsIgnoreCase(tagFilter)
            || preset.author.equalsIgnoreCase(tagFilter)
            || preset.name.containsIgnoreCase(tagFilter);
    };

    auto nextItemId = 1;
    auto addPreset = [this, &nextItemId] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        presetBox.addItem(preset.name, nextItemId++);
        visiblePresetBrowserPresets.push_back(preset);
    };

    auto sortPresets = [&sortMode] (std::vector<NateVSTAudioProcessor::PresetInfo>& presetsToSort)
    {
        std::stable_sort(presetsToSort.begin(),
                         presetsToSort.end(),
                         [&sortMode] (const auto& left, const auto& right)
                         {
                             if (sortMode == "Rating" && left.rating != right.rating)
                                 return left.rating > right.rating;

                             if (sortMode == "Newest" && left.lastModifiedMs != right.lastModifiedMs)
                                 return left.lastModifiedMs > right.lastModifiedMs;

                             if (sortMode == "Category")
                             {
                                 const auto categoryCompare = left.category.compareIgnoreCase(right.category);
                                 if (categoryCompare != 0)
                                     return categoryCompare < 0;
                             }

                             if (sortMode == "Pack")
                             {
                                 const auto packCompare = left.pack.compareIgnoreCase(right.pack);
                                 if (packCompare != 0)
                                     return packCompare < 0;
                             }

                             if (sortMode == "BPM" && left.bpm != right.bpm)
                                 return left.bpm > 0 && (right.bpm == 0 || left.bpm < right.bpm);

                             if (sortMode == "Key")
                             {
                                 const auto keyCompare = left.key.compareIgnoreCase(right.key);
                                 if (keyCompare != 0)
                                     return keyCompare < 0;
                             }

                             if (sortMode == "Author")
                             {
                                 const auto authorCompare = left.author.compareIgnoreCase(right.author);
                                 if (authorCompare != 0)
                                     return authorCompare < 0;
                             }

                             if (sortMode == "Source")
                             {
                                 const auto sourceCompare = left.source.compareIgnoreCase(right.source);
                                 if (sourceCompare != 0)
                                     return sourceCompare < 0;
                             }

                             if (sortMode == "Macros" && std::abs(left.macroIntensity - right.macroIntensity) > 0.001f)
                                 return left.macroIntensity > right.macroIntensity;

                             return left.name.compareIgnoreCase(right.name) < 0;
                         });
    };

    auto findPreset = [&library] (const juce::String& name) -> const NateVSTAudioProcessor::PresetInfo*
    {
        for (const auto& preset : library)
            if (preset.name == name)
                return &preset;

        return nullptr;
    };

    if (filter == "Recent")
    {
        for (const auto& recentName : recentNames)
            if (const auto* preset = findPreset(recentName))
                if (matchesTag(*preset) && matchesSearch(*preset))
                    addPreset(*preset);
    }
    else
    {
        std::vector<NateVSTAudioProcessor::PresetInfo> visiblePresets;
        for (const auto& preset : library)
        {
            const auto leafCategory = preset.category.fromLastOccurrenceOf("/", false, true);
            const auto matchesFilter = filter == "All"
                || (filter == "Favorites" && preset.isFavorite)
                || (filter == "Rated" && preset.rating > 0)
                || (filter == "5 Stars" && preset.rating == 5)
                || (filter == "4+ Stars" && preset.rating >= 4)
                || (filter == "Macro Rich" && preset.macroIntensity >= 0.30f)
                || (filter == "User" && ! preset.isFactory)
                || (filter == "Factory" && preset.isFactory)
                || (filter == "120-124 BPM" && preset.bpm >= 120 && preset.bpm <= 124)
                || (filter == "125-128 BPM" && preset.bpm >= 125 && preset.bpm <= 128)
                || (filter == "129-132 BPM" && preset.bpm >= 129 && preset.bpm <= 132)
                || (filter == "133+ BPM" && preset.bpm >= 133)
                || preset.category.equalsIgnoreCase(filter)
                || leafCategory.equalsIgnoreCase(filter)
                || preset.pack.equalsIgnoreCase(filter)
                || preset.key.equalsIgnoreCase(filter)
                || preset.author.equalsIgnoreCase(filter)
                || preset.folder.startsWithIgnoreCase(filter + "/");

            if (matchesFilter && matchesTag(preset) && matchesSearch(preset))
                visiblePresets.push_back(preset);
        }

        sortPresets(visiblePresets);
        for (const auto& preset : visiblePresets)
            addPreset(preset);
    }

    auto previousIndex = -1;
    for (auto index = 0; index < presetBox.getNumItems(); ++index)
    {
        if (presetBox.getItemText(index) == previousSelection)
        {
            previousIndex = index;
            break;
        }
    }
    if (previousIndex >= 0)
        presetBox.setSelectedItemIndex(previousIndex, juce::dontSendNotification);
    else if (presetBox.getNumItems() > 0)
        presetBox.setSelectedItemIndex(0, juce::dontSendNotification);

    presetBrowserList.updateContent();
    auto selectedBrowserRow = -1;
    for (auto index = 0; index < getNumRows(); ++index)
    {
        if (visiblePresetBrowserPresets[static_cast<size_t>(index)].name == presetBox.getText())
        {
            selectedBrowserRow = index;
            break;
        }
    }

    ignorePresetBrowserSelection = true;
    if (selectedBrowserRow >= 0)
        presetBrowserList.selectRow(selectedBrowserRow);
    else
        presetBrowserList.deselectAllRows();
    ignorePresetBrowserSelection = false;

    const auto* selectedPreset = findPreset(presetBox.getText());
    auto statusText = juce::String(presetBox.getNumItems()) + " presets | Filter: " + filter;
    if (tagFilter != "All Tags")
        statusText += " | Tag: " + tagFilter;
    statusText += " | Sort: " + sortMode;
    if (searchText.isNotEmpty())
        statusText += " | Search: " + searchText;
    if (selectedPreset != nullptr)
        statusText += " | " + presetMacroPreviewText(*selectedPreset);
    statusText += " | User: " + audioProcessor.getPresetDirectory().getFullPathName();
    presetStatusLabel.setText(statusText, juce::dontSendNotification);
    presetStatusLabel.setTooltip(selectedPreset != nullptr
                                     ? presetMacroPreviewText(*selectedPreset) + " | " + selectedPreset->pack + " | " + selectedPreset->key + " | " + formatPresetBpm(selectedPreset->bpm)
                                     : juce::String("Preset browser status"));
    presetBox.setTooltip(selectedPreset != nullptr
                             ? selectedPreset->name + " | " + presetMacroPreviewText(*selectedPreset)
                             : juce::String("Select a preset"));
    updateFavoritePresetButton();
}

void NateVSTAudioProcessorEditor::saveCurrentPreset()
{
    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
        presetName = presetBox.getText().trim();

    NateVSTAudioProcessor::PresetSaveOptions options;
    options.category = presetCategoryBox.getText().trim();
    options.author = presetAuthorEditor.getText().trim();
    options.pack = presetPackBox.getText().trim();
    options.key = presetKeyBox.getText().trim();
    options.bpm = parsePresetBpm(presetBpmBox.getText());

    if (audioProcessor.savePreset(presetName, options))
    {
        auto storedName = juce::File::createLegalFileName(presetName);
        if (storedName.isEmpty())
            storedName = "Untitled";

        presetStatusLabel.setText("Saved " + storedName, juce::dontSendNotification);
        refreshPresetList();
        presetBox.setText(storedName, juce::dontSendNotification);
        presetNameEditor.setText(storedName, juce::dontSendNotification);
        updateFavoritePresetButton();
        return;
    }

    presetStatusLabel.setText("Preset name required", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::loadSelectedPreset()
{
    releasePresetAuditionNote();

    const auto presetName = presetBox.getText().trim();
    if (audioProcessor.loadPreset(presetName))
    {
        refreshPresetList();
        presetBox.setText(presetName, juce::dontSendNotification);
        presetNameEditor.setText(presetName, juce::dontSendNotification);
        presetStatusLabel.setText("Loaded " + presetName, juce::dontSendNotification);
        updateFavoritePresetButton();
        updateSampleNameLabel();
        sequencerGrid.repaint();
        return;
    }

    presetStatusLabel.setText("Select a preset to load", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::loadPresetByOffset(int offset)
{
    const auto presetCount = presetBox.getNumItems();
    if (presetCount == 0)
    {
        presetStatusLabel.setText("No presets saved", juce::dontSendNotification);
        return;
    }

    auto selectedIndex = presetBox.getSelectedItemIndex();
    if (selectedIndex < 0)
        selectedIndex = 0;
    else
        selectedIndex = (selectedIndex + offset + presetCount) % presetCount;

    presetBox.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
    loadSelectedPreset();
}

void NateVSTAudioProcessorEditor::auditionSelectedPreset()
{
    releasePresetAuditionNote();

    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
    {
        presetStatusLabel.setText("Select a preset to audition", juce::dontSendNotification);
        return;
    }

    if (! audioProcessor.loadPreset(presetName))
    {
        presetStatusLabel.setText("Select a preset to audition", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    presetBox.setText(presetName, juce::dontSendNotification);
    presetNameEditor.setText(presetName, juce::dontSendNotification);
    updateFavoritePresetButton();
    updateSampleNameLabel();
    sequencerGrid.repaint();

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (const auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto note = juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 60.0f)
                                 + (readParameter(Parameters::ID::sequencerOctave, 0.0f) * 12.0f));
    while (note < 48)
        note += 12;
    while (note > 72)
        note -= 12;

    activePresetAuditionNote = juce::jlimit(0, 127, note);
    presetAuditionNoteOffMs = juce::Time::getMillisecondCounterHiRes() + presetAuditionDurationMs;
    audioProcessor.getMidiKeyboardState().noteOn(1, activePresetAuditionNote, presetAuditionVelocity);

    presetStatusLabel.setText("Auditioning " + presetName + " | "
                                  + juce::MidiMessage::getMidiNoteName(activePresetAuditionNote, true, true, 3),
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::releasePresetAuditionNote()
{
    if (activePresetAuditionNote < 0)
        return;

    audioProcessor.getMidiKeyboardState().noteOff(1, activePresetAuditionNote, 0.0f);
    activePresetAuditionNote = -1;
    presetAuditionNoteOffMs = 0.0;
}

void NateVSTAudioProcessorEditor::toggleFavoritePreset()
{
    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
    {
        presetStatusLabel.setText("Select a preset to favorite", juce::dontSendNotification);
        return;
    }

    const auto shouldBeFavorite = ! audioProcessor.isPresetFavorite(presetName);
    if (! audioProcessor.setPresetFavorite(presetName, shouldBeFavorite))
    {
        presetStatusLabel.setText("Favorite update failed", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    if (shouldBeFavorite || presetFilterBox.getText() != "Favorites")
        presetBox.setText(presetName, juce::dontSendNotification);

    updateFavoritePresetButton();
    presetStatusLabel.setText(juce::String(shouldBeFavorite ? "Favorited " : "Unfavorited ") + presetName,
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::setSelectedPresetRating()
{
    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
        return;

    const auto selectedId = presetRatingBox.getSelectedId();
    if (selectedId <= 0)
        return;

    const auto rating = juce::jlimit(0, 5, selectedId - 1);
    if (! audioProcessor.setPresetRating(presetName, rating))
    {
        presetStatusLabel.setText("Rating update failed", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    if ((rating > 0 || presetFilterBox.getText() != "Rated")
        && (rating == 5 || presetFilterBox.getText() != "5 Stars")
        && (rating >= 4 || presetFilterBox.getText() != "4+ Stars"))
    {
        presetBox.setText(presetName, juce::dontSendNotification);
    }

    updateFavoritePresetButton();
    presetStatusLabel.setText(rating > 0
                                  ? "Rated " + presetName + " | " + juce::String(rating) + " star" + (rating == 1 ? "" : "s")
                                  : "Cleared rating for " + presetName,
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateFavoritePresetButton()
{
    const auto presetName = presetBox.getText().trim();
    auditionPresetButton.setEnabled(presetName.isNotEmpty());
    favoritePresetButton.setEnabled(presetName.isNotEmpty());
    favoritePresetButton.setToggleState(presetName.isNotEmpty() && audioProcessor.isPresetFavorite(presetName),
                                        juce::dontSendNotification);
    presetRatingBox.setEnabled(presetName.isNotEmpty());
    presetRatingBox.setSelectedId(presetName.isNotEmpty() ? audioProcessor.getPresetRating(presetName) + 1 : 1,
                                  juce::dontSendNotification);

    if (presetName.isEmpty())
        return;

    for (const auto& preset : audioProcessor.getPresetLibrary())
    {
        if (preset.name != presetName)
            continue;

        presetCategoryBox.setText(preset.folder.isNotEmpty() ? preset.folder : preset.category,
                                  juce::dontSendNotification);
        presetAuthorEditor.setText(preset.author, juce::dontSendNotification);
        presetPackBox.setText(preset.pack, juce::dontSendNotification);
        presetKeyBox.setText(preset.key, juce::dontSendNotification);
        presetBpmBox.setText(formatPresetBpm(preset.bpm), juce::dontSendNotification);
        const auto preview = presetMacroPreviewText(preset);
        presetBox.setTooltip(preset.name + " | " + preview);
        presetStatusLabel.setTooltip(preview + " | " + preset.pack + " | " + preset.key + " | " + formatPresetBpm(preset.bpm));
        break;
    }
}
