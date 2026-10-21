#pragma once

// The tests' shared helpers, copied from eq1's tests/engine/Measure.h (eq1 at dbb1127); listed in eq1#231. What the
// pieces are driven with (every Shape, seeded noise) and how what comes out is measured. eq1's
// helpers drive its Engine; here impulseResponse runs a Cascade as eq1's Band does, and the Engine's scenes are left
// out.

#include "plugkit/BandShape.h"
#include "plugkit/Biquad.h"
#include "plugkit/ShapeDesign.h"

#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>
#include <random>
#include <vector>

namespace plugkit::test
{

// Every Shape, in its enum's order.
inline constexpr std::array allShapes = { Shape::Bell,  Shape::LowShelf, Shape::LowCut,    Shape::HighShelf, Shape::HighCut,
                                          Shape::Notch, Shape::BandPass, Shape::TiltShelf, Shape::FlatTilt,  Shape::AllPass };

// White noise between -1 and 1: the same sequence for the same seed, so a test repeats exactly.
class Noise
{
public:
    explicit Noise (std::mt19937::result_type seed) : random (seed) {}
    float operator()() { return unit (random); }

private:
    std::mt19937 random;
    std::uniform_real_distribution<float> unit { -1.0f, 1.0f };
};

inline BandShape bellBand (double frequency, double gain, double q)
{
    return { .shape = Shape::Bell, .gain = gain, .frequency = frequency, .q = q };
}

// Runs samples through a cascade in place, one BiquadState per section, in float between sections as eq1's Band
// does. states holds the sections' states from one call to the next.
inline void runCascade (const Cascade& cascade, std::array<BiquadState, maxSections>& states, std::vector<float>& samples)
{
    for (size_t s = 0; s < static_cast<size_t> (cascade.count); ++s)
        for (auto& sample : samples)
            sample = states[s].process (cascade.sections[s], sample);
}

// Processes a unit impulse through a cascade until the response has decayed. eq1's helper, which drives its Engine,
// with the cascade in place of the Engine: the same block size, length limit and decay test.
inline std::vector<float> impulseResponse (const Cascade& cascade)
{
    constexpr int blockSize = 4096;
    constexpr size_t maxLength = size_t { 1 } << 22;

    std::vector<float> response;
    std::vector<float> block (blockSize);
    std::array<BiquadState, maxSections> states {};
    float peak = 0.0f;

    while (response.size() < maxLength)
    {
        std::fill (block.begin(), block.end(), 0.0f);
        if (response.empty())
            block[0] = 1.0f;

        runCascade (cascade, states, block);
        response.insert (response.end(), block.begin(), block.end());

        float blockPeak = 0.0f;
        for (float s : block)
            blockPeak = std::max (blockPeak, std::abs (s));
        peak = std::max (peak, blockPeak);

        if (response.size() > blockSize && blockPeak < peak * 1.0e-9f)
            break;
    }
    return response;
}

// Magnitude in dB of an impulse response at frequency (Hz), by direct DFT.
inline double magnitudeDb (const std::vector<float>& response, double frequency, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const std::complex<double> step = std::polar (1.0, -w);
    std::complex<double> rotation = 1.0;
    std::complex<double> sum = 0.0;
    for (float s : response)
    {
        sum += static_cast<double> (s) * rotation;
        rotation *= step;
    }
    return 20.0 * std::log10 (std::abs (sum));
}

// Phase in radians, wrapped to +/-pi, of an impulse response at frequency (Hz), by direct DFT.
inline double phase (const std::vector<float>& response, double frequency, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const std::complex<double> step = std::polar (1.0, -w);
    std::complex<double> rotation = 1.0;
    std::complex<double> sum = 0.0;
    for (float s : response)
    {
        sum += static_cast<double> (s) * rotation;
        rotation *= step;
    }
    return std::arg (sum);
}

// Log-spaced frequencies from 10 Hz up to and including Nyquist.
inline std::vector<double> frequenciesUpToNyquist (double sampleRate, int count)
{
    std::vector<double> frequencies;
    const double nyquist = sampleRate / 2.0;
    for (int i = 0; i < count; ++i)
        frequencies.push_back (10.0 * std::pow (nyquist / 10.0, i / double (count - 1)));
    return frequencies;
}

} // namespace plugkit::test
