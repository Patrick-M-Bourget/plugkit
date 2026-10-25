// New in plugkit, lifted from eq1's plugin/FooterBar.cpp (eq1 at dbb1127): its OutputReadout and outputReadoutText,
// taking Output Gain's silent bottom; listed in eq1#231.

#include "OutputReadout.h"

#include "ValueText.h"

#include <cmath>

namespace plugkit::ui
{

juce::String outputReadoutText (double db, double silentDb)
{
    const bool silent = ! std::isfinite (db) || db <= silentDb;
    return valueText::signedDb (silent ? -std::numeric_limits<double>::infinity() : db, { .plusAtZero = true });
}

OutputReadout::OutputReadout()
    : FooterReadout ("Output", "OUT", "Output: Output Gain, with Auto Gain's estimate while Auto Gain is on. Click for the output controls")
{
    setButtonText (outputReadoutText (0.0));
}

} // namespace plugkit::ui
