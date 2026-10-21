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
