cmake_minimum_required(VERSION 3.28)

# A plugin that fetches plugkit sets its own minimum macOS version; plugkit's own build uses eq1's.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

# C as well as C++: JUCE refuses a project without it.
project(plugkit VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLUGKIT_BUILD_TESTS "Build plugkit's tests (only when plugkit is the top-level project)" ON)

# MSVC refuses an initializer list that mixes designated and positional members (C7556), where clang only warns;
# clang makes it an error too, so a Mac build can't pass where Windows fails. As eq1's root CMakeLists.txt.
set(plugkit_compile_options "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Werror=c99-designator>")

add_subdirectory(dsp)

include(FetchContent)
if(PROJECT_IS_TOP_LEVEL)
    # A .deps folder is shared by every worktree's universal build (scripts/check.sh), dependencies' build folders
    # included, so a build for other architectures fetches into a folder of its own. As eq1's root CMakeLists.txt.
    if(APPLE AND DEFINED FETCHCONTENT_BASE_DIR)
        get_filename_component(plugkit_deps_name "${FETCHCONTENT_BASE_DIR}" NAME)
        if(plugkit_deps_name STREQUAL ".deps" AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64;x86_64")
            message(FATAL_ERROR "FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR} is shared by universal builds, "
                "and this one builds for '${CMAKE_OSX_ARCHITECTURES}'. Configure with "
                "-DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\" (scripts/check.sh does), or with a folder of its own.")
        endif()
    endif()
endif()

add_subdirectory(ui)
add_subdirectory(test-support)

# A plugin that fetches plugkit (FetchContent_MakeAvailable) gets its targets, not its tests.
if(PROJECT_IS_TOP_LEVEL AND PLUGKIT_BUILD_TESTS)
    if(MSVC)
        add_compile_options(/utf-8)
    endif()
    enable_testing()
    add_subdirectory(tests)
endif()
