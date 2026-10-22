# plugkit-ui: the JUCE layer a plugin's Plugin Shell shares, copied from eq1's plugin/ (eq1 at dbb1127); each file names
# its eq1 source in its header. Staple, the UI kit, keeps its namespace staple; the rest is plugkit::ui.
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

# Staple's one font, Manrope (SIL Open Font License 1.1, assets/fonts/OFL.txt), bundled so the editor never falls
# back to a system font. As eq1's plugin/CMakeLists.txt, under plugkit's name.
juce_add_binary_data(plugkit-ui-fonts
    NAMESPACE StapleFonts
    HEADER_NAME StapleFonts.h
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/manrope-regular.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/manrope-medium.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/manrope-semibold.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/manrope-bold.ttf)

# An interface library, as JUCE's modules are: its sources compile into each target that links it, with that
# target's JUCE configuration, so a plugin links one copy of JUCE and of plugkit-ui. Link it from one target per
# executable (the plugin's). One source per line, sorted.
add_library(plugkit-ui INTERFACE)
add_library(plugkit::ui ALIAS plugkit-ui)
target_sources(plugkit-ui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/ABCompare.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/Accessibility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/AnalyzerSpectrum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/CompareGroup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/EditHistory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/EditorShell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/EditorTick.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/HeaderBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/KeyboardControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/KeyboardSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/OutputMeter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/OutputPopoverFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/OutputReadout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/PopoverSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/PresetBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/PresetBrowser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/PresetLibrary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/PresetSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/Session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/SetTogether.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/UiScaleMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/UserSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/ValueText.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/display/AnalyzerLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/display/DisplayGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/display/EdgeFadeLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/display/GridLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/Accessibility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/Fonts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/Icons.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/LookAndFeel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/Painting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/TooltipWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/WindowBackground.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/EdgeSelector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/IconButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/Knob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/KnobTooltip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/Overlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/ParseValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/Popover.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/TextChip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/Tween.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugkit/ui/staple/controls/TypeIn.cpp
)
target_include_directories(plugkit-ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(plugkit-ui INTERFACE cxx_std_20)
# juce_dsp for the Analyzer's FFT: a consumer that fetched JUCE has it. plugkit-dsp for the Output Level.
target_link_libraries(plugkit-ui INTERFACE plugkit-ui-fonts plugkit::dsp juce::juce_gui_basics juce::juce_audio_processors juce::juce_dsp juce::juce_data_structures)
