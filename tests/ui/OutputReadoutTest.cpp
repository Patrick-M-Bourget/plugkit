// Copied from eq1's tests/plugin/FooterTest.cpp (eq1 at dbb1127): its Output readout and output popover cases, against
// plugkit's test processor and its editor (TestProcessor.h), the readout and a popover on the frame put in its footer
// as a plugin's footer holds them; listed in eq1#231.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/OutputPopoverFrame.h"
#include "plugkit/ui/OutputReadout.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>

namespace
{

const juce::String minus = juce::String::charToString (0x2212);
const juce::KeyPress escape (juce::KeyPress::escapeKey);

// An output popover on the frame, standing in for a plugin's: eq1's card width, and one toggle, eq1's Output Meter's,
// which shows what the processor holds (meterShown, standing in for eq1's isOutputMeterShown) each time the frame has
// it follow the processor.
struct TestPopover final : plugkit::ui::OutputPopoverFrame
{
    TestPopover()
    {
        setCardSize (176, 120);
        addAndMakeVisible (meter);
        meter.setBounds (getCardBounds().reduced (12).removeFromBottom (30).removeFromLeft (48));
        followProcessor();
    }

    void followProcessor() override { meter.setToggleState (meterShown, juce::dontSendNotification); }

    bool meterShown = true;
    plugkit::ui::OutputToggle meter { "Output Meter", staple::Icon::meter };
};

// The test editor with an Output readout and the popover in its footer, left of its UI Scale menu, wired as eq1's
// FooterBar wires them: a click on the readout opens or closes the popover, the readout fill2 while it is open, and
// the popover back in the footer, hidden, once closed.
struct Footer : harness::OpenEditor<plugkit::test::TestProcessor>
{
    juce::Component& footer = *harness::findChild<juce::Component> (*editor, [] (juce::Component& c) { return c.getTitle() == "Footer"; });
    plugkit::ui::OutputReadout output;
    TestPopover popover;

    Footer()
    {
        output.onClick = [this] {
            if (popover.isOpen())
                popover.close();
            else
            {
                popover.openFrom (output);
                output.setOpen (popover.isOpen());
            }
        };
        popover.onClose = [this] {
            output.setOpen (false);
            footer.addChildComponent (popover);
        };
        footer.addAndMakeVisible (output);
        footer.addChildComponent (popover);
        output.setBounds (footer.getLocalBounds().withTrimmedRight (44 + 18).removeFromRight (70).withSizeKeepingCentre (70, 28));
    }

    ~Footer()
    {
        popover.onClose = nullptr;
        popover.close();
    }

    void openPopover()
    {
        output.onClick();
        REQUIRE (popover.isOpen());
    }
};

// The readout as it paints now, hovered or pressed or neither.
juce::Image paintOf (plugkit::ui::OutputReadout& readout, bool highlighted = false, bool down = false)
{
    return harness::paintImage (readout.getWidth(), readout.getHeight(), [&] (juce::Graphics& g) { readout.paintButton (g, highlighted, down); });
}

// What is behind its text: a pixel 3 px in from its left edge, left of the text's padding.
juce::Colour behindText (const juce::Image& image) { return image.getPixelAt (3, image.getHeight() / 2); }

// Its text's strongest ink: the highest alpha it paints.
int inkAlpha (const juce::Image& image)
{
    int strongest = 0;
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            strongest = std::max (strongest, static_cast<int> (image.getPixelAt (x, y).getAlpha()));
    return strongest;
}

} // namespace

TEST_CASE ("The Output readout's text is a signed level to one decimal, or -inf dB at Output Gain's silent bottom")
{
    using plugkit::ui::outputReadoutText;
    CHECK (outputReadoutText (0.0) == "+0.0 dB");
    CHECK (outputReadoutText (2.25) == "+2.3 dB");
    CHECK (outputReadoutText (-1.84) == minus + "1.8 dB");
    CHECK (outputReadoutText (-6.0) == minus + "6.0 dB");
    // A range with no silent bottom (comp1's -36 dB) reads its bottom as a level.
    CHECK (outputReadoutText (-36.0) == minus + "36.0 dB");
    CHECK (outputReadoutText (-std::numeric_limits<double>::infinity()) == minus + "inf dB");
    CHECK (outputReadoutText (std::numeric_limits<double>::quiet_NaN()) == minus + "inf dB");
    // At and below the silent bottom eq1's -80 dB is.
    CHECK (outputReadoutText (-80.0, -80.0) == minus + "inf dB");
    CHECK (outputReadoutText (-90.0, -80.0) == minus + "inf dB");
    CHECK (outputReadoutText (-79.9, -80.0) == minus + "79.9 dB");
}

TEST_CASE ("The Output readout reads as its value, draws text4 while not applied, and fill2 behind it while its popover is open")
{
    Footer host;
    auto& readout = host.output;
    REQUIRE (readout.getWidth() == 70);
    CHECK (readout.getTitle() == "Output");
    CHECK (readout.getButtonText() == "+0.0 dB");
    CHECK (readout.getTooltip().startsWith ("Output"));
    auto* handler = readout.getAccessibilityHandler();
    REQUIRE (handler != nullptr);
    REQUIRE (handler->getValueInterface() != nullptr);
    CHECK (handler->getValueInterface()->getCurrentValueAsString() == "+0.0 dB");

    // Applied, in text1 (opaque); not applied, in text4 (about a third of it).
    CHECK_FALSE (readout.isNotApplied());
    CHECK (inkAlpha (paintOf (readout)) > 0xC0);
    readout.setNotApplied (true);
    CHECK (readout.isNotApplied());
    CHECK (inkAlpha (paintOf (readout)) < 0x60);
    readout.setNotApplied (false);
    CHECK (inkAlpha (paintOf (readout)) > 0xC0);

    // Closed: nothing behind its text, and fill1 while hovered or pressed.
    CHECK_FALSE (readout.isOpen());
    CHECK (behindText (paintOf (readout)).getAlpha() == 0);
    CHECK (behindText (paintOf (readout, true)) == staple::tokens::colour::fill1);
    // Open: fill2, hovered and pressed alike.
    readout.setOpen (true);
    CHECK (readout.isOpen());
    for (const bool lit : { false, true })
    {
        CAPTURE (lit);
        CHECK (behindText (paintOf (readout, lit, lit)) == staple::tokens::colour::fill2);
    }
    readout.setOpen (false);
    CHECK (behindText (paintOf (readout)).getAlpha() == 0);
}

TEST_CASE ("The output popover opens above the Output readout, right-aligned to it, and closes on a second click, Escape or a click outside")
{
    Footer host;
    CHECK_FALSE (host.popover.isOpen());
    host.openPopover();
    CHECK (host.output.isOpen());
    // Popping in about where it ends up, from the first frame.
    harness::checkPopInStart (host.popover, host.popover.getCardBounds().toFloat() + host.popover.getPosition().toFloat());
    REQUIRE (host.settleUntil ([&] { return ! host.popover.isTransformed(); })); // popped in
    CHECK (host.popover.isVisible());
    const auto card = host.editor->getLocalArea (&host.popover, host.popover.getCardBounds());
    const auto readout = host.inEditor (host.output);
    CHECK (card.getBottom() < readout.getY());
    CHECK (card.getRight() == readout.getRight());

    host.output.onClick();
    CHECK_FALSE (host.popover.isOpen());
    CHECK_FALSE (host.output.isOpen());

    host.openPopover();
    host.popover.meter.grabKeyboardFocus();
    REQUIRE (host.popover.meter.hasKeyboardFocus (false));
    CHECK (host.press (escape));
    CHECK_FALSE (host.popover.isOpen());
    CHECK_FALSE (host.output.isOpen());

    host.openPopover();
    host.popover.mouseDown (harness::mouseEvent (host.footer, { 20.0f, 20.0f }, juce::ModifierKeys::leftButtonModifier));
    CHECK_FALSE (host.popover.isOpen());
    // And it opens again.
    host.openPopover();
}

TEST_CASE ("The output popover opened from a readout near the window's left edge keeps its card inside the window")
{
    Footer host;
    host.output.setBounds (host.output.getBounds().withX (4));
    host.openPopover();
    // In the overlay layer it opens in, the editor's scaled content.
    auto* layer = host.popover.getParentComponent();
    REQUIRE (layer != nullptr);
    const auto card = host.popover.getCardBounds() + host.popover.getPosition();
    CHECK (card.getX() == 0);
    CHECK (card.getBottom() < layer->getLocalArea (&host.output, host.output.getLocalBounds()).getY());
}

TEST_CASE ("The output popover follows the processor only while it is open, and shows the processor's state as it opens")
{
    Footer host;
    auto& toggle = host.popover.meter;
    host.openPopover();
    REQUIRE (toggle.getToggleState());
    // While open, on the editor's tick.
    host.popover.meterShown = false;
    REQUIRE (host.settleUntil ([&] { return ! toggle.getToggleState(); }));
    host.popover.meterShown = true;
    REQUIRE (host.settleUntil ([&] { return toggle.getToggleState(); }));
    host.output.onClick();
    REQUIRE_FALSE (host.popover.isOpen());
    // A restored session hides the Output Meter while the popover is closed: nothing follows it until
    // the popover opens.
    host.popover.meterShown = false;
    host.settle (300);
    CHECK (toggle.getToggleState());
    host.openPopover();
    CHECK_FALSE (toggle.getToggleState());
}

TEST_CASE ("An output toggle toggles on a click, filled with fill2 while on, its icon or letter in text1, else text3")
{
    Footer host;
    plugkit::ui::OutputToggle icon ("Phase Invert", staple::Icon::phaseInvert), letter ("Auto Gain", std::nullopt, "A");
    for (auto* toggle : { &icon, &letter })
    {
        host.footer.addAndMakeVisible (*toggle);
        toggle->setBounds (0, 0, 48, 30);
    }
    CHECK (icon.getTitle() == "Phase Invert");
    CHECK (letter.getTitle() == "Auto Gain");
    CHECK (letter.getButtonText() == "A");
    const auto paint = [] (plugkit::ui::OutputToggle& toggle) {
        return harness::paintImage (toggle.getWidth(), toggle.getHeight(), [&] (juce::Graphics& g) { toggle.paintButton (g, false, false); });
    };
    for (auto* toggle : { &icon, &letter })
    {
        CAPTURE (toggle->getTitle());
        CHECK_FALSE (toggle->getToggleState());
        auto off = paint (*toggle);
        CHECK (behindText (off).getAlpha() == 0);
        const int offInk = inkAlpha (off);
        CHECK (toggle->getClickingTogglesState());
        toggle->setToggleState (true, juce::dontSendNotification);
        const auto on = paint (*toggle);
        CHECK (behindText (on) == staple::tokens::colour::fill2);
        // text3 is dimmer than text1.
        CHECK (inkAlpha (on) > offInk);
        CHECK (offInk > 0);
    }
}
