#pragma once

// Copied from eq1's plugin/staple/WindowBackground.h (eq1 at dbb1127); listed in eq1#231.

#include <juce_graphics/juce_graphics.h>

namespace staple
{

// The window's background (HANDOFF.md §4, "Window"): bg0 under three soft neutral highlights, laid out
// as proportions of window, the window's bounds in logical pixels. The editor paints it once under
// everything; the EQ display, which paints no background of its own, fades its edges into it.
void paintWindowBackground (juce::Graphics& g, juce::Rectangle<float> window);

} // namespace staple
