// Rewritten from eq1's tests/engine/BellResponseTest.cpp (eq1 at dbb1127), which drives eq1's Engine, against
// designBand directly: the same grid and tolerances. Listed in eq1#231.

#include "Measure.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <complex>

using namespace plugkit;

namespace
{

// The analog Bell the Engine must match (eq1's ADR 0001): proportional-Q peaking filter,
// H(s) = (s^2 + s A/Q + 1) / (s^2 + s/(A Q) + 1) with s normalised to Frequency and A = 10^(Gain/40).
double analogBellDb (double f, double frequency, double gain, double q)
{
    const double a = std::pow (10.0, gain / 40.0);
    const std::complex<double> s { 0.0, f / frequency };
    return 20.0 * std::log10 (std::abs ((s * s + s * (a / q) + 1.0) / (s * s + s / (a * q) + 1.0)));
}

// Allowed error as a share of |Gain|. A biquad's response is flat at Nyquist while the analog
// Bell's skirt still slopes there, so the closer the Bell is to Nyquist, the looser the bound (eq1's ADR 0001).
double toleranceDb (double sampleRate, double frequency, double gain)
{
    const double position = frequency / (sampleRate / 2.0);
    const double share = position <= 0.45 ? 0.10 : position <= 0.73 ? 0.20 : 0.35;
    return std::max (0.05, share * std::abs (gain));
}

} // namespace

TEST_CASE ("Bell magnitude response matches the analog Bell up to Nyquist", "[response]")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    const double frequency = GENERATE (10.0, 40.0, 200.0, 1000.0, 4000.0, 8000.0, 10000.0, 13000.0, 16000.0, 18000.0, 20000.0, 24000.0, 30000.0);
    const double gain = GENERATE (-30.0, -12.0, -3.0, 3.0, 12.0, 30.0);
    const double q = GENERATE (0.025, 0.1, 0.7, 2.0, 10.0, 40.0);

    // A Bell above about 0.91 Nyquist is out of a biquad's reach; BellStabilityTest covers it.
    if (frequency > 0.91 * sampleRate / 2.0)
        return;

    const auto response = test::impulseResponse (designBand (test::bellBand (frequency, gain, q), sampleRate));

    for (double f : test::frequenciesUpToNyquist (sampleRate, 64))
    {
        CAPTURE (sampleRate, frequency, gain, q, f);
        const double error = test::magnitudeDb (response, f, sampleRate) - analogBellDb (f, frequency, gain, q);
        REQUIRE (std::abs (error) <= toleranceDb (sampleRate, frequency, gain));
    }
}

TEST_CASE ("Bell hits its Gain at its Frequency and passes DC and Nyquist like the analog Bell")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    const double frequency = GENERATE (100.0, 1000.0, 10000.0, 18000.0);
    const double gain = GENERATE (-18.0, 6.0, 24.0);

    const auto response = test::impulseResponse (designBand (test::bellBand (frequency, gain, 1.0), sampleRate));

    CAPTURE (sampleRate, frequency, gain);
    CHECK (std::abs (test::magnitudeDb (response, frequency, sampleRate) - gain) < 0.01);
    CHECK (std::abs (test::magnitudeDb (response, 0.0, sampleRate)) < 0.01);
    CHECK (std::abs (test::magnitudeDb (response, sampleRate / 2.0, sampleRate)
                     - analogBellDb (sampleRate / 2.0, frequency, gain, 1.0))
           < 0.01);
}
