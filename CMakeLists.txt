cmake_minimum_required(VERSION 3.22)
project(SakuraParadise VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(SKPR_COPY FALSE)
else()
    set(SKPR_COPY TRUE)
endif()

juce_add_plugin(SakuraParadise
    COMPANY_NAME "SakuraHell"
    PLUGIN_MANUFACTURER_CODE Sakr
    PLUGIN_CODE Skpr
    FORMATS VST3 Standalone
    PRODUCT_NAME "SakuraParadise"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${SKPR_COPY})

juce_generate_juce_header(SakuraParadise)

target_sources(SakuraParadise PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SakuraParadise PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SakuraParadise PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
