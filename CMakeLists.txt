cmake_minimum_required(VERSION 3.22)
project(Formwave VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows: target 8.1 and link the C++ runtime statically so a clean machine needs no redistributable.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(WINVER=0x0603 _WIN32_WINNT=0x0603)
endif()

# ---- audio core tests (no JUCE) ----
option(FORMWAVE_BUILD_TESTS "Build the core tests" ON)
if(FORMWAVE_BUILD_TESTS)
    enable_testing()
    add_executable(core_tests tests/core_tests.cpp)
    target_include_directories(core_tests PRIVATE Source/Core)
    add_test(NAME core_tests COMMAND core_tests ${CMAKE_BINARY_DIR}/demo.wav)
endif()

# ---- plug-in and standalone ----
option(FORMWAVE_BUILD_PLUGIN "Build the VST3 plug-in and standalone app (downloads JUCE)" ON)
if(FORMWAVE_BUILD_PLUGIN)
    include(FetchContent)
    # JUCE 7 is pinned on purpose: JUCE 8 requires Windows 10 1607 or later.
    set(FORMWAVE_JUCE_TAG "7.0.12" CACHE STRING "JUCE git tag")
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG ${FORMWAVE_JUCE_TAG} GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)

    set(FORMATS Standalone VST3)
    if(APPLE)
        list(APPEND FORMATS AU)
    endif()

    juce_add_plugin(Formwave
        COMPANY_NAME "Formwave"
        PLUGIN_MANUFACTURER_CODE Fwav
        PLUGIN_CODE Fw01
        FORMATS ${FORMATS}
        PRODUCT_NAME "Formwave"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice)

    juce_generate_juce_header(Formwave)

    target_sources(Formwave PRIVATE
        Source/Plugin/Processor.cpp
        Source/UI/Editor.cpp)
    target_include_directories(Formwave PRIVATE Source)
    target_compile_definitions(Formwave PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0 JUCE_STRICT_REFCOUNTEDPOINTER=1)
    target_link_libraries(Formwave
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
