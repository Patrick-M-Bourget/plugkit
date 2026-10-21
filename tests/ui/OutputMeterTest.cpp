// Copied from eq1's tests/plugin/OutputMeterTest.cpp (eq1 at dbb1127), against plugkit::ui: its processor cases on
// the Clip Light latch, as ClipLights cases.

#include "plugkit/ui/ClipLights.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

using plugkit::ui::ClipLights;

namespace
{

// A sine's peak and RMS in dBFS, at amplitude.
double peakOf (double amplitude) { return 20.0 * std::log10 (amplitude); }
double rmsOf (double amplitude) { return peakOf (amplitude / std::sqrt (2.0)); }

// The Output Level of an over by a float step: a sample at the next float above full scale.
const double justOver = peakOf (std::nextafter (1.0f, 2.0f));

} // namespace

TEST_CASE ("An over lights its channel's Clip Light, which stays lit until cleared, and clearing puts out both")
{
    ClipLights lights;
    // A sample at full scale is not an over; one a float step beyond it is.
    lights.note (0, { .peakDb = 0.0, .rmsDb = -3.0 });
    lights.note (1, { .peakDb = 0.0, .rmsDb = -3.0 });
    CHECK_FALSE (lights.isLit (0));
    CHECK_FALSE (lights.isLit (1));
    lights.note (0, { .peakDb = justOver, .rmsDb = -3.0 });
    CHECK (lights.isLit (0));
    lights.clear();

    lights.note (0, {});
    lights.note (1, { .peakDb = peakOf (1.5), .rmsDb = rmsOf (1.5) });
    CHECK_FALSE (lights.isLit (0));
    CHECK (lights.isLit (1));

    // Quiet again, read again: still lit.
    lights.note (0, { .peakDb = peakOf (0.1), .rmsDb = rmsOf (0.1) });
    lights.note (1, { .peakDb = peakOf (0.1), .rmsDb = rmsOf (0.1) });
    CHECK (lights.isLit (1));

    lights.note (0, { .peakDb = peakOf (1.5), .rmsDb = rmsOf (1.5) });
    REQUIRE (lights.isLit (0));
    lights.clear();
    CHECK_FALSE (lights.isLit (0));
    CHECK_FALSE (lights.isLit (1));
}

TEST_CASE ("A Clip Light lit on one thread reads lit on another")
{
    ClipLights lights;
    std::atomic<bool> stop { false };
    // One thread lights it over and over while another reads it.
    std::thread lighting ([&] {
        while (! stop)
            lights.note (1, { .peakDb = 6.0 });
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (10);
    while (! lights.isLit (1) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    CHECK (lights.isLit (1));
    stop = true;
    lighting.join();
    CHECK_FALSE (lights.isLit (0));
}
