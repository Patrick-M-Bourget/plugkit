# Copied from comp1's tests/engine/NoJuce.cmake, for plugkit-dsp.
# Fails when plugkit-dsp's sources include anything of JUCE's or its target links any JUCE library: plugkit-dsp is
# plain C++ (the ctest dsp_has_no_juce, tests/CMakeLists.txt).
# Usage: cmake -DDSP_DIR=<plugkit-dsp's source dir> "-DLINKS=<its target's link libraries>" -P NoJuce.cmake
file(GLOB_RECURSE sources "${DSP_DIR}/*.h" "${DSP_DIR}/*.hpp" "${DSP_DIR}/*.cpp")
set(found "")
foreach(source IN LISTS sources)
    file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"]*[Jj][Uu][Cc][Ee]")
    foreach(line IN LISTS lines)
        string(APPEND found "\n  ${source}: ${line}")
    endforeach()
endforeach()
if(LINKS MATCHES "[Jj][Uu][Cc][Ee]")
    string(APPEND found "\n  the plugkit-dsp target links ${LINKS}")
endif()
if(found)
    message(FATAL_ERROR "plugkit-dsp must not depend on JUCE:${found}")
endif()
list(LENGTH sources count)
message(STATUS "No JUCE in plugkit-dsp's ${count} sources or its links")
