# As eq1's tests/CMakeLists.txt: Catch2 at the same version, one executable, ctest labels from the tests' tags.
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(Catch2)
add_compile_options(${plugkit_compile_options})

include(Catch)

# One source per line, sorted, so tickets adding tests in parallel merge cleanly.
add_executable(plugkit_dsp_tests
    AllocationGuardTest.cpp
    BellResponseTest.cpp
    BellStabilityTest.cpp
    Double2Test.cpp
    LatestValueTest.cpp
    NoSubnormalsTest.cpp
    ShapeResponseTest.cpp
    SmootherTest.cpp
    StereoPlacementTest.cpp
)
target_link_libraries(plugkit_dsp_tests PRIVATE plugkit::dsp plugkit::test_support Catch2::Catch2WithMain)
catch_discover_tests(plugkit_dsp_tests ADD_TAGS_AS_LABELS)

# The CMake script NoJuce.cmake runs as a test: plugkit-dsp stays plain C++.
add_test(NAME dsp_has_no_juce
    COMMAND ${CMAKE_COMMAND} "-DDSP_DIR=${PROJECT_SOURCE_DIR}/dsp"
        "-DLINKS=$<TARGET_PROPERTY:plugkit-dsp,LINK_LIBRARIES>;$<TARGET_PROPERTY:plugkit-dsp,INTERFACE_LINK_LIBRARIES>"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/NoJuce.cmake)

# plugkit-ui's tests, in a plain executable compiling JUCE and plugkit-ui as a plugin's tests do. One source per line,
# as above.
add_executable(plugkit_plugin_tests
    ui/StapleTest.cpp
    ui/ValueTextTest.cpp
)
target_compile_definitions(plugkit_plugin_tests PRIVATE
    JUCE_STANDALONE_APPLICATION=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 PLUGKIT_HARNESS_VAR_PREFIX="PLUGKIT_")
target_link_libraries(plugkit_plugin_tests PRIVATE
    plugkit::ui
    plugkit::editor_harness
    Catch2::Catch2WithMain
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
catch_discover_tests(plugkit_plugin_tests)
