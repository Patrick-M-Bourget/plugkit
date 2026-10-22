// New in plugkit, lifted from eq1's plugin/FooterBar.cpp (eq1 at dbb1127): its OutputReadout and outputReadoutText,
// taking Output Gain's silent bottom; listed in eq1#231.

#include "OutputReadout.h"

#include "ValueText.h"
#include "staple/Tokens.h"

#include <cmath>

namespace plugkit::ui
{

namespace
{
namespace colour = staple::tokens::colour;
} // namespace

juce::String outputReadoutText (double db, double silentDb)
{
    const bool silent = ! std::isfinite (db) || db <= silentDb;
    return valueText::signedDb (silent ? -std::numeric_limits<double>::infinity() : db, { .plusAtZero = true });
}

OutputReadout::OutputReadout() : staple::TextChip (outputReadoutText (0.0), Look::plain)
{
    setName ("Output");
    setTitle ("Output");
    setTextIsValue (true);
    setTooltip ("Output: Output Gain, with Auto Gain's estimate while Auto Gain is on. Click for the output controls");
    setInk (colour::text1);
}

void OutputReadout::setNotApplied (bool n)
{
    notApplied = n;
    setInk (n ? colour::text4 : colour::text1);
}

void OutputReadout::setOpen (bool o)
{
    open = o;
    repaint();
}

void OutputReadout::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    if (open)
    {
        g.setColour (colour::fill2);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), staple::tokens::size::r2);
    }
    staple::TextChip::paintButton (g, highlighted && ! open, down && ! open);
}

} // namespace plugkit::ui
