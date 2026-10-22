// New in plugkit, lifted from eq1's plugin/OutputPopover.cpp (eq1 at dbb1127): OutputPopover's openFrom and tick, as
// OutputPopoverFrame's, and its OutputToggle; listed in eq1#231.

#include "OutputPopoverFrame.h"

#include "staple/Fonts.h"
#include "staple/Light.h"
#include "staple/Painting.h"
#include "staple/Tokens.h"

#include <algorithm>

namespace plugkit::ui
{

namespace
{
namespace colour = staple::tokens::colour;
namespace size = staple::tokens::size;
} // namespace

OutputToggle::OutputToggle (const juce::String& title, std::optional<staple::Icon> i, const juce::String& letter)
    : juce::Button (title), icon (i)
{
    setClickingTogglesState (true);
    setHasFocusOutline (true);
    setMouseClickGrabsKeyboardFocus (false);
    setTitle (title);
    setButtonText (letter);
}

void OutputToggle::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const bool on = getToggleState();
    const auto bounds = getLocalBounds().toFloat();
    if (on)
    {
        g.setColour (staple::lit (colour::fill2, highlighted, down));
        g.fillRoundedRectangle (bounds, size::r2);
    }
    const auto ink = (on || highlighted || down ? colour::text1 : colour::text3).withMultipliedAlpha (staple::enabledAlpha (*this));
    if (icon)
        staple::drawIcon (g, *icon, bounds.withSizeKeepingCentre (15.0f, 15.0f), ink);
    else
    {
        g.setFont (staple::font (size::fs4, staple::Weight::semiBold));
        g.setColour (ink);
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }
}

OutputPopoverFrame::OutputPopoverFrame() : EditorTick::Client (static_cast<juce::Component&> (*this)) {}

OutputPopoverFrame::~OutputPopoverFrame() = default;

void OutputPopoverFrame::openFrom (juce::Component& readout)
{
    followProcessor();
    open (readout, Placement::above);
    if (auto* layer = getParentComponent(); layer != nullptr && isOpen())
    {
        const auto anchor = layer->getLocalArea (&readout, readout.getLocalBounds());
        setTopLeftPosition (std::max (-getCardBounds().getX(), anchor.getRight() - getCardBounds().getRight()), getY());
    }
}

void OutputPopoverFrame::tick (double)
{
    if (isOpen())
        followProcessor();
}

} // namespace plugkit::ui
