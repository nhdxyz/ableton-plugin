cmake_minimum_required(VERSION 3.22)

project(Floorform VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Floorform
    COMPANY_NAME "Nate"
    BUNDLE_ID "com.nate.floorform"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nate
    PLUGIN_CODE Flfm
    FORMATS VST3
    PRODUCT_NAME "Floorform"
)

target_sources(Floorform
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
        Source/UI/LookAndFeel.cpp
        Source/UI/StepSequencerGrid.cpp
)

target_compile_definitions(Floorform
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Floorform
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
