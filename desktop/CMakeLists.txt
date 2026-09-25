cmake_minimum_required(VERSION 3.22)

project(ArtistStudioStemLab VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(STEMLAB_SHARED_SOURCES
    src/MainComponent.cpp
    src/MainComponent.h
    src/TrackAnalyzer.cpp
    src/TrackAnalyzer.h
    src/StemEngine.cpp
    src/StemEngine.h
)

set(STEMLAB_SHARED_LIBRARIES
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_add_gui_app(ArtistStudioStemLab
    PRODUCT_NAME "ArtistStudio Stem Lab"
    COMPANY_NAME "ArtistStudio"
    BUNDLE_ID "com.artiststudio.stemlab"
)

juce_generate_juce_header(ArtistStudioStemLab)

target_sources(ArtistStudioStemLab PRIVATE
    src/Main.cpp
    ${STEMLAB_SHARED_SOURCES}
)

target_compile_definitions(ArtistStudioStemLab PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ArtistStudioStemLab,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(ArtistStudioStemLab PRIVATE
    ${STEMLAB_SHARED_LIBRARIES}
)

juce_add_plugin(ArtistStudioStemLabVST3
    COMPANY_NAME "ArtistStudio"
    BUNDLE_ID "com.artiststudio.stemlab.vst3"
    PLUGIN_MANUFACTURER_CODE ArSt
    PLUGIN_CODE StLb
    FORMATS VST3
    PRODUCT_NAME "ArtistStudio Stem Lab"
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(ArtistStudioStemLabVST3)

target_sources(ArtistStudioStemLabVST3 PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h
    ${STEMLAB_SHARED_SOURCES}
)

target_compile_definitions(ArtistStudioStemLabVST3 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(ArtistStudioStemLabVST3 PRIVATE
    juce::juce_audio_processors
    ${STEMLAB_SHARED_LIBRARIES}
)
