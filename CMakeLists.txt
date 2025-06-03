cmake_minimum_required(VERSION 3.25)

# Tell cmake we have files in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(PamplejuceVersion)
include(CPM)
include(PamplejuceMacOS)
include(JUCEDefaults)

# project name cannot have spaces, but product can
set(PROJECT_NAME "SlopeOverload")
set(PRODUCT_NAME "Slope Overload")

set(COMPANY_NAME "Icebreaker Audio")

# Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.icebreakeraudio.slopeoverload")

# Options: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 LV2)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

add_subdirectory(JUCE)
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_plugin("${PROJECT_NAME}"
    # Icons for the standalone app
    # ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD FALSE

    # A four-character plugin id, First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE IceB

    # A unique four-character plugin id. This must have at least one upper-case character
    PLUGIN_CODE IASO

    FORMATS "${FORMATS}"

    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}"

    # Is it a synth?
    IS_SYNTH FALSE

    # Does it need MIDI In?
    NEEDS_MIDI_INPUT FALSE

    # VST3 Categories
    VST3_CATEGORIES Fx

    # LV2 URI
    LV2URI "urn://${BUNDLE_ID}"
    )

add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect stereo)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

include(Assets) # BinaryData target

include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Slope Overload"
)

target_link_libraries(SharedCode
    INTERFACE
    Assets
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)
