#pragma once

// Copied from eq1's plugin/display/DisplayGeometry.h (eq1 at dbb1127), in namespace plugkit::ui::display. The
// Frequency range is two fields, defaulting to eq1's 10 Hz to 30 kHz, where eq1 reads its Frequency parameter's range;
// the width below which the grid is narrow is a field, defaulting to eq1's 800 px (eq1's
// tokens::layout::narrowDisplayWidth); and handleOf (BandSettings), on eq1's Band model, stays in eq1.

#include <juce_graphics/juce_graphics.h>

namespace plugkit::ui::display
{

// Where a display draws things, for a display of width x height pixels: Frequency on a log scale from
// lowestFrequency to highestFrequency, and dB over +/- the Display Range, a handle's radius in from the top and
// bottom.
struct DisplayGeometry
{
    static constexpr float handleRadius = 9.0f;
    static constexpr float pixelStep = 2.0f; // the curves and spectra are evaluated every this many pixels

    int width = 0, height = 0;
    int rangeDb = 12; // the Display Range
    double lowestFrequency = 10.0, highestFrequency = 30000.0;
    int narrowWidth = 800; // below this width the grid drops its minor lines and every other Frequency label

    float xOf (double frequency) const;
    double frequencyAt (float x) const;
    float yOf (double db) const;
    double dbAt (float y) const;
    juce::Rectangle<float> bounds() const { return juce::Rectangle<int> (width, height).toFloat(); }

    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wfloat-equal")
    bool operator== (const DisplayGeometry&) const = default;
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE
};

} // namespace plugkit::ui::display
