// Copied from eq1's plugin/display/DisplayGeometry.cpp (eq1 at dbb1127), in namespace plugkit::ui::display, without
// handleOf (BandSettings), which stays in eq1.

#include "DisplayGeometry.h"

#include <cmath>

namespace plugkit::ui::display
{

float DisplayGeometry::xOf (double frequency) const
{
    return static_cast<float> (std::log (frequency / lowestFrequency) / std::log (highestFrequency / lowestFrequency) * width);
}

double DisplayGeometry::frequencyAt (float x) const
{
    return lowestFrequency * std::pow (highestFrequency / lowestFrequency, juce::jlimit (0.0, 1.0, static_cast<double> (x) / width));
}

float DisplayGeometry::yOf (double db) const
{
    const auto range = static_cast<float> (rangeDb);
    const float half = static_cast<float> (height) * 0.5f;
    return half - static_cast<float> (db) / range * (half - handleRadius);
}

double DisplayGeometry::dbAt (float y) const
{
    const auto range = static_cast<double> (rangeDb);
    const double half = height * 0.5;
    return (half - y) / (half - handleRadius) * range;
}

} // namespace plugkit::ui::display
