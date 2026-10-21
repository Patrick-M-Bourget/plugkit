// Rewritten from eq1's tests/engine/OutputLevelTest.cpp (eq1 at dbb1127), its generic cases, which play through eq1's
// Engine, against OutputLevelTap directly: the same signals, in the same block cuts, into one channel's tap. Listed in
// eq1#231.

#include "plugkit/OutputLevel.h"
#include "plugkit/OutputLevelTap.h"
#include "plugkit/test/AllocationGuard.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

using namespace plugkit;
using Catch::Matchers::WithinAbs;

namespace
{
// Plays signals into a tap, in blocks whose sizes cycle through 1, 37, 512 and 160 samples.
struct Player
{
    OutputLevelTap& tap;
    double sampleRate;
    long long position = 0;
    std::vector<float>* played = nullptr; // every sample played, when wanted

    void sine (double seconds, double amplitude, double frequency = 997.0)
    {
        play (seconds, [&] (long long n) {
            return static_cast<float> (amplitude * std::sin (2.0 * std::numbers::pi * frequency * static_cast<double> (n) / sampleRate));
        });
    }

    void silence (double seconds) { sine (seconds, 0.0); }

    // White noise, uniform between -amplitude and amplitude.
    void noise (double seconds, double amplitude, unsigned seed)
    {
        std::mt19937 random (seed);
        std::uniform_real_distribution<float> uniform (static_cast<float> (-amplitude), static_cast<float> (amplitude));
        play (seconds, [&] (long long) { return uniform (random); });
    }

private:
    // Plays sampleAt(position) onwards for the given time.
    template <typename SampleAt>
    void play (double seconds, SampleAt sampleAt)
    {
        constexpr int blockSizes[] = { 1, 37, 512, 160 };
        std::vector<float> block (512);
        const auto total = std::llround (seconds * sampleRate);
        for (long long done = 0, cut = 0; done < total; ++cut)
        {
            const int count = static_cast<int> (std::min<long long> (blockSizes[cut % 4], total - done));
            for (int i = 0; i < count; ++i)
            {
                block[static_cast<size_t> (i)] = sampleAt (position + i);
                if (played != nullptr)
                    played->push_back (block[static_cast<size_t> (i)]);
            }
            tap.process (block.data(), count);
            position += count;
            done += count;
        }
    }
};
} // namespace

TEST_CASE ("Output Level of a full-scale sine is 0 dBFS peak and -3.01 dB RMS")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    CAPTURE (sampleRate);

    OutputLevelTap tap;
    tap.prepare (sampleRate);
    Player { tap, sampleRate }.sine (0.5, 1.0);

    const auto level = tap.read();
    CHECK_THAT (level.peakDb, WithinAbs (0.0, 0.01));
    CHECK_THAT (level.rmsDb, WithinAbs (-3.01, 0.1));
}

TEST_CASE ("A single-sample over is reported by the next read, after quieter blocks, and only by that read")
{
    constexpr double sampleRate = 48000.0;
    OutputLevelTap tap;
    tap.prepare (sampleRate);

    std::vector<float> block (512, 0.0f);
    block[100] = 1.5f; // +3.52 dBFS
    tap.process (block.data(), 512);
    Player player { tap, sampleRate };
    player.sine (0.1, 0.25);

    CHECK_THAT (tap.read().peakDb, WithinAbs (3.52, 0.01));
    player.sine (0.1, 0.25);
    CHECK_THAT (tap.read().peakDb, WithinAbs (-12.04, 0.01));
}

TEST_CASE ("Output Level RMS covers only the last 300 ms, and silence reads exactly the floor")
{
    const double sampleRate = GENERATE (44100.0, 96000.0);
    CAPTURE (sampleRate);
    OutputLevelTap tap;
    tap.prepare (sampleRate);

    SECTION ("before anything is processed")
    {
        const auto level = tap.read();
        CHECK (level.peakDb == levelFloorDb);
        CHECK (level.rmsDb == levelFloorDb);
    }

    Player player { tap, sampleRate };
    player.sine (1.0, 0.9);

    SECTION ("half the window silent: half the power")
    {
        player.silence (0.15);
        CHECK_THAT (tap.read().rmsDb, WithinAbs (20.0 * std::log10 (0.9) - 3.01 - 3.01, 0.1));
    }

    SECTION ("a whole window of silence, even after noise far above full scale")
    {
        // Rounding in a running sum of squares would leave more than the floor after some of these.
        for (unsigned seed = 0; seed < 8; ++seed)
        {
            CAPTURE (seed);
            player.noise (0.1 + 0.07 * seed, 1000.0, seed);
            player.silence (0.3);
            tap.read(); // the loud passage's peak
            const auto level = tap.read();
            CHECK (level.rmsDb == levelFloorDb);
            CHECK (level.peakDb == levelFloorDb);
        }
    }

    SECTION ("a read doesn't reset RMS")
    {
        const double first = tap.read().rmsDb;
        CHECK (tap.read().rmsDb == first);
    }

    SECTION ("a re-prepare starts again from silence")
    {
        tap.prepare (sampleRate);
        const auto level = tap.read();
        CHECK (level.peakDb == levelFloorDb);
        CHECK (level.rmsDb == levelFloorDb);
    }
}

TEST_CASE ("Output Level RMS is the RMS of the last 300 ms of what was played, on a sine and on noise")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    CAPTURE (sampleRate);
    OutputLevelTap tap;
    tap.prepare (sampleRate);
    std::vector<float> played;
    Player player { tap, sampleRate, 0, &played };

    SECTION ("sine")
    {
        player.sine (0.5, 0.7, 1234.5);
    }
    SECTION ("noise")
    {
        player.noise (0.5, 0.5, 7);
    }
    SECTION ("noise after a louder passage, so the window has to forget it")
    {
        player.noise (0.4, 0.9, 3);
        player.noise (0.35, 0.2, 4);
    }

    // A rectangular window of the last 300 ms, summed the plain way.
    const auto window = static_cast<size_t> (std::lround (0.3 * sampleRate));
    REQUIRE (played.size() >= window);
    const double meanSquare = std::accumulate (played.end() - static_cast<long> (window), played.end(), 0.0, [] (double sum, float s) { return sum + static_cast<double> (s) * s; })
                              / static_cast<double> (window);
    CHECK_THAT (tap.read().rmsDb, WithinAbs (10.0 * std::log10 (meanSquare), 0.01));
}

TEST_CASE ("A NaN peak reads the floor")
{
    OutputLevelTap tap;
    tap.prepare (48000.0);
    const float nan[] = { std::numeric_limits<float>::quiet_NaN() };
    tap.process (nan, 1);
    const auto level = tap.read();
    CHECK (level.peakDb == levelFloorDb);
    CHECK (level.rmsDb == levelFloorDb);
}

TEST_CASE ("Processing and reading the Output Level never allocate")
{
    OutputLevelTap tap;
    tap.prepare (48000.0);
    std::vector<float> block (512, 0.5f);

    const test::AllocationGuard guard;
    for (int i = 0; i < 100; ++i)
    {
        tap.process (block.data(), 512);
        tap.read();
    }
    CHECK (guard.allocations() == 0);
}

TEST_CASE ("A reader can keep reading the Output Level while the tap is prepared at other sample rates")
{
    // A host re-prepares when the sample rate changes, while the editor's meter goes on reading.
    OutputLevelTap tap;
    tap.prepare (48000.0);
    std::atomic<bool> done { false };
    std::thread reader ([&] {
        while (! done)
            tap.read();
    });

    std::vector<float> block (256, 0.25f);
    for (int round = 0; round < 200; ++round)
    {
        tap.prepare (round % 2 == 0 ? 44100.0 : 96000.0);
        for (int b = 0; b < 4; ++b)
            tap.process (block.data(), 256);
    }
    done = true;
    reader.join();
    SUCCEED();
}
