# plugkit-dsp: plain C++20, no JUCE. The pieces of eq1's Engine that a plugin's own Engine shares, copied from eq1
# (eq1 at dbb1127) into namespace plugkit; each file is listed in eq1#231.
add_library(plugkit-dsp STATIC
    src/MatchedDesign.cpp
    src/ShapeDesign.cpp
)
add_library(plugkit::dsp ALIAS plugkit-dsp)

target_include_directories(plugkit-dsp PUBLIC include)
target_compile_features(plugkit-dsp PUBLIC cxx_std_20)
target_compile_options(plugkit-dsp PRIVATE ${plugkit_compile_options})

# eq1's Engine's warnings (eq1's engine/CMakeLists.txt).
if(MSVC)
    target_compile_options(plugkit-dsp PRIVATE /W4)
else()
    target_compile_options(plugkit-dsp PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()
