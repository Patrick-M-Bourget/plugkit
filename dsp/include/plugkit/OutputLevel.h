#pragma once

// Copied from eq1's engine/include/eq1/Engine.h (eq1 at dbb1127): its levelFloorDb and OutputLevel, in namespace
// plugkit, in a header of their own.

namespace plugkit
{

// What the Output Level and the Detection Level read for silence, and for anything quieter.
inline constexpr double levelFloorDb = -150.0;

// The Output Level of one channel, in dBFS.
struct OutputLevel
{
    double peakDb = levelFloorDb; // sample peak since the last read
    double rmsDb = levelFloorDb;  // over the last 300 ms, rounded up to whole 16-sample runs
};

} // namespace plugkit
