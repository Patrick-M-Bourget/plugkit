// What AllocationGuard (plugkit::test_support) measures, which a plugin's realtime-safety and footprint tests read: the
// heap allocations its own thread makes while it is alive, and the bytes they still hold.

#include "plugkit/test/AllocationGuard.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

using plugkit::test::AllocationGuard;

namespace
{
// What a guard reads at one moment. The checks come after the guard is gone: Catch's own may allocate.
struct Reading
{
    std::size_t allocations, bytesHeld;
};
Reading read (const AllocationGuard& guard) { return { guard.allocations(), guard.bytesHeld() }; }

// Where an allocation escapes to, so the compiler can't leave it out.
char* volatile escaped = nullptr;
} // namespace

TEST_CASE ("AllocationGuard counts its thread's allocations and the bytes they hold, freed ones given back")
{
    Reading before {}, held {}, freed {};
    {
        const AllocationGuard guard;
        before = read (guard);
        auto block = std::make_unique<char[]> (1000);
        escaped = block.get();
        held = read (guard);
        block.reset();
        freed = read (guard);
    }
    CHECK (before.allocations == 0);
    CHECK (before.bytesHeld == 0);
    CHECK (held.allocations == 1);
    CHECK (held.bytesHeld >= 1000);
    CHECK (freed.allocations == 1);
    CHECK (freed.bytesHeld == 0);
}

TEST_CASE ("AllocationGuard counts nothing another thread allocates")
{
    std::atomic<bool> go { false }, allocated { false };
    std::thread other ([&] {
        while (! go)
            std::this_thread::yield();
        std::vector<char> elsewhere (1000);
        char* volatile kept = elsewhere.data(); // escapes, so it isn't left out
        (void) kept;
        allocated = true;
        while (go)
            std::this_thread::yield();
    });
    Reading after {};
    {
        const AllocationGuard guard;
        go = true;
        while (! allocated)
            std::this_thread::yield();
        after = read (guard);
        go = false;
    }
    other.join();
    CHECK (after.allocations == 0);
    CHECK (after.bytesHeld == 0);
}
