// Copied from eq1's plugin/HeaderBar.cpp (eq1 at dbb1127), in namespace plugkit::ui: HeaderBar's wordmark, its layout
// and its placing of the Preset group (PresetBar) and the right group, taking the Session and the plugin's wordmark.
// The right group's buttons, their layout and their following of the side and the undo history are CompareGroup's
// (CompareGroup.cpp), so this has no tick of its own.

#include "HeaderBar.h"

#include "Session.h"
#include "staple/Fonts.h"

namespace plugkit::ui
{

namespace
{
namespace colour = staple::tokens::colour;
namespace size = staple::tokens::size;

constexpr int sidePadding = 6, groupGap = 16;

juce::Font wordmarkFont() { return staple::font (size::fs5, staple::Weight::semiBold).withExtraKerningFactor (-0.01f); }
} // namespace

HeaderBar::HeaderBar (Session& session, juce::String wordmark) : name (std::move (wordmark)), presetBar (session), compareGroup (session)
{
    presetBar.onEdit = [this] { compareGroup.showUndoState(); };
    presetBar.onIdealWidthChange = [this] { resized(); };
    addAndMakeVisible (presetBar);
    addAndMakeVisible (compareGroup);
    // Left to right.
    int order = 0;
    for (juce::Component* child : std::initializer_list<juce::Component*> { &presetBar, &compareGroup })
        child->setExplicitFocusOrder (++order);
}

void HeaderBar::paint (juce::Graphics& g)
{
    g.setFont (wordmarkFont());
    g.setColour (colour::text1);
    g.drawText (name, wordmarkBounds, juce::Justification::centredLeft, false);
}

void HeaderBar::resized()
{
    auto row = getLocalBounds().reduced (sidePadding, 0);
    // The Preset group as wide as it wants, short of crowding the side groups.
    const int rightGroupWidth = compareGroup.getIdealWidth();
    const int roomForCentre = row.getWidth() - 2 * groupGap - 2 * rightGroupWidth;
    constexpr int smallestCentre = 2 * (PresetBar::stepButtonSize + PresetBar::gap) + PresetNameButton::minimumWidth;
    const int centreWidth = juce::jmax (smallestCentre, juce::jmin (presetBar.getIdealWidth(), roomForCentre));
    const int side = (row.getWidth() - centreWidth - 2 * groupGap) / 2;
    auto left = row.removeFromLeft (side);
    wordmarkBounds = left.withWidth (juce::jmin (left.getWidth(), staple::textWidth (wordmarkFont(), name) + 1));
    row.removeFromLeft (groupGap);
    presetBar.setBounds (row.removeFromLeft (centreWidth));
    row.removeFromLeft (groupGap);
    // CompareGroup lays its buttons out from its right edge.
    compareGroup.setBounds (row);
}

} // namespace plugkit::ui
