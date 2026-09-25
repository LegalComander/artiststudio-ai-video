cmake_minimum_required(VERSION 3.22)

project(ArtistStudioStemLab VERSION 0.1.0 LANGUAGES C CXX)

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

juce_add_gui_app(ArtistStudioStemLab
    PRODUCT_NAME "ArtistStudio Stem Lab"
    COMPANY_NAME "ArtistStudio"
    BUNDLE_ID "com.artiststudio.stemlab"
)

juce_generate_juce_header(ArtistStudioStemLab)

target_sources(ArtistStudioStemLab PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/MainComponent.h
    src/TrackAnalyzer.cpp
    src/TrackAnalyzer.h
    src/StemEngine.cpp
    src/StemEngine.h
)

target_compile_definitions(ArtistStudioStemLab PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ArtistStudioStemLab,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(ArtistStudioStemLab PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
