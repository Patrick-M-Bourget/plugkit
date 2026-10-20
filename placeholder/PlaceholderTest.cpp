#include "Placeholder.h"

#include <array>
#include <cstdio>

int main()
{
    constexpr std::array<float, 4> samples { 0.5f, -0.25f, 1.0f, 0.25f };
    const auto got = plugkit::placeholder::sum (samples);
    if (got != 1.5f)
    {
        std::printf ("FAIL: the sum of 0.5, -0.25, 1 and 0.25 is 1.5, got %g\n", static_cast<double> (got));
        return 1;
    }
    std::printf ("The placeholder builds, links and sums\n");
    return 0;
}
