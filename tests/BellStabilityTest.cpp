// Rewritten from eq1's tests/engine/BellStabilityTest.cpp (eq1 at dbb1127), which drives eq1's Engine, against the
// pieces, gliding through the tests' Band model (BandModel.h): the same grid and tolerances. Listed in eq1#231.

#include "BandModel.h"
#include "Measure.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

using namespace plugkit;

TEST_CASE ("Bell is stable across its whole range, including Frequency above Nyquist")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0, 192000.0);
    const double frequency = GENERATE (10.0, 1000.0, 15000.0, 20000.0, 21000.0, 22000.0, 23500.0, 26000.0, 30000.0);
    const double gain = GENERATE (-30.0, -6.0, -0.1, 0.0, 0.1, 6.0, 30.0);
    const double q = GENERATE (0.025, 0.3, 1.0, 5.0, 40.0);

    const auto response = test::impulseResponse (designBand (test::bellBand (frequency, gain, q), sampleRate));

    CAPTURE (sampleRate, frequency, gain, q);
    REQUIRE (std::all_of (response.begin(), response.end(), [] (float s) { return std::isfinite (s); }));
    float tailPeak = 0.0f;
    for (size_t i = response.size() - 4096; i < response.size(); ++i)
        tailPeak = std::max (tailPeak, std::abs (response[i]));
    REQUIRE (tailPeak < 1.0e-4f); // decaying, though a Q 40 boost at 10 Hz rings for seconds
}

TEST_CASE ("Bell stays finite when its settings jump at random between blocks")
{
    const double sampleRate = GENERATE (44100.0, 96000.0);

    test::BandModel bell (sampleRate, 1);

    std::mt19937 random (1234);
    std::uniform_real_distribution<double> unit (0.0, 1.0);
    std::uniform_real_distribution<float> noise (-1.0f, 1.0f);
    std::vector<std::vector<float>> signal { std::vector<float> (64) };
    auto& left = signal[0];

    for (int block = 0; block < 20000; ++block)
    {
        const double frequency = 10.0 * std::pow (3000.0, unit (random));
        const double gain = -30.0 + 60.0 * unit (random);
        const double q = 0.025 * std::pow (1600.0, unit (random));
        bell.setShape (test::bellBand (frequency, gain, q));

        for (auto& s : left)
            s = noise (random);
        bell.process (signal);

        // Random jumps every 64 samples may ring hard, but never blow up.
        const bool bounded = std::all_of (left.begin(), left.end(), [] (float s) { return std::isfinite (s) && std::abs (s) < 1.0e6f; });
        CAPTURE (block, frequency, gain, q);
        REQUIRE (bounded);
    }
}
