cmake_minimum_required(VERSION 3.22)
project(DistortionPack VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS min")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

function(add_distortion_plugin target code)
    juce_add_plugin(${target}
        PRODUCT_NAME "${target}"
        COMPANY_NAME "VeniceAudio"
        PLUGIN_MANUFACTURER_CODE Vnca
        PLUGIN_CODE ${code}
        FORMATS VST3 AU Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES "Fx Distortion"
        AU_MAIN_TYPE "kAudioUnitType_Effect")
    target_sources(${target} PRIVATE Source/${target}.cpp)
    target_include_directories(${target} PRIVATE Source)
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endfunction()

add_distortion_plugin(TubeSaturator  Tubs)
add_distortion_plugin(FuzzMachine    Fuzz)
add_distortion_plugin(BitCrusher     Btcr)
add_distortion_plugin(TapeSaturation TpSt)
add_distortion_plugin(WaveFolder     WvFd)
