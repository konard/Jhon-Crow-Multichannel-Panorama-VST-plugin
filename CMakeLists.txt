cmake_minimum_required(VERSION 3.24)

project(MultichannelPanoramaVST
    VERSION 0.1.0
    DESCRIPTION "Preparation scaffold for a multichannel 3D panorama VST plugin"
    LANGUAGES CXX)

option(MULTICHANNEL_PANORAMA_BUILD_PLUGIN "Build the JUCE VST3 plugin scaffold" ON)
set(MULTICHANNEL_PANORAMA_JUCE_TAG "8.0.13" CACHE STRING "JUCE release tag used by FetchContent")

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
    PLUGIN_MANUFACTURER_CODE JhCr
    PLUGIN_CODE McPa
    FORMATS VST3
    PRODUCT_NAME "Multichannel Panorama")

target_sources(MultichannelPanorama
    PRIVATE
        source/PluginProcessor.cpp)

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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
