// New in plugkit, lifted from eq1's plugin/FooterBar.cpp (eq1 at dbb1127): its OutputReadout's look and behaviour, as
// FooterReadout's; listed in eq1#231.

#include "FooterReadout.h"

#include "staple/Tokens.h"

namespace plugkit::ui
{

namespace
{
namespace colour = staple::tokens::colour;
} // namespace

FooterReadout::FooterReadout (const juce::String& title, const juce::String& l, const juce::String& tooltip)
    : staple::TextChip ({}, Look::plain), label (l)
{
    setName (title);
    setTitle (title);
    setTextIsValue (true);
    setTooltip (tooltip);
    setInk (colour::text1);
}

void FooterReadout::setNotApplied (bool n)
{
    notApplied = n;
    setInk (n ? colour::text4 : colour::text1);
}

void FooterReadout::setOpen (bool o)
{
    open = o;
    repaint();
}

void FooterReadout::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    if (open)
    {
        g.setColour (colour::fill2);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), staple::tokens::size::r2);
    }
    staple::TextChip::paintButton (g, highlighted && ! open, down && ! open);
}

} // namespace plugkit::ui
