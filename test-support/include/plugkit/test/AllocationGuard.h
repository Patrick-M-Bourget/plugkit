#pragma once

// Copied from eq1's tests/engine/AllocationGuard.h (eq1 at dbb1127); listed in eq1#231. Test-only, in
// plugkit::test_support: it replaces operator new and delete in the test executable that links it.

#include <cstddef>

namespace plugkit::test
{

// Counts heap allocations made on the current thread while it is alive, and the bytes they hold:
// what the allocator gave out, less what the thread freed meanwhile.
class AllocationGuard
{
public:
    AllocationGuard();
    ~AllocationGuard();

    std::size_t allocations() const;
    std::size_t bytesHeld() const;
};

} // namespace plugkit::test
