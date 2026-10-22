// Copied from eq1's plugin/OutputPopover.cpp (eq1 at dbb1127): its OutputPanSlider, and the readout line its
// OutputPopover paints beneath Output Pan, as PopoverSlider's; listed in eq1#231.

#include "PopoverSlider.h"

#include "staple/Fonts.h"
#include "staple/Light.h"
#include "staple/Painting.h"
#include "staple/Tokens.h"

#include <algorithm>
#include <cmath>

namespace plugkit::ui
{

namespace
{
namespace colour = staple::tokens::colour;
namespace size = staple::tokens::size;
} // namespace

PopoverSlider::PopoverSlider (Fill f, const juce::String& title) : KeyboardSlider (title), fill (f)
{
    setTitle (title);
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

PopoverSlider::Readout PopoverSlider::readoutNow()
{
    const double value = getValue();
    return readout != nullptr ? readout (value) : Readout { getTitle(), {}, getTextFromValue (value) };
}

juce::Rectangle<int> PopoverSlider::readoutArea() const
{
    return { 0, trackHeight + readoutGap, getWidth(), readoutHeight };
}

void PopoverSlider::paint (juce::Graphics& g)
{
    const float alpha = staple::enabledAlpha (*this);
    const float width = static_cast<float> (getWidth());
    const float proportion = static_cast<float> (juce::jlimit (0.0, 1.0, valueToProportionOfLength (getValue())));
    const float valueX = width * proportion;
    const float fillFrom = fill == Fill::fromCentre ? width * 0.5f : 0.0f;

    g.setColour (colour::fill2.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle ({ 0.0f, 4.0f, width, 6.0f }, 3.0f);
    if (fill == Fill::fromCentre)
    {
        g.setColour (colour::line3.withMultipliedAlpha (alpha));
        g.fillRect (juce::Rectangle<float> (fillFrom - 0.5f, 2.0f, 1.0f, 10.0f));
    }
    g.setColour (colour::text2.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle ({ std::min (fillFrom, valueX), 4.0f, std::abs (valueX - fillFrom), 6.0f }, 3.0f);

    const juce::Rectangle<float> thumb { juce::jlimit (0.0f, width - 6.0f, valueX - 3.0f), 1.0f, 6.0f, 12.0f };
    staple::drawSoftShadow (g, thumb, 2.0f, staple::tokens::shadow::panThumb);
    g.setColour (colour::text1.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (thumb, 2.0f);

    const auto texts = readoutNow();
    const auto line = readoutArea();
    g.setFont (staple::font (size::fs1));
    g.setColour (colour::text3.withMultipliedAlpha (alpha));
    g.drawText (texts.left, line, juce::Justification::centredLeft, false);
    g.drawText (texts.centre, line, juce::Justification::centred, false);
    g.drawText (texts.right, line, juce::Justification::centredRight, false);
}

bool PopoverSlider::hitTest (int, int y)
{
    return y < trackHeight;
}

double PopoverSlider::landed (double value) const
{
    if (dragStep > 0.0 && std::isfinite (value))
        value = std::round (value / dragStep) * dragStep;
    return juce::jlimit (getMinimum(), getMaximum(), value);
}

double PopoverSlider::valueAt (float x)
{
    const double proportion = juce::jlimit (0.0, 1.0, static_cast<double> (x) / std::max (1, getWidth()));
    return landed (proportionOfLengthToValue (proportion));
}

double PopoverSlider::valueDraggedBy (double from, float pixels, bool)
{
    const double moved = static_cast<double> (pixels) / std::max (1, getWidth());
    return landed (proportionOfLengthToValue (juce::jlimit (0.0, 1.0, valueToProportionOfLength (from) + moved)));
}

void PopoverSlider::mouseDown (const juce::MouseEvent& e)
{
    if (isEnabled() && e.mods.isLeftButtonDown())
        startMouseDrag (e, DragAxis::horizontal, valueAt (e.position.x));
}

void PopoverSlider::mouseDrag (const juce::MouseEvent& e)
{
    continueMouseDrag (e);
}

void PopoverSlider::mouseUp (const juce::MouseEvent&)
{
    endMouseDrag();
}

} // namespace plugkit::ui
