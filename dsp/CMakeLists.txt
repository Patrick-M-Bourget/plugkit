# plugkit-dsp: plain C++20, no JUCE. The pieces of eq1's Engine that a plugin's own Engine shares, copied from eq1
# (eq1 at dbb1127) into namespace plugkit; each file is listed in eq1#231.
add_library(plugkit-dsp INTERFACE)
add_library(plugkit::dsp ALIAS plugkit-dsp)

target_include_directories(plugkit-dsp INTERFACE include)
target_compile_features(plugkit-dsp INTERFACE cxx_std_20)
