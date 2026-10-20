cmake_minimum_required(VERSION 3.28)

project(plugkit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# A plugin that fetches plugkit (FetchContent_MakeAvailable) gets its targets, not its tests.
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
endif()

add_subdirectory(placeholder)
