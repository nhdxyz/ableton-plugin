cmake_minimum_required(VERSION 3.22)

project(NateVST VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

enable_testing()

juce_add_plugin(NateVST
    COMPANY_NAME "Nate"
    BUNDLE_ID "com.nate.natevst"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nate
    PLUGIN_CODE NtVs
    FORMATS VST3
    PRODUCT_NAME "Nate VST"
)

target_sources(NateVST
    PRIVATE
        Source/Parameters.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/Synth/Distortion.cpp
        Source/Synth/Envelope.cpp
        Source/Synth/Filter.cpp
        Source/Synth/Oscillator.cpp
        Source/Synth/SynthEngine.cpp
        Source/Synth/Voice.cpp
        Source/Synth/WavetableFrameIO.cpp
        Source/Synth/WavetableFrameRecipes.cpp
        Source/Synth/WavetableModel.cpp
        Source/Randomization/Randomizer.cpp
        Source/Sampler/SamplePlayer.cpp
        Source/Sequencer/PatternSequencer.cpp
        Source/Effects/EffectsRack.cpp
        Source/UI/ClubMonitorDisplay.cpp
        Source/UI/FilterResponseDisplay.cpp
        Source/UI/FocusOverlayPanel.cpp
        Source/UI/FxRackRow.cpp
        Source/UI/HouseLayerRackDisplay.cpp
        Source/UI/HomeOverviewDisplay.cpp
        Source/UI/HomeSignalFlowDisplay.cpp
        Source/UI/HomeSessionDisplay.cpp
        Source/UI/LookAndFeel.cpp
        Source/UI/LowEndAssistant.cpp
        Source/UI/MacroAssignmentPad.cpp
        Source/UI/MacroPerformanceMap.cpp
        Source/UI/ModCurveDisplay.cpp
        Source/UI/ModMatrixRow.cpp
        Source/UI/ModRouteMapDisplay.cpp
        Source/UI/ModSourceMeter.cpp
        Source/UI/OscillatorLaneOverview.cpp
        Source/UI/OutputMeter.cpp
        Source/UI/OutputOscilloscopeDisplay.cpp
        Source/UI/OutputSpectrumDisplay.cpp
        Source/UI/PresetCrateMapDisplay.cpp
        Source/UI/PresetLibrarySummary.cpp
        Source/UI/PresetSaveSummary.cpp
        Source/UI/PumpCurveDisplay.cpp
        Source/UI/RandomMorphPad.cpp
        Source/UI/SampleChopPanel.cpp
        Source/UI/SampleRecorderPanel.cpp
        Source/UI/SampleWaveformDisplay.cpp
        Source/UI/StereoFieldDisplay.cpp
        Source/UI/StepSequencerGrid.cpp
        Source/UI/Theme.cpp
        Source/UI/WavetableDisplay.cpp
        Source/UI/WavetableFrameStrip.cpp
        Source/UI/XYMacroPad.cpp
)

target_compile_definitions(NateVST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NateVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

add_executable(NateVSTLayoutAudit Tests/EditorLayoutAudit.cpp)

target_link_libraries(NateVSTLayoutAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME EditorLayoutAudit COMMAND NateVSTLayoutAudit)

add_executable(NateVSTSampleWaveformDisplayAudit Tests/SampleWaveformDisplayAudit.cpp)

target_link_libraries(NateVSTSampleWaveformDisplayAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SampleWaveformDisplayAudit COMMAND NateVSTSampleWaveformDisplayAudit)

add_executable(NateVSTSampleRecorderAudit Tests/SampleRecorderAudit.cpp)

target_link_libraries(NateVSTSampleRecorderAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SampleRecorderAudit COMMAND NateVSTSampleRecorderAudit)

add_executable(NateVSTWavetableDisplayAudit Tests/WavetableDisplayAudit.cpp)

target_link_libraries(NateVSTWavetableDisplayAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME WavetableDisplayAudit COMMAND NateVSTWavetableDisplayAudit)

add_executable(NateVSTWavetableFrameStripAudit
    Tests/WavetableFrameStripAudit.cpp
    Source/UI/WavetableFrameStrip.cpp
    Source/UI/Theme.cpp
)

target_link_libraries(NateVSTWavetableFrameStripAudit
    PRIVATE
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME WavetableFrameStripAudit COMMAND NateVSTWavetableFrameStripAudit)

add_executable(NateVSTOscillatorLaneOverviewAudit
    Tests/OscillatorLaneOverviewAudit.cpp
    Source/UI/OscillatorLaneOverview.cpp
    Source/UI/Theme.cpp
)

target_link_libraries(NateVSTOscillatorLaneOverviewAudit
    PRIVATE
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME OscillatorLaneOverviewAudit COMMAND NateVSTOscillatorLaneOverviewAudit)

add_executable(NateVSTThemeAudit Tests/ThemeAudit.cpp Source/UI/Theme.cpp)

target_link_libraries(NateVSTThemeAudit
    PRIVATE
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME ThemeAudit COMMAND NateVSTThemeAudit)

add_executable(NateVSTPresetSaveAudit Tests/PresetSaveAudit.cpp)

target_link_libraries(NateVSTPresetSaveAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME PresetSaveAudit COMMAND NateVSTPresetSaveAudit)

add_executable(NateVSTFactoryPresetLibraryAudit Tests/FactoryPresetLibraryAudit.cpp)

target_link_libraries(NateVSTFactoryPresetLibraryAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(NateVSTFactoryPresetLibraryAudit
    PRIVATE
        NATE_VST_FACTORY_PRESETS_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory Presets"
)

add_test(NAME FactoryPresetLibraryAudit COMMAND NateVSTFactoryPresetLibraryAudit)

add_executable(NateVSTFactoryPresetRenderAudit Tests/FactoryPresetRenderAudit.cpp)

target_link_libraries(NateVSTFactoryPresetRenderAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(NateVSTFactoryPresetRenderAudit
    PRIVATE
        NATE_VST_FACTORY_PRESETS_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory Presets"
)

add_test(NAME FactoryPresetRenderAudit COMMAND NateVSTFactoryPresetRenderAudit)

add_executable(NateVSTKeyboardSpamCpuAudit Tests/KeyboardSpamCpuAudit.cpp)

target_link_libraries(NateVSTKeyboardSpamCpuAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME KeyboardSpamCpuAudit COMMAND NateVSTKeyboardSpamCpuAudit)

add_executable(NateVSTProcessorPerformanceAudit Tests/ProcessorPerformanceAudit.cpp)

target_link_libraries(NateVSTProcessorPerformanceAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME ProcessorPerformanceAudit COMMAND NateVSTProcessorPerformanceAudit)

add_executable(NateVSTPresetPreviewAudit Tests/PresetPreviewAudit.cpp)

target_link_libraries(NateVSTPresetPreviewAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(NateVSTPresetPreviewAudit
    PRIVATE
        NATE_VST_FACTORY_PRESETS_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory Presets"
)

add_test(NAME PresetPreviewAudit COMMAND NateVSTPresetPreviewAudit)

add_executable(NateVSTRandomCandidateAudit Tests/RandomCandidateAudit.cpp)

target_link_libraries(NateVSTRandomCandidateAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME RandomCandidateAudit COMMAND NateVSTRandomCandidateAudit)

add_executable(NateVSTGlobalEditHistoryAudit Tests/GlobalEditHistoryAudit.cpp)

target_link_libraries(NateVSTGlobalEditHistoryAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME GlobalEditHistoryAudit COMMAND NateVSTGlobalEditHistoryAudit)

add_executable(NateVSTPumpTelemetryAudit Tests/PumpTelemetryAudit.cpp)

target_link_libraries(NateVSTPumpTelemetryAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME PumpTelemetryAudit COMMAND NateVSTPumpTelemetryAudit)

add_executable(NateVSTGuardTelemetryAudit Tests/GuardTelemetryAudit.cpp)

target_link_libraries(NateVSTGuardTelemetryAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME GuardTelemetryAudit COMMAND NateVSTGuardTelemetryAudit)

add_executable(NateVSTGuardDynamicsAudit Tests/GuardDynamicsAudit.cpp)

target_link_libraries(NateVSTGuardDynamicsAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME GuardDynamicsAudit COMMAND NateVSTGuardDynamicsAudit)

add_executable(NateVSTDriveBassSafetyAudit Tests/DriveBassSafetyAudit.cpp)

target_link_libraries(NateVSTDriveBassSafetyAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME DriveBassSafetyAudit COMMAND NateVSTDriveBassSafetyAudit)

add_executable(NateVSTSequencerPatternSceneAudit Tests/SequencerPatternSceneAudit.cpp)

target_link_libraries(NateVSTSequencerPatternSceneAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SequencerPatternSceneAudit COMMAND NateVSTSequencerPatternSceneAudit)

add_executable(NateVSTSequencerHousePatternAudit Tests/SequencerHousePatternAudit.cpp)

target_link_libraries(NateVSTSequencerHousePatternAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SequencerHousePatternAudit COMMAND NateVSTSequencerHousePatternAudit)

add_executable(NateVSTSequencerRatchetAudit Tests/SequencerRatchetAudit.cpp)

target_link_libraries(NateVSTSequencerRatchetAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SequencerRatchetAudit COMMAND NateVSTSequencerRatchetAudit)

add_executable(NateVSTNoiseSourceAudit Tests/NoiseSourceAudit.cpp)

target_link_libraries(NateVSTNoiseSourceAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME NoiseSourceAudit COMMAND NateVSTNoiseSourceAudit)

add_executable(NateVSTSourceCharacterAudit Tests/SourceCharacterAudit.cpp)

target_link_libraries(NateVSTSourceCharacterAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SourceCharacterAudit COMMAND NateVSTSourceCharacterAudit)

add_executable(NateVSTWavetableModelAudit
    Tests/WavetableModelAudit.cpp
    Source/Synth/WavetableModel.cpp
)

target_link_libraries(NateVSTWavetableModelAudit
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME WavetableModelAudit COMMAND NateVSTWavetableModelAudit)

add_executable(NateVSTWavetableFrameRecipeAudit
    Tests/WavetableFrameRecipeAudit.cpp
    Source/Synth/WavetableFrameRecipes.cpp
    Source/Synth/WavetableModel.cpp
    Source/Synth/Oscillator.cpp
)

target_link_libraries(NateVSTWavetableFrameRecipeAudit
    PRIVATE
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME WavetableFrameRecipeAudit COMMAND NateVSTWavetableFrameRecipeAudit)

add_executable(NateVSTWavetableFrameIOAudit
    Tests/WavetableFrameIOAudit.cpp
    Source/Synth/WavetableFrameIO.cpp
    Source/Synth/WavetableFrameRecipes.cpp
    Source/Synth/WavetableModel.cpp
    Source/Synth/Oscillator.cpp
)

target_link_libraries(NateVSTWavetableFrameIOAudit
    PRIVATE
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME WavetableFrameIOAudit COMMAND NateVSTWavetableFrameIOAudit)

add_executable(NateVSTModulationSourceParityAudit Tests/ModulationSourceParityAudit.cpp)

target_link_libraries(NateVSTModulationSourceParityAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME ModulationSourceParityAudit COMMAND NateVSTModulationSourceParityAudit)

add_executable(NateVSTModulationRouteShapeAudit Tests/ModulationRouteShapeAudit.cpp)

target_link_libraries(NateVSTModulationRouteShapeAudit
    PRIVATE
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME ModulationRouteShapeAudit COMMAND NateVSTModulationRouteShapeAudit)

add_executable(NateVSTSamplerChopAudit Tests/SamplerChopAudit.cpp)

target_link_libraries(NateVSTSamplerChopAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SamplerChopAudit COMMAND NateVSTSamplerChopAudit)

add_executable(NateVSTEffectsSendAudit Tests/EffectsSendAudit.cpp)

target_link_libraries(NateVSTEffectsSendAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME EffectsSendAudit COMMAND NateVSTEffectsSendAudit)

add_executable(NateVSTSpectrumSnapshotAudit Tests/SpectrumSnapshotAudit.cpp)

target_link_libraries(NateVSTSpectrumSnapshotAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME SpectrumSnapshotAudit COMMAND NateVSTSpectrumSnapshotAudit)

add_executable(NateVSTStereoFieldTelemetryAudit Tests/StereoFieldTelemetryAudit.cpp)

target_link_libraries(NateVSTStereoFieldTelemetryAudit
    PRIVATE
        NateVST
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_test(NAME StereoFieldTelemetryAudit COMMAND NateVSTStereoFieldTelemetryAudit)

set(NATE_VST_FACTORY_PRESETS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory Presets")
set(NATE_VST_FACTORY_PRESETS_DEST "$ENV{HOME}/Library/Application Support/Nate VST/Factory Presets")

if(EXISTS "${NATE_VST_FACTORY_PRESETS_SOURCE}")
    add_custom_command(TARGET NateVST POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${NATE_VST_FACTORY_PRESETS_DEST}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${NATE_VST_FACTORY_PRESETS_SOURCE}" "${NATE_VST_FACTORY_PRESETS_DEST}"
        COMMENT "Installing Nate VST factory presets"
    )
endif()

add_custom_target(validate_release
    COMMAND "${CMAKE_COMMAND}" -E env
            "BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_release.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
)

add_custom_target(install_nate_vst3
    COMMAND "${CMAKE_COMMAND}" -E env
            "BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/install_vst3.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
)

add_custom_target(package_release
    COMMAND "${CMAKE_COMMAND}" -E env
            "BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/package_release.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
)
