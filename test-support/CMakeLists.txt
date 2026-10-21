# plugkit::test_support: helpers for a plugin's test executables, kept out of plugkit-dsp. An object library, so a
# test executable that links it always gets AllocationGuard's operator new and delete, which replace the standard
# ones there, whatever the link order.
add_library(plugkit-test-support OBJECT
    src/AllocationGuard.cpp
)
add_library(plugkit::test_support ALIAS plugkit-test-support)

target_include_directories(plugkit-test-support PUBLIC include)
target_compile_features(plugkit-test-support PUBLIC cxx_std_20)
target_compile_options(plugkit-test-support PRIVATE ${plugkit_compile_options})

# plugkit::editor_harness: the editor test harness (plugkit/test/EditorHarness.h), for a plugin's editor tests, with
# plugkit-ui's headers but not its sources, which the plugin's own target compiles. The test target defines
# PLUGKIT_HARNESS_VAR_PREFIX, the plugin's VAR_PREFIX, and links Catch2. Only where plugkit-ui is.
if(TARGET plugkit-ui)
    add_library(plugkit-editor-harness INTERFACE)
    add_library(plugkit::editor_harness ALIAS plugkit-editor-harness)
    target_include_directories(plugkit-editor-harness INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<TARGET_PROPERTY:plugkit-ui,INTERFACE_INCLUDE_DIRECTORIES>)
endif()
