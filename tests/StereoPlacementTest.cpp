// Rewritten from eq1's tests/engine/StereoPlacementTest.cpp (eq1 at dbb1127), which drives eq1's Engine, against
// Placement's routing and Mid/Side encode and decode through the tests' Band model (BandModel.h): the same signals and
// tolerances. Listed in eq1#231.

#include "BandModel.h"
#include "Measure.h"

#include "plugkit/Placement.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace plugkit;

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int length = 8192;

std::vector<float> noise (unsigned seed)
{
    std::mt19937 random (seed);
    std::uniform_real_distribution<float> unit (-0.5f, 0.5f);
    std::vector<float> samples (length);
    for (auto& s : samples)
        s = unit (random);
    return samples;
}

// Processes signal in place through a Band placed by placement, with as many channels as signal has: eq1's tests'
// Band, a Bell at 1 kHz, +12 dB, Q 1.
void process (StereoPlacement placement, std::vector<std::vector<float>>& signal)
{
    test::BandModel band (sampleRate, static_cast<int> (signal.size()), placement);
    band.setShape (test::bellBand (1000.0, 12.0, 1.0));
    band.process (signal);
}

// What a Stereo Band does to one channel on its own: the reference for every Placement.
std::vector<float> filtered (std::vector<float> samples)
{
    std::vector<std::vector<float>> mono { std::move (samples) };
    process (StereoPlacement::Stereo, mono);
    return mono[0];
}

void checkClose (const std::vector<float>& actual, const std::vector<float>& expected, float tolerance)
{
    REQUIRE (actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i)
    {
        CAPTURE (i);
        REQUIRE (std::abs (actual[i] - expected[i]) <= tolerance);
    }
}

std::vector<float> combine (const std::vector<float>& a, const std::vector<float>& b, float sign)
{
    std::vector<float> result (a.size());
    for (size_t i = 0; i < a.size(); ++i)
        result[i] = 0.5f * (a[i] + sign * b[i]);
    return result;
}
std::vector<float> midOf (const std::vector<float>& left, const std::vector<float>& right) { return combine (left, right, 1.0f); }
std::vector<float> sideOf (const std::vector<float>& left, const std::vector<float>& right) { return combine (left, right, -1.0f); }
} // namespace

TEST_CASE ("A Stereo Band processes both channels")
{
    const auto left = noise (1), right = noise (2);
    std::vector<std::vector<float>> signal { left, right };
    process (StereoPlacement::Stereo, signal);

    checkClose (signal[0], filtered (left), 1.0e-6f);
    checkClose (signal[1], filtered (right), 1.0e-6f);
}

TEST_CASE ("A Left or Right Band processes only its channel and leaves the other untouched")
{
    const bool leftBand = GENERATE (true, false);
    CAPTURE (leftBand);
    const auto left = noise (1), right = noise (2);
    std::vector<std::vector<float>> signal { left, right };
    process (leftBand ? StereoPlacement::Left : StereoPlacement::Right, signal);

    const auto& processed = signal[leftBand ? 0 : 1];
    const auto& untouched = signal[leftBand ? 1 : 0];
    checkClose (processed, filtered (leftBand ? left : right), 1.0e-6f);
    REQUIRE (untouched == (leftBand ? right : left));
}

TEST_CASE ("A Mid or Side Band processes only its part of the signal and leaves the other unchanged")
{
    const bool midBand = GENERATE (true, false);
    CAPTURE (midBand);
    const auto left = noise (1), right = noise (2);
    std::vector<std::vector<float>> signal { left, right };
    process (midBand ? StereoPlacement::Mid : StereoPlacement::Side, signal);

    const auto mid = midOf (signal[0], signal[1]), side = sideOf (signal[0], signal[1]);
    const auto originalMid = midOf (left, right), originalSide = sideOf (left, right);
    // The untouched part comes back to within float rounding of the encode and decode.
    checkClose (midBand ? mid : side, filtered (midBand ? originalMid : originalSide), 1.0e-6f);
    checkClose (midBand ? side : mid, midBand ? originalSide : originalMid, 1.0e-6f);
}

TEST_CASE ("On a mono track, a Side Band has no effect and every other Stereo Placement processes the signal")
{
    const auto placement = GENERATE (from_range (test::allPlacements));
    CAPTURE (static_cast<int> (placement));
    const auto input = noise (3);
    std::vector<std::vector<float>> signal { input };
    process (placement, signal);

    if (placement == StereoPlacement::Side)
        REQUIRE (signal[0] == input);
    else
        checkClose (signal[0], filtered (input), 1.0e-6f);
}

namespace
{
// Processes signal in place through bands, with each band's shape changed to changed a quarter of the way in, so
// Frequency, Gain and Q glide. Each band runs over the whole stretch in turn: bands placed on different channels
// don't hear each other, so the order doesn't change what they do.
void processWithGlide (std::vector<test::BandModel>& bands, const BandShape& changed, std::vector<std::vector<float>>& signal)
{
    constexpr size_t quarter = length / 4;
    for (auto& band : bands)
        band.process (signal, 0, quarter);
    for (auto& band : bands)
    {
        band.setShape (changed);
        band.process (signal, quarter, length - quarter);
    }
}

std::vector<std::uint32_t> bitsOf (const std::vector<float>& samples)
{
    std::vector<std::uint32_t> bits (samples.size());
    std::transform (samples.begin(), samples.end(), bits.begin(), [] (float s) { return std::bit_cast<std::uint32_t> (s); });
    return bits;
}
} // namespace

TEST_CASE ("A Stereo Band on a stereo track sounds bit for bit as a Left and a Right Band with its settings, every Shape, gliding")
{
    // The Stereo Band runs both channels in one loop, a lane each; a Left or Right Band runs one channel
    // on its own. Each lane must round exactly as the single channel does.
    const auto shape = GENERATE (from_range (test::allShapes));
    const bool brickwall = GENERATE (false, true);
    if (brickwall && ! isCut (shape))
        return;
    CAPTURE (static_cast<int> (shape), brickwall);

    const auto band = [&] (double frequency, double gainDb, double q) {
        return BandShape { .shape = shape, .slope = 24.0, .brickwall = brickwall, .gain = gainDb, .frequency = frequency, .q = q };
    };
    std::vector<test::BandModel> stereo { test::BandModel (sampleRate, 2, StereoPlacement::Stereo) };
    std::vector<test::BandModel> split { test::BandModel (sampleRate, 2, StereoPlacement::Left),
                                         test::BandModel (sampleRate, 2, StereoPlacement::Right) };
    for (auto* bands : { &stereo, &split })
        for (auto& b : *bands)
            b.setShape (band (1000.0, 12.0, 1.0));

    const auto left = noise (1), right = noise (2);
    std::vector<std::vector<float>> stereoSignal { left, right }, splitSignal { left, right };
    processWithGlide (stereo, band (1500.0, 6.0, 2.0), stereoSignal);
    processWithGlide (split, band (1500.0, 6.0, 2.0), splitSignal);

    REQUIRE (stereoSignal[0] != left); // the Band did something
    REQUIRE (bitsOf (stereoSignal[0]) == bitsOf (splitSignal[0]));
    REQUIRE (bitsOf (stereoSignal[1]) == bitsOf (splitSignal[1]));
}

TEST_CASE ("A placed part plays back on the channels it came from, decoded on its own")
{
    // What a part plays on each of three output channels, its channel p being 10 + p, for every Placement on stereo
    // and on mono. Left = Mid + Side and Right = Mid - Side; a third channel is silent, and so is Side on mono.
    struct Case
    {
        StereoPlacement placement;
        int numChannels;
        std::array<float, 3> expected;
    };
    const auto c = GENERATE (Case { StereoPlacement::Stereo, 2, { 10.0f, 11.0f, 0.0f } },
                             Case { StereoPlacement::Left, 2, { 10.0f, 0.0f, 0.0f } },
                             Case { StereoPlacement::Right, 2, { 0.0f, 10.0f, 0.0f } },
                             Case { StereoPlacement::Mid, 2, { 10.0f, 10.0f, 0.0f } },
                             Case { StereoPlacement::Side, 2, { 10.0f, -10.0f, 0.0f } },
                             Case { StereoPlacement::Stereo, 1, { 10.0f, 0.0f, 0.0f } },
                             Case { StereoPlacement::Right, 1, { 10.0f, 0.0f, 0.0f } },
                             Case { StereoPlacement::Mid, 1, { 10.0f, 0.0f, 0.0f } },
                             Case { StereoPlacement::Side, 1, { 0.0f, 0.0f, 0.0f } });
    CAPTURE (static_cast<int> (c.placement), c.numChannels);
    for (int ch = 0; ch < 3; ++ch)
    {
        CAPTURE (ch);
        CHECK (placedSample (c.placement, c.numChannels, ch, [] (int p) { return 10.0f + static_cast<float> (p); })
               == c.expected[static_cast<size_t> (ch)]);
    }
}
