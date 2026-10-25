#pragma once

// New in plugkit, lifted from eq1's plugin/OutputPopover.h (eq1 at dbb1127): the frame of its OutputPopover (opening
// from the readout, following the processor) as a base, FooterPopoverFrame, whose followProcessor a plugin's popover
// gives, and its OutputToggle; eq1's Output Gain knob, Pan Mode, Output Pan, Phase Invert, Auto Gain and Output Meter
// toggle, and its layout, stay in eq1. Listed in eq1#231.

#include "EditorTick.h"
#include "staple/Icons.h"
#include "staple/controls/Popover.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

namespace plugkit::ui
{

// One of a footer popover's toggles (eq1's Phase Invert, Auto Gain and Output Meter): an icon, or a letter (Auto
// Gain's "A"), on fill2 in text1 while on, else in text3. A click toggles it; it never takes focus from a click.
class OutputToggle final : public juce::Button
{
public:
    OutputToggle (const juce::String& title, std::optional<staple::Icon> icon, const juce::String& letter = {});
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    std::optional<staple::Icon> icon;
};

// The frame of a popover a footer readout (FooterReadout) opens, the Output readout's output popover among them: a
// staple::Popover that opens above the readout, right-aligned to it, and closes on Escape or a click outside it and the readout. While open it
// follows the processor on the editor's tick (EditorTick), and it shows the processor's state again as it opens,
// through followProcessor. A plugin's popover derives from it: it adds its controls, sizes its card (setCardSize),
// lays them out, and gives followProcessor, which it also calls once its controls are built. The footer opens and
// closes it from the readout's click, and puts it back in the footer, hidden, in onClose, as eq1's does.
class FooterPopoverFrame : public staple::Popover, private EditorTick::Client
{
public:
    FooterPopoverFrame();
    ~FooterPopoverFrame() override;

    // Opens above the readout, right-aligned to it, kept inside the overlay layer, once it shows the processor's state.
    void openFrom (juce::Component& readout);

protected:
    // Shows what the processor holds that a host parameter attachment doesn't (eq1's Output Meter toggle, and
    // whether the track is stereo, in its output popover), which a restored session changes too: as it opens, and on each tick while open.
    virtual void followProcessor() = 0;

private:
    void tick (double seconds) override;
};

} // namespace plugkit::ui
