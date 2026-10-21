// Rewritten from eq1's tests/engine/StereoPlacementTest.cpp (eq1 at dbb1127), which drives eq1's Engine, against
// Placement's routing and Mid/Side encode and decode directly: the same signals and tolerances. Listed in eq1#231.

#include "Measure.h"

#include "plugkit/Placement.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <array>
#include <cmath>
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

// eq1's tests' Band: a Bell at 1 kHz, +12 dB, Q 1.
const Cascade& bell()
{
    static const Cascade cascade = designBand (test::bellBand (1000.0, 12.0, 1.0), sampleRate);
    return cascade;
}

// Filters samples in place through the Bell, from a clear state.
void filter (std::vector<float>& samples)
{
    std::array<BiquadState, maxSections> states {};
    test::runCascade (bell(), states, samples);
}

// Processes signal in place by placement, as eq1's Band places its filter (Band::Chain::runPlaced and runMidSide):
// each channel routePlacement selects is filtered as it is; a Mid or Side part is encoded, filtered and decoded.
void process (StereoPlacement placement, std::vector<std::vector<float>>& signal)
{
    routePlacement (
        placement,
        static_cast<int> (signal.size()),
        [&] (int, int ch) { filter (signal[static_cast<size_t> (ch)]); },
        [&] (bool mid) {
            auto& left = signal[0];
            auto& right = signal[1];
            std::vector<float> midSamples (left.size()), sideSamples (left.size());
            for (size_t i = 0; i < left.size(); ++i)
            {
                midSamples[i] = encodeMid (left[i], right[i]);
                sideSamples[i] = encodeSide (left[i], right[i]);
            }
            filter (mid ? midSamples : sideSamples);
            for (size_t i = 0; i < left.size(); ++i)
            {
                left[i] = decodeLeft (midSamples[i], sideSamples[i]);
                right[i] = decodeRight (midSamples[i], sideSamples[i]);
            }
        });
}

// What a Stereo Band does to one channel on its own: the reference for every Placement.
std::vector<float> filtered (std::vector<float> samples)
{
    filter (samples);
    return samples;
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

TEST_CASE ("A placed part plays back on the channels it came from, decoded on its own")
{
    // New in plugkit (eq1 tests placedSample only through its Engine's Solo): what a part plays on each of three
    // output channels, its channel p being 10 + p. Left = Mid + Side and Right = Mid - Side; a third channel is silent.
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
