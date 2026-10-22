#pragma once

// New in plugkit, lifted from eq1's plugin/FooterBar.h (eq1 at dbb1127): its OutputReadout, with isOpen, and
// outputReadoutText, taking Output Gain's silent bottom, where eq1's reads its own parameter's; listed in eq1#231.

#include "staple/controls/TextChip.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <limits>

namespace plugkit::ui
{

// What the Output readout shows: a level in dB to one decimal, signed ("+0.0 dB", "−1.8 dB"), or "−inf dB" at or
// below silentDb, Output Gain's silent bottom (eq1's -80 dB), or for a level that isn't finite. A range with no
// silent bottom (comp1's) leaves silentDb out.
juce::String outputReadoutText (double db, double silentDb = -std::numeric_limits<double>::infinity());

// The footer's Output readout: Output Gain, plus Auto Gain's estimate while Auto Gain is on, as text the plugin sets
// (outputReadoutText) on its editor's tick. A click opens and closes the output popover (OutputPopoverFrame), which the
// plugin's footer wires to it; fill2 behind it while that is open. A screen reader reads it as a button titled
// "Output" whose value is its text.
class OutputReadout final : public staple::TextChip
{
public:
    OutputReadout();

    // Drawn in text4 while Global Bypass leaves it unapplied.
    void setNotApplied (bool notApplied);
    bool isNotApplied() const { return notApplied; }
    // Whether its popover is open.
    void setOpen (bool open);
    bool isOpen() const { return open; }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    bool notApplied = false, open = false;
};

} // namespace plugkit::ui
