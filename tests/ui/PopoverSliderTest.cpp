// Copied from eq1's tests/plugin/FooterTest.cpp (eq1 at dbb1127): its Output Pan cases, as the popover slider's, against
// plugkit's test processor and its editor (TestProcessor.h), with the slider in its body; and the level a slider filled
// from the left is for, which eq1 has none of. Listed in eq1#231.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/PopoverSlider.h"
#include "plugkit/ui/staple/Tokens.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

using Catch::Matchers::WithinAbs;
using plugkit::ui::PopoverSlider;

namespace
{

constexpr double minusInf = -std::numeric_limits<double>::infinity();
namespace colour = staple::tokens::colour;
// The fill, text2 over the track's fill2.
const juce::Colour filled = colour::fill2.overlaidWith (colour::text2);

// The test editor with a popover slider, 200 px wide, in its body, attached to the test processor's Gain (-30 to
// +30 dB, linear) unless left unattached.
struct Host : harness::OpenEditor<plugkit::test::TestProcessor>
{
    PopoverSlider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;

    explicit Host (PopoverSlider::Fill fill, bool attached = true) : slider (fill, "Gain")
    {
        if (attached)
            attachment = std::make_unique<juce::SliderParameterAttachment> (parameter (plugkit::test::TestProcessor::gainId), slider);
        showing ("Body").addAndMakeVisible (slider);
        slider.setBounds (10, 200, 200, PopoverSlider::height);
    }

    int undoSteps() { return processor.editHistory().undoSteps(); }

    // A press at x on the track, a drag to x + dx and a release, as one gesture; clicks 2 for a double-click's second
    // press.
    void drag (float x, float dx = 0.0f, int clicks = 1)
    {
        const juce::Point<float> from { x, 7.0f }, to { x + dx, 7.0f };
        const juce::ModifierKeys left (juce::ModifierKeys::leftButtonModifier);
        slider.mouseDown (harness::mouseEvent (slider, from, left, from, clicks));
        if (dx != 0.0f)
            slider.mouseDrag (harness::mouseEvent (slider, to, left, from, clicks));
        slider.mouseUp (harness::mouseEvent (slider, to, {}, from, clicks));
    }

    void doubleClick (float x)
    {
        drag (x);
        drag (x, 0.0f, 2);
        slider.mouseDoubleClick (harness::mouseEvent (slider, { x, 7.0f }, {}, juce::Point<float> { x, 7.0f }, 2));
    }

    juce::Image paint()
    {
        return harness::paintImage (slider.getWidth(), slider.getHeight(), [this] (juce::Graphics& g) { slider.paint (g); });
    }
};

// comp1's Dry and Wet range (its plugin/Parameters.cpp, levelRange): -inf (Off) to +12 dB, the travel even in dB from
// -52 dB, its bottom Off.
juce::NormalisableRange<double> levelRange()
{
    return { minusInf,
             12.0,
             [] (double, double, double normalised) { return normalised <= 0.0 ? minusInf : -52.0 + 64.0 * normalised; },
             [] (double, double, double value) { return value <= -52.0 ? 0.0 : std::min (1.0, (value + 52.0) / 64.0); },
             [] (double, double, double value) { return value <= -52.0 ? minusInf : value; } };
}

bool isOff (double value) { return std::isinf (value) && value < 0.0; }

// Whether a pixel is colour, within the rounding of drawing it translucent on a transparent image.
bool near (juce::Colour pixel, juce::Colour colour)
{
    const auto off = [] (juce::uint8 a, juce::uint8 b) { return std::abs (static_cast<int> (a) - static_cast<int> (b)) > 3; };
    return ! (off (pixel.getAlpha(), colour.getAlpha()) || off (pixel.getRed(), colour.getRed()) || off (pixel.getGreen(), colour.getGreen())
              || off (pixel.getBlue(), colour.getBlue()));
}

// The strongest ink in rows from top to bottom: the highest alpha painted there.
int inkAlpha (const juce::Image& image, int top, int bottom)
{
    int strongest = 0;
    for (int y = top; y < bottom; ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            strongest = std::max (strongest, static_cast<int> (image.getPixelAt (x, y).getAlpha()));
    return strongest;
}

} // namespace

TEST_CASE ("A popover slider is a 14 px track over a 14 px readout line, 4 px apart, and only its track takes the mouse")
{
    Host host (PopoverSlider::Fill::fromCentre);
    auto& slider = host.slider;
    CHECK (PopoverSlider::height == 32);
    CHECK (slider.readoutArea() == juce::Rectangle<int> (0, 18, 200, 14));
    CHECK (slider.getSliderStyle() == juce::Slider::LinearHorizontal);
    CHECK (slider.getTextBoxPosition() == juce::Slider::NoTextBox);
    CHECK (slider.getTitle() == "Gain");
    CHECK (slider.hitTest (100, 0));
    CHECK (slider.hitTest (100, 13));
    CHECK_FALSE (slider.hitTest (100, 14));
    CHECK_FALSE (slider.hitTest (100, 25));
}

TEST_CASE ("A popover slider's click, drag and double-click set its parameter on whole drag steps, each one undo step")
{
    Host host (PopoverSlider::Fill::fromCentre);
    auto& slider = host.slider;
    slider.setDragStep (1.0);
    slider.setDoubleClickReturnValue (true, 0.0);
    slider.setArrowSteps (5.0, 1.0);
    const auto gain = [&host] { return host.value (plugkit::test::TestProcessor::gainId); };
    REQUIRE_THAT (gain(), WithinAbs (0.0, 1.0e-6));

    // A click 70.5% across: +12.3, landing on +12.
    host.drag (141.0f);
    CHECK_THAT (gain(), WithinAbs (12.0, 1.0e-4));
    CHECK (host.undoSteps() == 1);

    // A press and a drag 10 px right, 3 dB: one gesture.
    host.drag (140.0f, 10.0f);
    CHECK_THAT (gain(), WithinAbs (15.0, 1.0e-4));
    CHECK (host.undoSteps() == 2);

    // Past either end, it stops there.
    host.drag (190.0f, 300.0f);
    CHECK_THAT (gain(), WithinAbs (30.0, 1.0e-4));
    host.drag (10.0f, -300.0f);
    CHECK_THAT (gain(), WithinAbs (-30.0, 1.0e-4));
    CHECK (host.undoSteps() == 4);

    // The second press of a double-click sets the return value, inside its own gesture.
    host.doubleClick (60.0f);
    CHECK_THAT (gain(), WithinAbs (0.0, 1.0e-4));
    CHECK (host.undoSteps() == 6);

    slider.grabKeyboardFocus();
    CHECK (host.press (juce::KeyPress (juce::KeyPress::leftKey)));
    CHECK_THAT (gain(), WithinAbs (-5.0, 1.0e-4));
    CHECK (host.undoSteps() == 7);
    host.processor.editHistory().undo();
    CHECK_THAT (gain(), WithinAbs (0.0, 1.0e-4));
}

TEST_CASE ("A popover slider without a drag step lands anywhere, and a disabled one ignores the mouse")
{
    Host host (PopoverSlider::Fill::fromLeft);
    const auto gain = [&host] { return host.value (plugkit::test::TestProcessor::gainId); };
    host.drag (141.0f);
    CHECK_THAT (gain(), WithinAbs (12.3, 1.0e-3));

    host.slider.setEnabled (false);
    host.drag (50.0f, 20.0f);
    host.doubleClick (50.0f);
    CHECK_THAT (gain(), WithinAbs (12.3, 1.0e-3));
    CHECK (host.undoSteps() == 1);
}

TEST_CASE ("A level popover slider travels as its range does: Off at its left end, its top at its right")
{
    Host host (PopoverSlider::Fill::fromLeft, false);
    auto& slider = host.slider;
    slider.setNormalisableRange (levelRange());
    slider.setValue (minusInf, juce::dontSendNotification);
    REQUIRE (isOff (slider.getValue()));

    host.drag (100.0f);
    CHECK_THAT (slider.getValue(), WithinAbs (-20.0, 1.0e-9));
    host.drag (200.0f);
    CHECK_THAT (slider.getValue(), WithinAbs (12.0, 1.0e-9));
    host.drag (0.0f);
    CHECK (isOff (slider.getValue()));
    // From Off, a drag a quarter of the way along: -36 dB.
    host.drag (0.0f, 50.0f);
    CHECK_THAT (slider.getValue(), WithinAbs (-36.0, 1.0e-9));
    // Back past the left end: Off.
    host.drag (50.0f, -80.0f);
    CHECK (isOff (slider.getValue()));

    // With whole-dB steps, Off stays Off and the rest land on whole dB.
    slider.setDragStep (1.0);
    host.drag (101.0f);
    CHECK_THAT (slider.getValue(), WithinAbs (-20.0, 1.0e-9));
    host.drag (0.0f);
    CHECK (isOff (slider.getValue()));
}

TEST_CASE ("A popover slider fills from its centre past a centre tick, or from its left end, in text2 on its fill2 track")
{
    // At 0.75 of the travel: +15 dB.
    SECTION ("From the centre")
    {
        Host host (PopoverSlider::Fill::fromCentre);
        host.set (plugkit::test::TestProcessor::gainId, 15.0f);
        host.settle();
        const auto image = host.paint();
        CHECK (near (image.getPixelAt (125, 7), filled));
        CHECK (near (image.getPixelAt (50, 7), colour::fill2));
        // The centre tick, above the track, a pixel wide across two.
        CHECK (image.getPixelAt (100, 3).getAlpha() > 0);
        CHECK (image.getPixelAt (100, 3).getAlpha() <= colour::line3.getAlpha());
        CHECK (image.getPixelAt (102, 3).getAlpha() == 0);
        // The thumb, at the value.
        CHECK (near (image.getPixelAt (150, 7), colour::text1));

        host.set (plugkit::test::TestProcessor::gainId, -15.0f);
        host.settle();
        const auto left = host.paint();
        CHECK (near (left.getPixelAt (75, 7), filled));
        CHECK (near (left.getPixelAt (125, 7), colour::fill2));
    }
    SECTION ("From the left")
    {
        Host host (PopoverSlider::Fill::fromLeft);
        host.set (plugkit::test::TestProcessor::gainId, 15.0f);
        host.settle();
        const auto image = host.paint();
        CHECK (near (image.getPixelAt (20, 7), filled));
        CHECK (near (image.getPixelAt (125, 7), filled));
        CHECK (near (image.getPixelAt (175, 7), colour::fill2));
        CHECK (image.getPixelAt (100, 3).getAlpha() == 0); // no centre tick

        // Off: no fill.
        host.slider.setNormalisableRange (levelRange());
        host.slider.setValue (minusInf, juce::dontSendNotification);
        const auto off = host.paint();
        CHECK (near (off.getPixelAt (20, 7), colour::fill2));
        CHECK (near (off.getPixelAt (175, 7), colour::fill2));
    }
}

TEST_CASE ("A popover slider's readout line shows its title and value text, or its owner's texts, dimmed while disabled")
{
    Host host (PopoverSlider::Fill::fromLeft);
    auto& slider = host.slider;
    using Readout = PopoverSlider::Readout;
    CHECK (slider.readoutNow() == Readout { "Gain", {}, slider.getTextFromValue (0.0) });
    host.set (plugkit::test::TestProcessor::gainId, -6.0f);
    host.settle();
    CHECK (slider.readoutNow() == Readout { "Gain", {}, slider.getTextFromValue (-6.0) });

    // The readout line is drawn in its own row, in text3.
    const auto image = host.paint();
    const int ink = inkAlpha (image, slider.readoutArea().getY(), slider.readoutArea().getBottom());
    CHECK (ink > 0);
    CHECK (ink <= colour::text3.getAlpha() + 3);

    // An owner's texts (eq1's Output Pan's ends and readout).
    slider.readout = [] (double value) { return Readout { "L", value < 0.0 ? "6 L" : "Centre", "R" }; };
    CHECK (slider.readoutNow() == Readout { "L", "6 L", "R" });
    slider.readout = [] (double) { return Readout {}; };
    CHECK (inkAlpha (host.paint(), slider.readoutArea().getY(), slider.readoutArea().getBottom()) == 0);

    // Disabled, the track and its readout line are dimmed.
    slider.readout = nullptr;
    slider.setEnabled (false);
    const auto dimmed = host.paint();
    CHECK (inkAlpha (dimmed, slider.readoutArea().getY(), slider.readoutArea().getBottom()) < ink);
    CHECK (inkAlpha (dimmed, 0, PopoverSlider::trackHeight) < inkAlpha (image, 0, PopoverSlider::trackHeight));
}

TEST_CASE ("A popover slider reads to a screen reader as one slider, its title and value")
{
    Host host (PopoverSlider::Fill::fromLeft);
    host.slider.describe (host.parameter (plugkit::test::TestProcessor::gainId));
    auto* handler = host.slider.getAccessibilityHandler();
    REQUIRE (handler != nullptr);
    CHECK (handler->getRole() == juce::AccessibilityRole::slider);
    CHECK (handler->getTitle() == "Gain");
    REQUIRE (handler->getValueInterface() != nullptr);
    CHECK (handler->getValueInterface()->getCurrentValueAsString().contains ("dB"));
}
