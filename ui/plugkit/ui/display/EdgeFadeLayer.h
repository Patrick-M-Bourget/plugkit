#pragma once

// Copied from eq1's plugin/display/EdgeFadeLayer.h (eq1 at dbb1127), in namespace plugkit::ui::display, the four fade
// distances an EdgeFades argument rather than eq1's tokens::layout::fade*.

#include "DisplayGeometry.h"

#include <array>
#include <functional>

namespace plugkit::ui::display
{

// How far in from each edge of a display its fade reaches, in logical pixels. Each display has its own.
struct EdgeFades
{
    float top = 0.0f, bottom = 0.0f, left = 0.0f, right = 0.0f;
};

// The display's edges dissolve into what lies behind it, as the handoff's mask does: whatever the
// display draws under the fades shows fully inside and not at all at each edge, over its
// EdgeFades, and where two edges' fades meet they multiply. Drawing what lies behind over the display
// with the opposite alpha looks the same and costs a few small images a frame.
struct EdgeFadeOverlay
{
    // One edge's fade, in device pixels: its image and where its top left sits from the display's.
    struct Strip
    {
        juce::Image image;
        juce::Point<int> at;
    };
    std::array<Strip, 4> strips; // top, bottom, left, right; they don't overlap
    float scale = 1.0f;          // device pixels per logical pixel
};

// paintBehind's drawing (in the display's logical pixels) in the fades only, at scale, each strip
// whole device pixels, so it is drawn back pixel for pixel at any scale; its alpha is 1 at each edge
// and 0 where the fades end.
EdgeFadeOverlay edgeFadeOverlay (const DisplayGeometry& geometry, const EdgeFades& fades, float scale, const std::function<void (juce::Graphics&)>& paintBehind);

// Draws the overlay over the grid, the Analyzer and the curves; the handles, labels and the Display
// Range chip go over it, unfaded.
void paintEdgeFades (juce::Graphics& g, const EdgeFadeOverlay& overlay);

} // namespace plugkit::ui::display
