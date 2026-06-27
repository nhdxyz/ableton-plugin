#include "PluginEditor.h"

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
    return { "User", "Bass", "Stab", "Lead", "UKG", "FX", "Sequence", "Sample" };
}

juce::StringArray presetFilterChoices()
{
    return { "All", "Favorites", "Recent", "User", "Factory", "Bass", "Stab", "Lead", "UKG", "FX", "Sequence", "Sample" };
}

float smoothMeterValue(float current, float target)
{
    target = juce::jlimit(0.0f, 2.0f, target);
    const auto coefficient = target > current ? 0.65f : 0.18f;
    return current + ((target - current) * coefficient);
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
    keyboardPanicButton.onClick = [this] { audioProcessor.getMidiKeyboardState().allNotesOff(0); };
    addAndMakeVisible(keyboardPanicButton);

    keyboardRangeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    keyboardRangeLabel.setJustificationType(juce::Justification::centred);
    keyboardRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(keyboardRangeLabel);
    updateKeyboardRangeLabel();

    configureSectionLabel(homeSectionLabel, "HOME");
    configureSectionLabel(homeEngineLabel, "SOURCE");
    configureSectionLabel(homeShapeLabel, "SHAPE");
    configureSectionLabel(homeLabLabel, "MOTION");
    configureSectionLabel(homeLibraryLabel, "LIBRARY");
    configureSectionLabel(synthSectionLabel, "SYNTH");
    configureSectionLabel(randomSectionLabel, "LAB");
    configureSectionLabel(modSectionLabel, "MOD");
    configureSectionLabel(modSourceLabel, "SOURCES");
    configureSectionLabel(modMacroLabel, "MACROS");
    configureSectionLabel(modLfoLabel, "LFO 1");
    configureSectionLabel(modEnvelopeLabel, "MOD ENV 1");
    configureSectionLabel(modMatrixLabel, "ROUTING");
    configureSectionLabel(sampleSectionLabel, "SAMPLE");
    configureSectionLabel(sequencerSectionLabel, "SEQ");
    configureSectionLabel(futureSectionLabel, "FX");
    configureSectionLabel(librarySectionLabel, "LIBRARY");

    sampleNameLabel.setText("No sample", juce::dontSendNotification);
    sampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    sampleNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(sampleNameLabel);

    presetStatusLabel.setJustificationType(juce::Justification::centredLeft);
    presetStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(presetStatusLabel);

    randomStatusLabel.setText("Ready", juce::dontSendNotification);
    randomStatusLabel.setJustificationType(juce::Justification::centredLeft);
    randomStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(randomStatusLabel);

    fxRackStatusLabel.setJustificationType(juce::Justification::centredLeft);
    fxRackStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(fxRackStatusLabel);

    const std::array<juce::String, 4> modSourceTexts {
        "Tone: cutoff + resonance",
        "Dirt: drive + output trim",
        "Motion: filter env + osc2 tune",
        "Space: delay + reverb sends"
    };

    for (size_t index = 0; index < modSourceRows.size(); ++index)
    {
        auto& label = modSourceRows[index];
        label.setText(modSourceTexts[index], juce::dontSendNotification);
        label.setFont(juce::FontOptions(12.0f));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xffc7d7d4));
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

    presetNameEditor.setTextToShowWhenEmpty("Preset name", juce::Colour(0xff617078));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(presetNameEditor);

    waveformBox.addItemList(Parameters::waveformChoices(), 1);
    addAndMakeVisible(waveformBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::oscWave, waveformBox));

    osc2WaveBox.addItemList(Parameters::waveformChoices(), 1);
    addAndMakeVisible(osc2WaveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::osc2Wave, osc2WaveBox));

    filterModeBox.addItemList(Parameters::filterModeChoices(), 1);
    addAndMakeVisible(filterModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterMode, filterModeBox));

    recipeBox.addItemList(Parameters::randomRecipeChoices(), 1);
    addAndMakeVisible(recipeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomRecipe, recipeBox));

    sequencerRateBox.addItemList(Parameters::sequencerRateChoices(), 1);
    addAndMakeVisible(sequencerRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerRate, sequencerRateBox));

    sequencerGrooveBox.addItemList(Parameters::sequencerGrooveModeChoices(), 1);
    sequencerGrooveBox.setTextWhenNothingSelected("Groove");
    addAndMakeVisible(sequencerGrooveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerGrooveMode, sequencerGrooveBox));

    sequencerPatternBox.addItem("Bass", 1);
    sequencerPatternBox.addItem("Stab", 2);
    sequencerPatternBox.addItem("UKG 2-Step", 3);
    sequencerPatternBox.addItem("Shuffle Bass", 4);
    sequencerPatternBox.addItem("Organ Skank", 5);
    sequencerPatternBox.addItem("Vocal Chop", 6);
    sequencerPatternBox.addItem("Late Stab", 7);
    sequencerPatternBox.setSelectedId(3, juce::dontSendNotification);
    addAndMakeVisible(sequencerPatternBox);

    sampleModeBox.addItem("Gate", 1);
    sampleModeBox.addItem("One Shot", 2);
    sampleModeBox.setTextWhenNothingSelected("Mode");
    addAndMakeVisible(sampleModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::samplePlaybackMode, sampleModeBox));

    sampleStutterRateBox.addItem("1/8", 1);
    sampleStutterRateBox.addItem("1/16", 2);
    sampleStutterRateBox.addItem("1/32", 3);
    sampleStutterRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(sampleStutterRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleStutterRate, sampleStutterRateBox));

    addAndMakeVisible(presetBox);

    presetCategoryBox.addItemList(presetCategoryChoices(), 1);
    presetCategoryBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetCategoryBox);

    presetFilterBox.addItemList(presetFilterChoices(), 1);
    presetFilterBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetFilterBox);

    fxAddBox.addItem("Tone", 1);
    fxAddBox.addItem("EQ", 2);
    fxAddBox.addItem("Drive", 3);
    fxAddBox.addItem("Crush", 4);
    fxAddBox.addItem("Pump", 5);
    fxAddBox.addItem("Tremolo", 6);
    fxAddBox.addItem("Phaser", 7);
    fxAddBox.addItem("Flanger", 8);
    fxAddBox.addItem("Chorus", 9);
    fxAddBox.addItem("Delay", 10);
    fxAddBox.addItem("Reverb", 11);
    fxAddBox.addItem("Width", 12);
    fxAddBox.addItem("Guard", 13);
    fxAddBox.setTextWhenNothingSelected("Add FX");
    addAndMakeVisible(fxAddBox);

    fxPumpRateBox.addItem("1/4", 1);
    fxPumpRateBox.addItem("1/8", 2);
    fxPumpRateBox.addItem("1/8T", 3);
    fxPumpRateBox.addItem("1/16", 4);
    fxPumpRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxPumpRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpRate, fxPumpRateBox));

    fxTremoloRateBox.addItem("1/4", 1);
    fxTremoloRateBox.addItem("1/8", 2);
    fxTremoloRateBox.addItem("1/8T", 3);
    fxTremoloRateBox.addItem("1/16", 4);
    fxTremoloRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxTremoloRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxTremoloRate, fxTremoloRateBox));

    lfo1ShapeBox.addItemList(Parameters::lfoShapeChoices(), 1);
    lfo1ShapeBox.setTextWhenNothingSelected("Shape");
    addAndMakeVisible(lfo1ShapeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Shape, lfo1ShapeBox));

    lfo1SyncRateBox.addItemList(Parameters::lfoSyncRateChoices(), 1);
    lfo1SyncRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(lfo1SyncRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1SyncRate, lfo1SyncRateBox));

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

    fxChorusEnabledButton.setButtonText("Chorus");
    addAndMakeVisible(fxChorusEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxChorusEnabled, fxChorusEnabledButton));

    fxDelayEnabledButton.setButtonText("Delay");
    addAndMakeVisible(fxDelayEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelayEnabled, fxDelayEnabledButton));

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
    configureSlider(unisonVoicesSlider, unisonVoicesLabel, "Voices", Parameters::ID::unisonVoices);
    configureSlider(unisonDetuneSlider, unisonDetuneLabel, "Detune", Parameters::ID::unisonDetune);
    configureSlider(unisonBlendSlider, unisonBlendLabel, "Blend", Parameters::ID::unisonBlend);
    configureSlider(unisonSpreadSlider, unisonSpreadLabel, "Spread", Parameters::ID::unisonSpread);
    configureSlider(glideSlider, glideLabel, "Glide", Parameters::ID::glideTime);
    configureSlider(macroToneSlider, macroToneLabel, "Tone", Parameters::ID::macroTone);
    configureSlider(macroDirtSlider, macroDirtLabel, "Dirt", Parameters::ID::macroDirt);
    configureSlider(macroMotionSlider, macroMotionLabel, "Motion", Parameters::ID::macroMotion);
    configureSlider(macroSpaceSlider, macroSpaceLabel, "Space", Parameters::ID::macroSpace);
    configureSlider(lfo1RateSlider, lfo1RateLabel, "Rate", Parameters::ID::lfo1Rate);
    configureSlider(lfo1DepthSlider, lfo1DepthLabel, "Depth", Parameters::ID::lfo1Depth);
    configureSlider(lfo1PhaseSlider, lfo1PhaseLabel, "Phase", Parameters::ID::lfo1Phase);
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

    configureSlider(attackSlider, attackLabel, "Attack", Parameters::ID::ampAttack);
    configureSlider(decaySlider, decayLabel, "Decay", Parameters::ID::ampDecay);
    configureSlider(sustainSlider, sustainLabel, "Sustain", Parameters::ID::ampSustain);
    configureSlider(releaseSlider, releaseLabel, "Release", Parameters::ID::ampRelease);
    configureSlider(cutoffSlider, cutoffLabel, "Cutoff", Parameters::ID::filterCutoff);
    configureSlider(resonanceSlider, resonanceLabel, "Res", Parameters::ID::filterResonance);
    configureSlider(filterEnvSlider, filterEnvLabel, "F Env", Parameters::ID::filterEnvAmount);
    configureSlider(driveSlider, driveLabel, "Drive", Parameters::ID::driveAmount);
    configureSlider(outputSlider, outputLabel, "Output", Parameters::ID::outputGain);
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
        audioProcessor.generateRandomPatch();
        setRandomStatus("Generated");
    };
    mutateButton.onClick = [this]
    {
        audioProcessor.mutateRandomPatch();
        setRandomStatus("Mutated");
    };
    variationButton.onClick = [this]
    {
        audioProcessor.createRandomVariation();
        setRandomStatus("Variation");
    };
    undoRandomButton.onClick = [this]
    {
        setRandomStatus(audioProcessor.undoRandomization() ? "Undo restored" : "Nothing to undo");
        updateSampleNameLabel();
        sequencerGrid.repaint();
    };
    loadSampleButton.onClick = [this] { chooseSampleFile(); };
    clearSampleButton.onClick = [this]
    {
        audioProcessor.clearSample();
        updateSampleNameLabel();
    };
    randomCutButton.onClick = [this]
    {
        setRandomStatus(audioProcessor.randomizeSampleCut() ? "Sample randomized" : "Sample skipped");
    };
    ukgChopButton.onClick = [this]
    {
        if (audioProcessor.randomizeUkgVocalChop())
        {
            sequencerGrid.repaint();
            setRandomStatus("UKG chop ready");
        }
        else
        {
            setRandomStatus("UKG chop skipped");
        }
    };
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
    refreshPresetsButton.onClick = [this] { refreshPresetList(); };
    favoritePresetButton.onClick = [this] { toggleFavoritePreset(); };
    presetFilterBox.onChange = [this] { refreshPresetList(); };
    presetBox.onChange = [this] { updateFavoritePresetButton(); };
    fxAddBox.onChange = [this]
    {
        const auto selectedId = fxAddBox.getSelectedId();
        if (selectedId > 0)
            addFxModule(static_cast<FxModule>(selectedId - 1));
    };
    fxRemoveButton.onClick = [this] { removeSelectedFxModule(); };
    fxToneSlotButton.onClick = [this] { selectFxModule(FxModule::tone); };
    fxEqSlotButton.onClick = [this] { selectFxModule(FxModule::eq); };
    fxDistortionSlotButton.onClick = [this] { selectFxModule(FxModule::distortion); };
    fxBitcrushSlotButton.onClick = [this] { selectFxModule(FxModule::bitcrush); };
    fxPumpSlotButton.onClick = [this] { selectFxModule(FxModule::pump); };
    fxTremoloSlotButton.onClick = [this] { selectFxModule(FxModule::tremolo); };
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
    addAndMakeVisible(undoRandomButton);
    addAndMakeVisible(loadSampleButton);
    addAndMakeVisible(clearSampleButton);
    addAndMakeVisible(randomCutButton);
    addAndMakeVisible(ukgChopButton);
    addAndMakeVisible(randomSequencerButton);
    addAndMakeVisible(clearSequencerButton);
    addAndMakeVisible(bassPatternButton);
    addAndMakeVisible(stabPatternButton);
    addAndMakeVisible(ukgPatternButton);
    addAndMakeVisible(applyPatternButton);
    addAndMakeVisible(copySequencerButton);
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
    addAndMakeVisible(refreshPresetsButton);
    addAndMakeVisible(favoritePresetButton);
    addAndMakeVisible(fxRemoveButton);
    addAndMakeVisible(fxToneSlotButton);
    addAndMakeVisible(fxEqSlotButton);
    addAndMakeVisible(fxDistortionSlotButton);
    addAndMakeVisible(fxBitcrushSlotButton);
    addAndMakeVisible(fxPumpSlotButton);
    addAndMakeVisible(fxTremoloSlotButton);
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
    stopTimer();
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
        auto topRow = homeContent.removeFromTop(260);
        auto engineArea = topRow.removeFromLeft(360).reduced(5);
        auto shapeArea = topRow.reduced(5);
        auto bottomRow = homeContent.withTrimmedTop(16);
        auto labArea = bottomRow.removeFromLeft(360).reduced(5);
        auto libraryArea = bottomRow.reduced(5);

        for (auto area : { engineArea, shapeArea, labArea, libraryArea })
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
        auto topRow = modContent.removeFromTop(132);
        auto sourceArea = topRow.removeFromLeft(300).reduced(5);
        auto macroArea = topRow.reduced(5);
        auto controlsRow = modContent.withTrimmedTop(12).removeFromTop(138);
        auto lfoArea = controlsRow.removeFromLeft(410).reduced(5);
        auto envelopeArea = controlsRow.reduced(5);
        auto matrixArea = modContent.withTrimmedTop(164).reduced(5);

        for (auto area : { sourceArea, macroArea, lfoArea, envelopeArea, matrixArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::effects)
    {
        auto fxContent = contentArea.reduced(18).withTrimmedTop(36);
        fxContent.removeFromTop(48);
        fxContent.removeFromTop(10);
        auto rackArea = fxContent.removeFromLeft(220).reduced(5);
        auto detailArea = fxContent.reduced(5);

        for (auto area : { rackArea, detailArea })
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
            monoButton.setVisible(true);
            setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, true);
            setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, true);
            setSliderVisible(glideSlider, glideLabel, true);
            recipeBox.setVisible(true);
            generateButton.setVisible(true);
            mutateButton.setVisible(true);
            variationButton.setVisible(true);
            presetBox.setVisible(true);
            presetCategoryBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            presetNameEditor.setVisible(true);
            savePresetButton.setVisible(true);
            presetStatusLabel.setVisible(true);

            homeSectionLabel.setBounds(content.removeFromTop(28));
            auto dashboard = content.withTrimmedTop(8);
            auto topRow = dashboard.removeFromTop(260);
            auto engineArea = topRow.removeFromLeft(360).reduced(18, 12);
            auto shapeArea = topRow.reduced(18, 12);
            auto bottomRow = dashboard.withTrimmedTop(16);
            auto labArea = bottomRow.removeFromLeft(360).reduced(18, 12);
            auto libraryArea = bottomRow.reduced(18, 12);

            homeEngineLabel.setBounds(engineArea.removeFromTop(24));
            auto waveRow = engineArea.removeFromTop(36);
            const auto waveButtonWidth = waveRow.getWidth() / 4;
            sineWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            sawWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            squareWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            triangleWaveButton.setBounds(waveRow.reduced(3, 4));

            auto osc2Row = engineArea.removeFromTop(36).withTrimmedTop(3);
            const auto osc2WaveButtonWidth = osc2Row.getWidth() / 4;
            osc2SineWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SawWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SquareWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2TriangleWaveButton.setBounds(osc2Row.reduced(3, 4));

            auto filterRow = engineArea.removeFromTop(36).withTrimmedTop(3);
            const auto filterButtonWidth = filterRow.getWidth() / 3;
            lowpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            bandpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            highpassFilterButton.setBounds(filterRow.reduced(3, 4));
            monoButton.setBounds(engineArea.removeFromTop(32).reduced(3, 3));
            layoutKnobRow(engineArea.removeFromTop(68).withTrimmedTop(2), { &unisonVoicesSlider, &unisonSpreadSlider, &glideSlider });

            homeShapeLabel.setBounds(shapeArea.removeFromTop(24));
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            layoutKnobRow(shapeArea.removeFromTop(116).withTrimmedTop(4), { &subLevelSlider, &cutoffSlider, &driveSlider, &outputSlider });

            homeLabLabel.setBounds(labArea.removeFromTop(24));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroMotionSlider, macroMotionLabel, true);
            setSliderVisible(macroSpaceSlider, macroSpaceLabel, true);
            layoutKnobRow(labArea.removeFromTop(95).withTrimmedTop(4), { &macroToneSlider, &macroDirtSlider, &macroMotionSlider, &macroSpaceSlider });
            recipeBox.setBounds(labArea.removeFromTop(38).reduced(3, 4));
            auto labButtonRow = labArea.removeFromTop(36).withTrimmedTop(4);
            generateButton.setBounds(labButtonRow.removeFromLeft(110).reduced(3, 4));
            mutateButton.setBounds(labButtonRow.removeFromLeft(100).reduced(3, 4));
            variationButton.setBounds(labButtonRow.removeFromLeft(112).reduced(3, 4));

            homeLibraryLabel.setBounds(libraryArea.removeFromTop(24));
            auto loadRow = libraryArea.removeFromTop(42);
            previousPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            presetBox.setBounds(loadRow.removeFromLeft(220).reduced(3, 4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            loadPresetButton.setBounds(loadRow.removeFromLeft(78).reduced(3, 4));
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
            auto rowOne = content.removeFromTop(130);
            layoutKnobRow(rowOne, { &osc1LevelSlider, &osc2LevelSlider, &subLevelSlider, &noiseLevelSlider, &octaveSlider, &tuneSlider, &osc2OctaveSlider, &osc2TuneSlider });
            auto rowTwo = content.removeFromTop(130).withTrimmedTop(8);
            layoutKnobRow(rowTwo, { &unisonVoicesSlider, &unisonDetuneSlider, &unisonBlendSlider, &unisonSpreadSlider, &cutoffSlider, &resonanceSlider, &filterEnvSlider, &driveSlider, &outputSlider });
            auto rowThree = content.removeFromTop(125).withTrimmedTop(12);
            layoutKnobRow(rowThree, { &glideSlider, &attackSlider, &decaySlider, &sustainSlider, &releaseSlider });
            break;
        }

        case Panel::lab:
        {
            randomSectionLabel.setVisible(true);
            recipeBox.setVisible(true);
            generateButton.setVisible(true);
            mutateButton.setVisible(true);
            variationButton.setVisible(true);
            undoRandomButton.setVisible(true);
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
            recipeBox.setBounds(actionRow.removeFromLeft(230).reduced(4));
            generateButton.setBounds(actionRow.removeFromLeft(120).reduced(4));
            mutateButton.setBounds(actionRow.removeFromLeft(120).reduced(4));
            variationButton.setBounds(actionRow.removeFromLeft(120).reduced(4));
            undoRandomButton.setBounds(actionRow.removeFromLeft(92).reduced(4));
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
            lfo1SyncButton.setVisible(true);
            lfo1RetriggerButton.setVisible(true);
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
            }

            auto modContent = content.withTrimmedTop(8);
            auto topRow = modContent.removeFromTop(132);
            auto sourceArea = topRow.removeFromLeft(300).reduced(18, 12);
            auto macroArea = topRow.reduced(18, 12);

            modSourceLabel.setBounds(sourceArea.removeFromTop(24));
            for (auto& label : modSourceRows)
                label.setBounds(sourceArea.removeFromTop(24).reduced(3, 2));

            modMacroLabel.setBounds(macroArea.removeFromTop(24));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroMotionSlider, macroMotionLabel, true);
            setSliderVisible(macroSpaceSlider, macroSpaceLabel, true);
            layoutKnobRow(macroArea.removeFromTop(92).withTrimmedTop(2), { &macroToneSlider, &macroDirtSlider, &macroMotionSlider, &macroSpaceSlider });

            modContent.removeFromTop(12);
            auto generatorRow = modContent.removeFromTop(138);
            auto lfoArea = generatorRow.removeFromLeft(410).reduced(18, 12);
            auto envelopeArea = generatorRow.reduced(18, 12);

            modLfoLabel.setBounds(lfoArea.removeFromTop(24));
            auto lfoModeRow = lfoArea.removeFromTop(36).withTrimmedTop(3);
            lfo1ShapeBox.setBounds(lfoModeRow.removeFromLeft(118).reduced(3, 4));
            lfo1SyncRateBox.setBounds(lfoModeRow.removeFromLeft(98).reduced(3, 4));
            lfo1SyncButton.setBounds(lfoModeRow.removeFromLeft(72).reduced(3, 4));
            lfo1RetriggerButton.setBounds(lfoModeRow.removeFromLeft(84).reduced(3, 4));
            setSliderVisible(lfo1RateSlider, lfo1RateLabel, true);
            setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, true);
            setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, true);
            layoutKnobRow(lfoArea.removeFromTop(72).withTrimmedTop(4), { &lfo1RateSlider, &lfo1DepthSlider, &lfo1PhaseSlider });

            modEnvelopeLabel.setBounds(envelopeArea.removeFromTop(24));
            setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, true);
            setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, true);
            setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, true);
            setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, true);
            setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, true);
            layoutKnobRow(envelopeArea.removeFromTop(102).withTrimmedTop(6), {
                &modEnv1AttackSlider,
                &modEnv1DecaySlider,
                &modEnv1SustainSlider,
                &modEnv1ReleaseSlider,
                &modEnv1DepthSlider
            });

            modContent.removeFromTop(12);
            auto matrixArea = modContent.reduced(18, 10);
            modMatrixLabel.setBounds(matrixArea.removeFromTop(26));

            for (size_t index = 0; index < modSlotRows.size(); ++index)
            {
                auto row = matrixArea.removeFromTop(25).reduced(3, 1);
                modSlotRows[index].setBounds(row.removeFromLeft(32).reduced(2, 0));
                modSourceBoxes[index].setBounds(row.removeFromLeft(142).reduced(3, 1));
                modDestinationBoxes[index].setBounds(row.removeFromLeft(190).reduced(3, 1));
                modAmountSliders[index].setBounds(row.removeFromLeft(260).reduced(3, 1));
            }

            break;
        }

        case Panel::sample:
        {
            sampleSectionLabel.setVisible(true);
            loadSampleButton.setVisible(true);
            clearSampleButton.setVisible(true);
            randomCutButton.setVisible(true);
            ukgChopButton.setVisible(true);
            sampleEnabledButton.setVisible(true);
            sampleReverseButton.setVisible(true);
            sampleModeBox.setVisible(true);
            sampleStutterEnabledButton.setVisible(true);
            sampleStutterRateBox.setVisible(true);
            sampleNameLabel.setVisible(true);
            sampleSectionLabel.setBounds(content.removeFromTop(28));
            auto actionRow = content.removeFromTop(48);
            loadSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            clearSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            randomCutButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            ukgChopButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            sampleEnabledButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleReverseButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleModeBox.setBounds(actionRow.removeFromLeft(112).reduced(4));
            auto stutterRow = content.removeFromTop(42).withTrimmedTop(4);
            sampleNameLabel.setBounds(stutterRow.removeFromLeft(430).reduced(8, 4));
            sampleStutterEnabledButton.setBounds(stutterRow.removeFromLeft(96).reduced(4));
            sampleStutterRateBox.setBounds(stutterRow.removeFromLeft(108).reduced(4));
            auto cutRow = content.removeFromTop(70).withTrimmedTop(18);
            setSliderVisible(sampleStartSlider, sampleStartLabel, true);
            setSliderVisible(sampleEndSlider, sampleEndLabel, true);
            sampleStartSlider.setBounds(cutRow.removeFromLeft(cutRow.getWidth() / 2).reduced(48, 6));
            sampleEndSlider.setBounds(cutRow.reduced(48, 6));
            content.removeFromTop(20);
            setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, true);
            setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, true);
            setSliderVisible(sampleGainSlider, sampleGainLabel, true);
            setSliderVisible(sampleMixSlider, sampleMixLabel, true);
            setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, true);
            layoutKnobRow(content.removeFromTop(105), { &sampleTransposeSlider, &sampleGainSlider, &sampleMixSlider });
            layoutKnobRow(content.removeFromTop(105).withTrimmedTop(8), { &samplePitchRampSlider, &sampleStutterRepeatsSlider });
            break;
        }

        case Panel::sequencer:
        {
            sequencerSectionLabel.setVisible(true);
            sequencerEnabledButton.setVisible(true);
            rateEighthButton.setVisible(true);
            rateSixteenthButton.setVisible(true);
            rateThirtySecondButton.setVisible(true);
            sequencerGrooveBox.setVisible(true);
            sequencerPatternBox.setVisible(true);
            applyPatternButton.setVisible(true);
            copySequencerButton.setVisible(true);
            randomSequencerButton.setVisible(true);
            clearSequencerButton.setVisible(true);
            sequencerGrid.setVisible(true);
            sequencerSectionLabel.setBounds(content.removeFromTop(28));
            auto timingRow = content.removeFromTop(44);
            sequencerEnabledButton.setBounds(timingRow.removeFromLeft(70).reduced(4));
            auto rateRow = timingRow.removeFromLeft(168);
            const auto rateButtonWidth = rateRow.getWidth() / 3;
            rateEighthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateSixteenthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateThirtySecondButton.setBounds(rateRow.reduced(3, 4));
            sequencerGrooveBox.setBounds(timingRow.removeFromLeft(150).reduced(4));
            auto patternRow = content.removeFromTop(44).withTrimmedTop(2);
            sequencerPatternBox.setBounds(patternRow.removeFromLeft(190).reduced(4));
            applyPatternButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            copySequencerButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            randomSequencerButton.setBounds(patternRow.removeFromLeft(104).reduced(4));
            clearSequencerButton.setBounds(patternRow.removeFromLeft(86).reduced(4));
            setSliderVisible(sequencerRootSlider, sequencerRootLabel, true);
            setSliderVisible(sequencerGateSlider, sequencerGateLabel, true);
            setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, true);
            setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, true);
            setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, true);
            setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, true);
            setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, true);
            layoutKnobRow(content.removeFromTop(98).withTrimmedTop(6), {
                &sequencerRootSlider,
                &sequencerGateSlider,
                &sequencerSwingSlider,
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

            auto actionRow = content.removeFromTop(48);
            fxAddBox.setVisible(true);
            fxRemoveButton.setVisible(true);
            fxRackStatusLabel.setVisible(true);
            fxAddBox.setBounds(actionRow.removeFromLeft(170).reduced(4));
            fxRemoveButton.setBounds(actionRow.removeFromLeft(86).reduced(4));
            fxRackStatusLabel.setBounds(actionRow.reduced(8, 4));

            content.removeFromTop(10);
            auto rackArea = content.removeFromLeft(220).reduced(18, 16);
            rackArea.removeFromTop(26);
            auto detailArea = content.reduced(24, 18);
            detailArea.removeFromTop(30);

            for (auto module : { FxModule::tone,
                                 FxModule::eq,
                                 FxModule::distortion,
                                 FxModule::bitcrush,
                                 FxModule::pump,
                                 FxModule::tremolo,
                                 FxModule::phaser,
                                 FxModule::flanger,
                                 FxModule::chorus,
                                 FxModule::delay,
                                 FxModule::reverb,
                                 FxModule::width,
                                 FxModule::guard })
            {
                auto& slotButton = fxSlotButton(module);
                const auto isVisible = shouldShowFxModule(module);
                slotButton.setVisible(isVisible);

                if (isVisible)
                {
                    slotButton.setBounds(rackArea.removeFromTop(36).reduced(0, 3));
                    rackArea.removeFromTop(2);
                }
            }

            auto detailHeader = detailArea.removeFromTop(38);
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
                    fxPumpEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    fxPumpRateBox.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxPumpDepthSlider, fxPumpDepthLabel, true);
                    setSliderVisible(fxPumpShapeSlider, fxPumpShapeLabel, true);
                    setSliderVisible(fxPumpPhaseSlider, fxPumpPhaseLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxPumpDepthSlider, &fxPumpShapeSlider, &fxPumpPhaseSlider });
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
                    fxDelayEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
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
            savePresetButton.setVisible(true);
            presetBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            refreshPresetsButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            librarySectionLabel.setBounds(content.removeFromTop(28));
            auto saveRow = content.removeFromTop(48);
            presetCategoryBox.setBounds(saveRow.removeFromLeft(140).reduced(4));
            presetNameEditor.setBounds(saveRow.removeFromLeft(300).reduced(4));
            savePresetButton.setBounds(saveRow.removeFromLeft(90).reduced(4));
            auto loadRow = content.removeFromTop(54).withTrimmedTop(10);
            presetFilterBox.setBounds(loadRow.removeFromLeft(130).reduced(4));
            previousPresetButton.setBounds(loadRow.removeFromLeft(44).reduced(4));
            presetBox.setBounds(loadRow.removeFromLeft(250).reduced(4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(44).reduced(4));
            loadPresetButton.setBounds(loadRow.removeFromLeft(90).reduced(4));
            favoritePresetButton.setBounds(loadRow.removeFromLeft(70).reduced(4));
            refreshPresetsButton.setBounds(loadRow.removeFromLeft(100).reduced(4));
            presetStatusLabel.setBounds(content.removeFromTop(36).reduced(6, 4));
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
    slider.setMouseDragSensitivity(64);
    slider.setVelocityBasedMode(false);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
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
    slider.setMouseDragSensitivity(180);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
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

    const auto cellWidth = area.getWidth() / count;

    for (auto* component : components)
    {
        component->setVisible(true);
        component->setBounds(area.removeFromLeft(cellWidth).reduced(4, 0));
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

void NateVSTAudioProcessorEditor::loadSampleFile(const juce::File& file)
{
    if (audioProcessor.loadSampleFile(file))
        updateSampleNameLabel();
}

void NateVSTAudioProcessorEditor::updateSampleNameLabel()
{
    const auto sampleName = audioProcessor.getLoadedSampleName();
    sampleNameLabel.setText(sampleName.isNotEmpty() ? sampleName : "No sample", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::setRandomStatus(const juce::String& action)
{
    const auto locks = audioProcessor.getActiveRandomizationLockSummary();
    randomStatusLabel.setText(locks.isNotEmpty() ? action + " | Locked: " + locks : action + " | No locks",
                              juce::dontSendNotification);
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
    setPlainParameterValue(fxEnabledParameterID(selectedFxModule), 0.0f);

    if (selectedFxModule != FxModule::guard)
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

void NateVSTAudioProcessorEditor::updateFxRackControls()
{
    if (! shouldShowFxModule(selectedFxModule))
        selectedFxModule = FxModule::guard;

    for (auto module : { FxModule::tone,
                         FxModule::eq,
                         FxModule::distortion,
                         FxModule::bitcrush,
                         FxModule::pump,
                         FxModule::tremolo,
                         FxModule::phaser,
                         FxModule::flanger,
                         FxModule::chorus,
                         FxModule::delay,
                         FxModule::reverb,
                         FxModule::width,
                         FxModule::guard })
    {
        auto& button = fxSlotButton(module);
        const auto isEnabled = isFxModuleEnabled(module);
        button.setButtonText(juce::String(isEnabled ? "On " : "Off ") + fxModuleName(module));
        button.setToggleState(module == selectedFxModule, juce::dontSendNotification);
    }

    fxRackStatusLabel.setText(fxModuleName(selectedFxModule) + " | " + fxModuleSummary(selectedFxModule),
                              juce::dontSendNotification);
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
        case FxModule::pump: return "sync depth shape";
        case FxModule::tremolo: return "sync trem pan";
        case FxModule::phaser: return "rate depth mix";
        case FxModule::flanger: return "short delay feedback";
        case FxModule::chorus: return "rate depth mix";
        case FxModule::delay: return "time feedback mix";
        case FxModule::reverb: return "size damping mix";
        case FxModule::width: return "mono bass width";
        case FxModule::guard: return "push and ceiling";
    }

    return {};
}

juce::TextButton& NateVSTAudioProcessorEditor::fxSlotButton(FxModule module)
{
    switch (module)
    {
        case FxModule::tone: return fxToneSlotButton;
        case FxModule::eq: return fxEqSlotButton;
        case FxModule::distortion: return fxDistortionSlotButton;
        case FxModule::bitcrush: return fxBitcrushSlotButton;
        case FxModule::pump: return fxPumpSlotButton;
        case FxModule::tremolo: return fxTremoloSlotButton;
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

void NateVSTAudioProcessorEditor::setPlainParameterValue(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
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
        &synthSectionLabel, &randomSectionLabel, &modSectionLabel, &modSourceLabel, &modMacroLabel, &modLfoLabel, &modEnvelopeLabel, &modMatrixLabel, &sampleSectionLabel, &sequencerSectionLabel,
        &futureSectionLabel, &librarySectionLabel, &sampleNameLabel, &presetStatusLabel, &randomStatusLabel,
        &waveformBox, &osc2WaveBox, &filterModeBox, &recipeBox, &sequencerRateBox, &sequencerGrooveBox, &sequencerPatternBox, &sampleModeBox, &sampleStutterRateBox, &presetBox, &presetCategoryBox,
        &presetFilterBox, &fxAddBox, &fxPumpRateBox, &fxTremoloRateBox, &lfo1ShapeBox, &lfo1SyncRateBox,
        &monoButton, &sampleEnabledButton, &sampleReverseButton, &sampleStutterEnabledButton, &sequencerEnabledButton,
        &fxDistortionEnabledButton, &fxBitcrushEnabledButton, &fxPumpEnabledButton, &fxTremoloEnabledButton, &fxChorusEnabledButton, &fxDelayEnabledButton, &fxReverbEnabledButton, &fxWidthEnabledButton,
        &fxToneEnabledButton, &fxEqEnabledButton, &fxPhaserEnabledButton, &fxGuardEnabledButton,
        &fxFlangerEnabledButton,
        &randomLockPitchButton, &randomLockEnvelopeButton, &randomLockFilterButton, &randomLockSourceButton,
        &randomLockSampleButton, &randomLockFxButton, &randomLockOutputButton, &randomLockSequencerButton,
        &lfo1SyncButton, &lfo1RetriggerButton,
        &generateButton, &mutateButton, &variationButton, &undoRandomButton, &loadSampleButton, &clearSampleButton,
        &randomCutButton, &ukgChopButton, &randomSequencerButton, &clearSequencerButton,
        &bassPatternButton, &stabPatternButton, &ukgPatternButton, &applyPatternButton, &copySequencerButton,
        &sineWaveButton, &sawWaveButton, &squareWaveButton, &triangleWaveButton,
        &osc2SineWaveButton, &osc2SawWaveButton, &osc2SquareWaveButton, &osc2TriangleWaveButton,
        &lowpassFilterButton, &bandpassFilterButton, &highpassFilterButton,
        &rateEighthButton, &rateSixteenthButton, &rateThirtySecondButton,
        &previousPresetButton, &nextPresetButton,
        &savePresetButton, &loadPresetButton, &refreshPresetsButton, &favoritePresetButton,
        &fxRemoveButton, &fxToneSlotButton, &fxEqSlotButton, &fxDistortionSlotButton, &fxBitcrushSlotButton, &fxPumpSlotButton, &fxTremoloSlotButton, &fxPhaserSlotButton, &fxFlangerSlotButton, &fxChorusSlotButton,
        &fxDelaySlotButton, &fxReverbSlotButton, &fxWidthSlotButton, &fxGuardSlotButton,
        &presetNameEditor, &fxRackStatusLabel,
        &sequencerGrid
    });

    for (auto& label : modSourceRows)
        label.setVisible(false);

    for (size_t index = 0; index < modSlotRows.size(); ++index)
    {
        modSlotRows[index].setVisible(false);
        modSourceBoxes[index].setVisible(false);
        modDestinationBoxes[index].setVisible(false);
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
    setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, false);
    setSliderVisible(unisonDetuneSlider, unisonDetuneLabel, false);
    setSliderVisible(unisonBlendSlider, unisonBlendLabel, false);
    setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, false);
    setSliderVisible(glideSlider, glideLabel, false);
    setSliderVisible(macroToneSlider, macroToneLabel, false);
    setSliderVisible(macroDirtSlider, macroDirtLabel, false);
    setSliderVisible(macroMotionSlider, macroMotionLabel, false);
    setSliderVisible(macroSpaceSlider, macroSpaceLabel, false);
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

void NateVSTAudioProcessorEditor::timerCallback()
{
    updateSegmentedSelectors();
    updateOutputMeter();
    updateKeyboardRangeLabel();
    updateFxRackControls();
}

void NateVSTAudioProcessorEditor::refreshPresetList()
{
    const auto previousSelection = presetBox.getText();
    presetBox.clear(juce::dontSendNotification);

    const auto library = audioProcessor.getPresetLibrary();
    const auto recentNames = audioProcessor.getRecentPresetNames();
    auto filter = presetFilterBox.getText().trim();
    if (filter.isEmpty())
        filter = "All";

    auto nextItemId = 1;
    auto addPreset = [this, &nextItemId] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        presetBox.addItem(preset.name, nextItemId++);
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
                addPreset(*preset);
    }
    else
    {
        for (const auto& preset : library)
        {
            const auto matchesFilter = filter == "All"
                || (filter == "Favorites" && preset.isFavorite)
                || (filter == "User" && ! preset.isFactory)
                || (filter == "Factory" && preset.isFactory)
                || preset.category.equalsIgnoreCase(filter);

            if (matchesFilter)
                addPreset(preset);
        }
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

    presetStatusLabel.setText(juce::String(presetBox.getNumItems()) + " presets | Filter: " + filter
                                  + " | User: " + audioProcessor.getPresetDirectory().getFullPathName(),
                              juce::dontSendNotification);
    updateFavoritePresetButton();
}

void NateVSTAudioProcessorEditor::saveCurrentPreset()
{
    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
        presetName = presetBox.getText().trim();

    const auto category = presetCategoryBox.getText().trim();
    if (audioProcessor.savePreset(presetName, category))
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

void NateVSTAudioProcessorEditor::updateFavoritePresetButton()
{
    const auto presetName = presetBox.getText().trim();
    favoritePresetButton.setEnabled(presetName.isNotEmpty());
    favoritePresetButton.setToggleState(presetName.isNotEmpty() && audioProcessor.isPresetFavorite(presetName),
                                        juce::dontSendNotification);
}
