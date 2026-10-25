#pragma once

// New in plugkit, lifted from eq1's plugin/FooterBar.h (eq1 at dbb1127): its OutputReadout's look and behaviour, with
// isOpen, as a readout any setting in the footer can use; listed in eq1#231.

#include "staple/controls/TextChip.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugkit::ui
{

// A footer readout: a setting's value as text the plugin sets on its editor's tick, in text1. A click opens and closes
// its popover (FooterPopoverFrame), which the plugin's footer wires to it; fill2 behind it while that is open. A screen
// reader reads it as a button titled by its setting whose value is its text. The Output readout (OutputReadout) is one.
class FooterReadout : public staple::TextChip
{
public:
    // Titled by its setting ("Output"), with a tooltip saying what it reads and that a click opens its popover.
    FooterReadout (const juce::String& title, const juce::String& tooltip);

    // Drawn in text4 while its setting is not applied (Global Bypass on, for Output).
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
