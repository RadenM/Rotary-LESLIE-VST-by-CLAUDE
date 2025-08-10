# Leslie Rotary VST Plugin - CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

project(LeslieRotaryVST VERSION 1.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE Setup

# Download JUCE (recommended for self-contained builds)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(JUCE)

# Plugin Definition
juce_add_plugin(LeslieRotaryVST
    COMPANY_NAME "Your Company"
    BUNDLE_ID com.RadenM.LeslieRotaryVST
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE LsRo
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Leslie Rotary")

# Generate JuceHeader.h
juce_generate_juce_header(LeslieRotaryVST)

# Source Files
target_sources(LeslieRotaryVST
    PRIVATE
        Source/LeslieRotaryProcessor.h
        Source/LeslieRotaryProcessor.cpp
        Source/LeslieRotaryEditor.h
        Source/LeslieRotaryEditor.cpp)

# Required JUCE modules
target_link_libraries(LeslieRotaryVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Preprocessor definitions
target_compile_definitions(LeslieRotaryVST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

# Additional compiler flags for optimization
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(LeslieRotaryVST PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /arch:AVX2>)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(LeslieRotaryVST PRIVATE
        NOMINMAX=1
        WIN32_LEAN_AND_MEAN=1)
    # After building the VST3 target, copy the full .vst3 bundle to the user's dev folder
    # Note: $<TARGET_FILE_DIR:...> points to the binary inside Contents/x86_64-win.
    # We copy the bundle root by going two directories up and copying the directory.
    add_custom_command(TARGET LeslieRotaryVST_VST3 POST_BUILD
        # Remove existing dev drop to avoid stale/locked files
        COMMAND ${CMAKE_COMMAND} -E remove_directory "E:/Music Production/TheSampleBase/VST/64/Leslie Rotary.vst3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "E:/Music Production/TheSampleBase/VST/64"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:LeslieRotaryVST_VST3>/../.."
            "E:/Music Production/TheSampleBase/VST/64/Leslie Rotary.vst3"
    )
elseif(APPLE)
    set_target_properties(LeslieRotaryVST PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO)
endif()

# Build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Install directories
if(APPLE)
    set(VST3_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
    set(AU_INSTALL_DIR   "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
elseif(WIN32)
    set(VST3_INSTALL_DIR "$ENV{PROGRAMFILES}/Common Files/VST3")
else()
    set(VST3_INSTALL_DIR "$ENV{HOME}/.vst3")
endif()

# Installation rules
install(TARGETS LeslieRotaryVST
    LIBRARY DESTINATION ${VST3_INSTALL_DIR}
    RUNTIME DESTINATION ${VST3_INSTALL_DIR})

# Helper target
add_custom_target(install_plugin
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    COMMENT "Installing plugin to system directories")

# Build info
message(STATUS "Leslie Rotary VST Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Plugin Formats: AU VST3 Standalone")
message(STATUS "  Install Directory (VST3): ${VST3_INSTALL_DIR}")

# Options
option(LESLIE_BUILD_TESTS "Build unit tests" OFF)
option(LESLIE_ENABLE_PROFILING "Enable performance profiling" OFF)

if(LESLIE_ENABLE_PROFILING)
    target_compile_definitions(LeslieRotaryVST PRIVATE LESLIE_ENABLE_PROFILING=1)
endif()
