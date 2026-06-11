cmake_minimum_required(VERSION 3.24)

project(MultichannelPanoramaVST
    VERSION 0.3.0
    DESCRIPTION "Multichannel 3D panorama VST plugin (VST3 + optional VST2 .dll)"
    LANGUAGES C CXX)

option(MULTICHANNEL_PANORAMA_BUILD_PLUGIN "Build the JUCE VST plugin" ON)
set(MULTICHANNEL_PANORAMA_JUCE_TAG "8.0.13" CACHE STRING "JUCE release tag used by FetchContent")

# Path to a VST2 SDK (a directory that contains pluginterfaces/vst2.x/aeffect.h).
# When supplied, a legacy VST2 ".dll" target is built in addition to VST3. This is
# what FL Studio scans for when it does not pick up the VST3 package. The VST2 SDK
# is NOT bundled in this repository (Steinberg licensing); the CI workflow fetches
# the headers from a public mirror, or you can point this at your own copy.
set(MULTICHANNEL_PANORAMA_VST2_SDK_PATH "" CACHE PATH "Path to a VST2 SDK root (enables the VST2 .dll build)")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT MULTICHANNEL_PANORAMA_BUILD_PLUGIN)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${MULTICHANNEL_PANORAMA_JUCE_TAG}
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(juce)

# Build VST3 by default; add a legacy VST2 ".dll" target when a VST2 SDK is supplied.
set(MULTICHANNEL_PANORAMA_FORMATS VST3)

if(MULTICHANNEL_PANORAMA_VST2_SDK_PATH AND EXISTS "${MULTICHANNEL_PANORAMA_VST2_SDK_PATH}/pluginterfaces/vst2.x/aeffect.h")
    message(STATUS "VST2 SDK found at ${MULTICHANNEL_PANORAMA_VST2_SDK_PATH}; enabling VST2 (.dll) build")
    juce_set_vst2_sdk_path("${MULTICHANNEL_PANORAMA_VST2_SDK_PATH}")
    list(APPEND MULTICHANNEL_PANORAMA_FORMATS VST)
elseif(MULTICHANNEL_PANORAMA_VST2_SDK_PATH)
    message(WARNING "MULTICHANNEL_PANORAMA_VST2_SDK_PATH is set to '${MULTICHANNEL_PANORAMA_VST2_SDK_PATH}' but pluginterfaces/vst2.x/aeffect.h was not found there; building VST3 only")
endif()

juce_add_plugin(MultichannelPanorama
    COMPANY_NAME "Jhon-Crow"
    COMPANY_WEBSITE "https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin"
    BUNDLE_ID "com.jhoncrow.multichannelpanorama"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${CMAKE_BINARY_DIR}/artifacts/vst3"
    VST_COPY_DIR "${CMAKE_BINARY_DIR}/artifacts/vst2"
    PLUGIN_MANUFACTURER_CODE JhCr
    PLUGIN_CODE McPa
    FORMATS ${MULTICHANNEL_PANORAMA_FORMATS}
    PRODUCT_NAME "Multichannel Panorama")

target_sources(MultichannelPanorama
    PRIVATE
        source/PluginProcessor.cpp
        source/PluginEditor.cpp)

target_include_directories(MultichannelPanorama
    PRIVATE
        source)

target_compile_definitions(MultichannelPanorama
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MultichannelPanorama
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
