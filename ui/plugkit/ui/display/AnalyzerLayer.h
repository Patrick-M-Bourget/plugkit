#pragma once

// Copied from eq1's plugin/display/AnalyzerLayer.h (eq1 at dbb1127), in namespace plugkit::ui::display, over
// AnalyzerFrame.h rather than eq1's DisplayFrame.h, and spectrumLine made public.

#include "AnalyzerFrame.h"
#include "DisplayGeometry.h"

namespace plugkit::ui::display
{

// Where a spectrum is drawn at x, with Analyzer Tilt: the top of the display is 0 dB, the bottom the
// Analyzer's range below.
float spectrumYAt (const DisplayGeometry& geometry, const AnalyzerSettings& settings, const AnalyzerSpectrum& spectrum, float x);

// A spectrum's line across the display: one path through spectrumYAt, a point every DisplayGeometry::pixelStep
// from x = 0 to the right edge.
juce::Path spectrumLine (const DisplayGeometry& geometry, const AnalyzerSettings& settings, const AnalyzerSpectrum& spectrum);

// The Analyzer, behind the curves: the main spectrum (post-EQ, or pre-EQ when only it is shown) with a
// gradient fill and a line, Peak Hold a faint line, pre-EQ a faint line beside post-EQ, and the
// Sidechain, when the frame has one, a line in its own tint.
void paintAnalyzer (juce::Graphics& g, const DisplayGeometry& geometry, const AnalyzerFrame& frame);

} // namespace plugkit::ui::display
