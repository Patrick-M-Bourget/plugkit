#pragma once

// New in plugkit, lifted from eq1's plugin/FooterBar.h (eq1 at dbb1127): its OutputReadout's look and behaviour, with
// isOpen, as a readout any setting in the footer can use; listed in eq1#231.

#include "staple/controls/TextChip.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugkit::ui
{

// A footer readout: a setting's value as text the plugin sets on its editor's tick ("SAT +6.0 dB"), in text1. Given
// less than its full width (getIdealWidth), it shows only its short label ("SAT"), clipped if it has less room even
// than that; it is never hidden. A click opens and closes its popover (FooterPopoverFrame), which the plugin's footer
// wires to it; fill2 behind it while that is open. A screen reader reads it as a button titled by its setting whose
// value is its whole text, shrunk or not. The Output readout (OutputReadout) is one.
class FooterReadout : public staple::TextChip
{
public:
    // Titled by its setting ("Saturation"), with the short label it shrinks to ("SAT") and a tooltip saying what it
    // reads and that a click opens its popover.
    FooterReadout (const juce::String& title, const juce::String& label, const juce::String& tooltip);

    // The width it needs to show its label: the least a footer gives it.
    int getShrunkWidth() const { return idealWidthFor (label); }
    // Whether it has less than its full width, so shows only its label.
    bool isShrunk() const { return getWidth() < getIdealWidth(); }

    // Drawn in text4 while its setting is not applied (Global Bypass on, for Output).
    void setNotApplied (bool notApplied);
    bool isNotApplied() const { return notApplied; }
    // Whether its popover is open.
    void setOpen (bool open);
    bool isOpen() const { return open; }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

protected:
    juce::String shownText() const override { return isShrunk() ? label : getButtonText(); }

private:
    juce::String label;
    bool notApplied = false, open = false;
};

} // namespace plugkit::ui
