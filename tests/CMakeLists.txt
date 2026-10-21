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
    SmootherTest.cpp
)
target_link_libraries(plugkit_dsp_tests PRIVATE plugkit::dsp Catch2::Catch2WithMain)
catch_discover_tests(plugkit_dsp_tests ADD_TAGS_AS_LABELS)
