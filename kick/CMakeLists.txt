set(TriBaseKickSources
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
)

juce_add_plugin(TriBaseKick
    COMPANY_NAME "TriBase"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE TRIB
    PLUGIN_CODE TBKK
    FORMATS AU VST3
    BUNDLE_ID com.tribase.kick
    PRODUCT_NAME "TriBase Kick"
    VST3_CATEGORIES "Instrument|Synth"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

juce_generate_juce_header(TriBaseKick)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TriBaseKickSources})

target_sources(TriBaseKick PRIVATE ${TriBaseKickSources})

target_include_directories(TriBaseKick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${TRIBASE_SHARED_INCLUDE_DIR}
)

target_compile_definitions(TriBaseKick PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_SCREEN_CAPTURE_KIT=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TriBaseKick
    PRIVATE
        tribase_warnings
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
)
