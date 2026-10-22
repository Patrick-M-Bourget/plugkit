#pragma once

// Copied from eq1's plugin/OutputPopover.h (eq1 at dbb1127): its OutputPanSlider, as a slider that fills from its
// centre (eq1's Output Pan) or from its left (a level), with the readout line eq1's OutputPopover paints beneath
// Output Pan; Output Pan's own text (outputPanText), tooltip, steps and Pan Mode stay in eq1. Listed in eq1#231.

#include "KeyboardSlider.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace plugkit::ui
{

// A horizontal slider for a popover, the full width of its row: a 6 px track (fill2) with a small thumb (text1),
// filled in text2 from its centre to the value, past a centre tick (eq1's Output Pan), or from its left end to the
// value (a level), and a readout line beneath it in fs1 text3: a text at its left, one centred and one at its right.
// It travels as its range does (valueToProportionOfLength), so an attached parameter's own mapping holds, an Off
// at -inf at the bottom included. A click on the track or a drag sets it, as one gesture (one undo step), on whole
// multiples of its drag step if it has one; the second press of a double-click sets the double-click return value
// (KeyboardSlider). Only the track takes the mouse. Its arrow steps, double-click return value, tooltip and title
// are its owner's to set, as for any KeyboardSlider; the readout line repaints with the value, and its owner
// repaints it when something else its text shows changes (eq1's Pan Mode).
class PopoverSlider : public KeyboardSlider
{
public:
    enum class Fill
    {
        fromCentre, // eq1's Output Pan
        fromLeft    // a level
    };

    // What the readout line shows: at its left, centred and at its right (eq1's "L", "40 L" and "R").
    struct Readout
    {
        juce::String left, centre, right;
        bool operator== (const Readout&) const = default;
    };

    // The track's row, the gap below it and the readout line's row: eq1's Output Pan and its readout.
    static constexpr int trackHeight = 14, readoutGap = 4, readoutHeight = 14;
    static constexpr int height = trackHeight + readoutGap + readoutHeight;

    explicit PopoverSlider (Fill fill, const juce::String& title = {});

    Fill getFill() const { return fill; }

    // The readout line's texts for a value. By default its title at the left and its value's text at the right
    // (a level's "Dry" and "-6.0 dB").
    std::function<Readout (double value)> readout;
    // What the readout line shows now.
    Readout readoutNow();

    // A click or drag lands on a whole multiple of step, in its own units (eq1's Output Pan: 1, whole percent); 0, the
    // default, lands anywhere its interval allows.
    void setDragStep (double step) { dragStep = step; }

    // Where the readout line is, in its own bounds.
    juce::Rectangle<int> readoutArea() const;

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override {}

private:
    double valueAt (float x);
    double landed (double value) const;
    double valueDraggedBy (double from, float pixels, bool fine) override;

    Fill fill;
    double dragStep = 0.0;
};

} // namespace plugkit::ui
