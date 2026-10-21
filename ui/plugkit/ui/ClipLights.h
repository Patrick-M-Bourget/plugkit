#pragma once

// Copied from eq1's plugin/PluginProcessor.cpp and .h (eq1 at dbb1127), in namespace plugkit::ui: the Clip Light
// latch eq1's processor keeps (its clipLit array, and the over check in readOutputLevel, isClipLit and
// clearClipLights), as a class of its own.

#include "plugkit/OutputLevel.h"

#include <array>
#include <atomic>

namespace plugkit::ui
{

// The Output Meter's Clip Lights, one per channel, mono and stereo only. A peak above 0 dBFS, a sample beyond full
// scale, lights its channel's; full scale itself is not an over. A light stays lit until clear(), editor or not. Not
// saved: a plugin clears them when it restores a session. Any thread may light, read or clear them, without a lock.
class ClipLights
{
public:
    static constexpr int numChannels = 2;

    // Lights channel's Clip Light if level is an over; returns level, so readOutputLevel can pass it on.
    OutputLevel note (int channel, OutputLevel level)
    {
        if (level.peakDb > 0.0 && channel >= 0 && channel < numChannels)
            lit[static_cast<size_t> (channel)] = true;
        return level;
    }

    bool isLit (int channel) const
    {
        return channel >= 0 && channel < numChannels && lit[static_cast<size_t> (channel)].load();
    }

    // Puts out both.
    void clear()
    {
        for (auto& light : lit)
            light = false;
    }

private:
    std::array<std::atomic<bool>, numChannels> lit {};
};

} // namespace plugkit::ui
