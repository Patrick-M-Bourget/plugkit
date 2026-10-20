# A minimal target that proves the toolchain plugkit's CI builds and tests with (scripts/check.sh build and test),
# until plugkit-dsp replaces it.
add_library(plugkit_placeholder STATIC Placeholder.cpp)
target_include_directories(plugkit_placeholder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(PROJECT_IS_TOP_LEVEL)
    add_executable(plugkit_placeholder_tests PlaceholderTest.cpp)
    target_link_libraries(plugkit_placeholder_tests PRIVATE plugkit_placeholder)
    add_test(NAME placeholder COMMAND plugkit_placeholder_tests)
endif()
