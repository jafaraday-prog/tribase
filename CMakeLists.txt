cmake_minimum_required(VERSION 3.21)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum supported macOS version" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build Universal binaries" FORCE)

project(TriBase VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchDependencies)

add_library(tribase_warnings INTERFACE)
target_compile_options(tribase_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

set(TRIBASE_SHARED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shared")

juce_add_plugin(TriBaseInstrument
    COMPANY_NAME "TriBase"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE TRIB
    PLUGIN_CODE TBIN
    FORMATS AU VST3
    BUNDLE_ID com.tribase.instrument
    PRODUCT_NAME "TriBase Instrument"
    VST3_CATEGORIES "Instrument|Synth"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

juce_generate_juce_header(TriBaseInstrument)

set(TriBaseInstrumentSources
    instrument/source/PluginProcessor.cpp
    instrument/source/PluginEditor.cpp
    instrument/source/SynthVoice.cpp
    instrument/source/SynthSound.cpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TriBaseInstrumentSources})

target_sources(TriBaseInstrument PRIVATE ${TriBaseInstrumentSources})
target_sources(TriBaseInstrument PRIVATE
    shared/ui/TriBaseStyle.h
    shared/ui/XenoLookAndFeel.h
    shared/ui/XenoLookAndFeel.cpp
)

target_include_directories(TriBaseInstrument PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${TRIBASE_SHARED_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/instrument/source"
)

target_compile_definitions(TriBaseInstrument PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_SCREEN_CAPTURE_KIT=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TriBaseInstrument
    PRIVATE
        tribase_warnings
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
)

juce_add_plugin(TriBaseBassManager
    COMPANY_NAME "TriBase"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE TRIB
    PLUGIN_CODE TBBM
    FORMATS AU VST3
    BUNDLE_ID com.tribase.bassmanager
    PRODUCT_NAME "TriBase Bass Manager"
    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(TriBaseBassManager)

set(TriBaseBassManagerSources
    effect/source/PluginProcessor.cpp
    effect/source/PluginEditor.cpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TriBaseBassManagerSources})

target_sources(TriBaseBassManager PRIVATE ${TriBaseBassManagerSources})
target_sources(TriBaseBassManager PRIVATE
    shared/ui/TriBaseStyle.h
    shared/ui/XenoLookAndFeel.h
    shared/ui/XenoLookAndFeel.cpp
)
target_sources(TriBaseBassManager PRIVATE shared/dsp/LookaheadDetector.cpp)

target_include_directories(TriBaseBassManager PRIVATE
    "${TRIBASE_SHARED_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/effect/source"
)

target_compile_definitions(TriBaseBassManager PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_SCREEN_CAPTURE_KIT=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TriBaseBassManager
    PRIVATE
        tribase_warnings
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
)

add_subdirectory(kick)
