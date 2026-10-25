#pragma once

// New in plugkit, lifted from eq1's plugin/FooterBar.h (eq1 at dbb1127): its OutputReadout, as a FooterReadout, and
// outputReadoutText, taking Output Gain's silent bottom, where eq1's reads its own parameter's; listed in eq1#231.

#include "FooterReadout.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <limits>

namespace plugkit::ui
{

// What the Output readout shows: a level in dB to one decimal, signed ("+0.0 dB", "−1.8 dB"), or "−inf dB" at or
// below silentDb, Output Gain's silent bottom (eq1's -80 dB), or for a level that isn't finite. A range with no
// silent bottom (comp1's) leaves silentDb out.
juce::String outputReadoutText (double db, double silentDb = -std::numeric_limits<double>::infinity());

// The footer's Output readout: a FooterReadout titled "Output", shrinking to "OUT", showing Output Gain, plus Auto
// Gain's estimate while Auto Gain is on, as text the plugin sets (outputReadoutText) on its editor's tick. Its popover is
// the plugin's output popover. It is not applied while Global Bypass is on.
class OutputReadout final : public FooterReadout
{
public:
    OutputReadout();
};

} // namespace plugkit::ui
