#pragma once

// What a Shape design reads from a Band's settings. Shape, isCut, hasSlope and liveGainLimitDb are copied from eq1's
// engine/include/eq1/Settings.h (eq1 at dbb1127): the same enumerators in the same order, the same values. BandShape
// holds the fields of eq1's BandSettings that structureOf() and designBand() read, and nothing else (comp1#5's
// decision); eq1 converts its BandSettings to one at those call sites. Listed in eq1#231.

namespace plugkit
{

// In Pro-Q 4's order, which eq1's host parameter follows (eq1's ADR 0003).
enum class Shape
{
    Bell,
    LowShelf,
    LowCut,
    HighShelf,
    HighCut,
    Notch,
    BandPass,
    TiltShelf,
    FlatTilt,
    AllPass,
};

inline bool isCut (Shape shape) { return shape == Shape::LowCut || shape == Shape::HighCut; }

// Bell ignores Slope until Bell Slope (eq1#19); Flat Tilt has none.
inline bool hasSlope (Shape shape) { return shape != Shape::Bell && shape != Shape::FlatTilt; }

// Live Gain never goes beyond +/- this many dB, whatever Gain and Dynamic Range are.
inline constexpr double liveGainLimitDb = 30.0;

// A Band's filter settings, with eq1's BandSettings defaults.
struct BandShape
{
    Shape shape = Shape::Bell;
    // dB/oct, 0 to 96. Kept as set: the design raises it to the Shape's minimum and rounds it to a whole order, so
    // switching Shape and back restores it (eq1's ADR 0003).
    double slope = 12.0;
    bool brickwall = false; // Low Cut and High Cut only: overrides Slope
    double gain = 0.0;         // dB
    double frequency = 1000.0; // Hz
    double q = 1.0;
};

} // namespace plugkit
