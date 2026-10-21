// New in plugkit: eq1 has no test of its own for NoSubnormals (eq1's engine/src/NoSubnormals.h), which its Engine
// holds while it processes.

#include "plugkit/NoSubnormals.h"

#include <catch2/catch_test_macros.hpp>

#include <limits>

using namespace plugkit;

namespace
{
// Half the smallest normal double: a subnormal, unless the floating-point unit flushes it to zero. volatile, so the
// compiler can't work it out at compile time, where the thread's mode doesn't apply.
double halfSmallestNormal()
{
    volatile double smallest = std::numeric_limits<double>::min();
    volatile double half = smallest * 0.5;
    return half;
}
} // namespace

TEST_CASE ("NoSubnormals flushes subnormal results to zero while alive, and restores the thread's mode after")
{
    REQUIRE (halfSmallestNormal() > 0.0);
    {
        const NoSubnormals noSubnormals;
#if PLUGKIT_SUBNORMALS_X86 || PLUGKIT_SUBNORMALS_ARM64
        CHECK (halfSmallestNormal() == 0.0);
#endif
    }
    CHECK (halfSmallestNormal() > 0.0);
}
