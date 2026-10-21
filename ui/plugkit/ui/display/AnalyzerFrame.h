#pragma once

// Copied from eq1's plugin/display/DisplayFrame.h (eq1 at dbb1127), in namespace plugkit::ui::display: its
// AnalyzerFrame only, in a header of its own. The EQ display's DisplayFrame, sameBand and bandColour stay in eq1.

#include "../AnalyzerSettings.h"
#include "../AnalyzerSpectrum.h"

namespace plugkit::ui::display
{

// The Analyzer's spectra this frame.
struct AnalyzerFrame
{
    const AnalyzerSettings& settings;
    const AnalyzerSpectrum &preEq, &postEq, &sidechain;
    const AnalyzerSpectrum* held = nullptr; // the spectrum Peak Hold is drawn for, if any
};

} // namespace plugkit::ui::display
