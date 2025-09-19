include(FetchContent)

set(JUCE_GIT_TAG "7.0.12" CACHE STRING "JUCE version to fetch")
set(VST3_SDK_GIT_TAG "v3.7.11_build_19" CACHE STRING "VST3 SDK version to fetch")

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${JUCE_GIT_TAG}
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG ${VST3_SDK_GIT_TAG}
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce vst3sdk)

set(JUCE_VST3_PATH ${vst3sdk_SOURCE_DIR} CACHE PATH "Path to the downloaded VST3 SDK" FORCE)
set(VST3_SDK_DIR ${vst3sdk_SOURCE_DIR} CACHE PATH "Alias for the VST3 SDK path" FORCE)
