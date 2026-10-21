# plugkit-ui: the JUCE layer a plugin's Plugin Shell shares, copied from eq1's plugin/ (eq1 at dbb1127); each file is
# listed in eq1#231. Staple, the UI kit, keeps its namespace staple; the rest is plugkit::ui.
#
# Each plugin pins JUCE. plugkit-ui links the juce:: targets its consumer has already fetched, so the plugin's JUCE
# is the one built, and never fetches JUCE itself when they exist: a plugin fetches JUCE before plugkit. A consumer
# without JUCE (a plugin's Engine-only build) gets no plugkit::ui. plugkit's own build fetches the JUCE eq1 and comp1
# pin, to build and test it.
if(NOT TARGET juce::juce_gui_basics)
    if(NOT PROJECT_IS_TOP_LEVEL)
        message(STATUS "plugkit-ui: no juce:: targets, so no plugkit::ui; fetch JUCE before plugkit to link it")
        return()
    endif()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.3
        GIT_SHALLOW TRUE
        SYSTEM)
    FetchContent_MakeAvailable(JUCE)
endif()

# An interface library, as JUCE's modules are: its sources compile into each target that links it, with that
# target's JUCE configuration, so a plugin links one copy of JUCE and of plugkit-ui. Link it from one target per
# executable (the plugin's). One source per line, sorted.
add_library(plugkit-ui INTERFACE)
add_library(plugkit::ui ALIAS plugkit-ui)
target_sources(plugkit-ui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/ValueText.cpp
)
target_include_directories(plugkit-ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(plugkit-ui INTERFACE cxx_std_20)
target_link_libraries(plugkit-ui INTERFACE juce::juce_gui_basics juce::juce_audio_processors)
