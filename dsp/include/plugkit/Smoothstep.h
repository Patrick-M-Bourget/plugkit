#pragma once

// Copied from eq1's engine/src/Smoothstep.h (eq1 at dbb1127); listed in eq1#231.

#include <algorithm>

namespace plugkit
{

// 0 below 0, 1 above 1 and smooth in between: a path from 0 to 1 with no corner at either end.
inline double smoothstep (double x)
{
    x = std::clamp (x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

} // namespace plugkit
