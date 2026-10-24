#pragma once

// Copied from eq1's plugin/display/DisplayFrame.h (eq1 at dbb1127), in namespace plugkit::ui::display: its
// AnalyzerFrame only, in a header of its own, its Sidechain spectrum optional. The EQ display's DisplayFrame, sameBand
// and bandColour stay in eq1.

#include "../AnalyzerSettings.h"
#include "../AnalyzerSpectrum.h"

#include <functional>
#include <optional>

namespace plugkit::ui::display
{

// The Analyzer's spectra this frame. A display with no Sidechain spectrum leaves sidechain out, and nothing is drawn
// for it whatever settings.showSidechain says; one with a Sidechain passes it as eq1 does, `.sidechain = spectrum`.
struct AnalyzerFrame
{
    const AnalyzerSettings& settings;
    const AnalyzerSpectrum &preEq, &postEq;
    std::optional<std::reference_wrapper<const AnalyzerSpectrum>> sidechain {};
    const AnalyzerSpectrum* held = nullptr; // the spectrum Peak Hold is drawn for, if any
};

} // namespace plugkit::ui::display
