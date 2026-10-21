#pragma once

// Copied from eq1's engine/src/LatestValue.h (eq1 at dbb1127), with <cstddef> included for std::size_t; listed in
// eq1#231.

#include <array>
#include <atomic>
#include <cstddef>

namespace plugkit
{

// Lock-free handoff of the newest value from one writer thread to one reader thread (a triple
// buffer). Neither side blocks or allocates; the reader skips values it was too slow to see.
template <typename T>
class LatestValue
{
public:
    void publish (const T& value)
    {
        slots[static_cast<std::size_t> (back)] = value;
        back = middle.exchange (back | fresh, std::memory_order_acq_rel) & indexMask;
    }

    // The newest value published since the last call, or null if there is none.
    const T* takeLatest()
    {
        if ((middle.load (std::memory_order_relaxed) & fresh) == 0)
            return nullptr;
        front = middle.exchange (front, std::memory_order_acq_rel) & indexMask;
        return &slots[static_cast<std::size_t> (front)];
    }

private:
    static constexpr int fresh = 4, indexMask = 3;

    std::array<T, 3> slots {};
    int back = 0;  // writer only
    int front = 1; // reader only
    std::atomic<int> middle { 2 };
};

} // namespace plugkit
