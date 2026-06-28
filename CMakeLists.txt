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
        Source/Randomization/Randomizer.cpp
        Source/Sampler/SamplePlayer.cpp
        Source/Sequencer/PatternSequencer.cpp
        Source/Effects/EffectsRack.cpp
        Source/UI/FilterResponseDisplay.cpp
        Source/UI/FxRackRow.cpp
        Source/UI/HomeOverviewDisplay.cpp
        Source/UI/HomeSessionDisplay.cpp
        Source/UI/LookAndFeel.cpp
        Source/UI/LowEndAssistant.cpp
        Source/UI/ModCurveDisplay.cpp
        Source/UI/ModMatrixRow.cpp
        Source/UI/ModRouteMapDisplay.cpp
        Source/UI/ModSourceMeter.cpp
        Source/UI/OutputMeter.cpp
        Source/UI/OutputOscilloscopeDisplay.cpp
        Source/UI/OutputSpectrumDisplay.cpp
        Source/UI/PresetLibrarySummary.cpp
        Source/UI/PresetSaveSummary.cpp
        Source/UI/PumpCurveDisplay.cpp
        Source/UI/SampleWaveformDisplay.cpp
        Source/UI/StereoFieldDisplay.cpp
        Source/UI/StepSequencerGrid.cpp
        Source/UI/WavetableDisplay.cpp
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
