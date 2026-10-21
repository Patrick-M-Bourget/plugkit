#pragma once

// Copied from eq1's plugin/LevelBallistics.h (eq1 at dbb1127), in namespace plugkit::ui, its floor
// plugkit::levelFloorDb.

#include "plugkit/OutputLevel.h"

#include <algorithm>

namespace plugkit::ui
{

// How a level meter moves: it rises at once to a louder level read, and falls at 20 dB/s, no lower than
// the level read.
class LevelBallistics
{
public:
    static constexpr double fallDbPerSecond = 20.0;

    // A level read seconds after the one before; returns the level to show.
    double update (double levelDb, double seconds)
    {
        shown = std::max (levelDb, shown - fallDbPerSecond * seconds);
        return shown;
    }

    // Shows nothing, as before the first read.
    void reset() { shown = levelFloorDb; }

private:
    double shown = levelFloorDb;
};

} // namespace plugkit::ui
