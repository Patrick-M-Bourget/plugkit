// Rewritten from eq1's tests/engine/SettingsHandoffTest.cpp (eq1 at dbb1127), "Settings set on another thread while
// audio is processing reach the Engine", which hands settings to eq1's Engine, against LatestValue directly: the same
// edits on one thread, taken on another. Listed in eq1#231.

#include "plugkit/LatestValue.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <thread>

using namespace plugkit;

namespace
{
// A snapshot with as many slots as eq1's Settings has Bands, each holding the edit that wrote it.
struct Snapshot
{
    std::array<double, 24> slots {};
};
} // namespace

TEST_CASE ("Values published on another thread while the reader takes them arrive whole, the last one last")
{
    LatestValue<Snapshot> latest;
    REQUIRE (latest.takeLatest() == nullptr);

    std::atomic<bool> done { false };
    std::thread editor ([&] {
        // Every slot changes on every edit, so a torn snapshot would leave slots out of step.
        for (int edit = 0; edit < 2000; ++edit)
        {
            Snapshot snapshot;
            snapshot.slots.fill (edit);
            latest.publish (snapshot);
        }
        Snapshot final;
        final.slots.fill (-1.0);
        latest.publish (final);
        done = true;
    });

    double newest = -2.0;
    bool whole = true, inOrder = true;
    const auto take = [&] {
        if (const auto* snapshot = latest.takeLatest())
        {
            const double edit = snapshot->slots[0];
            whole = whole && std::all_of (snapshot->slots.begin(), snapshot->slots.end(), [edit] (double s) { return s == edit; });
            inOrder = inOrder && (edit == -1.0 || edit > newest);
            newest = edit;
        }
    };
    while (! done)
        take();
    editor.join();
    take();

    CHECK (whole);
    CHECK (inOrder);
    CHECK (newest == -1.0);
    CHECK (latest.takeLatest() == nullptr); // nothing new since
}
