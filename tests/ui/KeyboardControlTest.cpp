// Copied from eq1's tests/plugin/KeyboardControlTest.cpp (eq1 at dbb1127), its cases on sliders' keys
// (KeyboardSlider), against plugkit's test editor and its two Knobs: undo steps are counted (TestProcessor.h's
// EditSteps) and not undone, and Threshold's, Gain Scale's and the popovers' cases, and those of eq1's editor-wide keys
// (KeyboardControl) and EQ display, stay in eq1; listed in eq1#231.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/staple/LookAndFeel.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>

using Catch::Matchers::WithinAbs;
using OpenEditor = harness::OpenEditor<plugkit::test::TestProcessor>;

namespace
{

const juce::KeyPress left (juce::KeyPress::leftKey), right (juce::KeyPress::rightKey), up (juce::KeyPress::upKey),
    down (juce::KeyPress::downKey);

// The test editor, with its undo steps counted.
struct EveryControl : OpenEditor
{
    plugkit::test::EditSteps history { processor };

    EveryControl() { settle(); }

    std::vector<juce::Slider*> sliders()
    {
        return findAll<juce::Slider> ([] (juce::Slider& s) { return s.isShowing() && s.isEnabled(); });
    }
};

} // namespace

namespace
{
void checkArrowSteps (OpenEditor& host, const std::vector<juce::Slider*>& sliders)
{
    for (auto* slider : sliders)
    {
        CAPTURE (slider->getName());
        slider->grabKeyboardFocus();
        REQUIRE (slider->hasKeyboardFocus (false));
        const auto position = [slider] { return slider->valueToProportionOfLength (slider->getValue()); };
        const double start = position();

        CHECK (host.press (down));
        CHECK_THAT (position(), WithinAbs (std::max (0.0, start - 0.01), 1.0e-4));
        const double lowered = position();
        CHECK (host.press (harness::withShift (right)));
        CHECK_THAT (position(), WithinAbs (lowered + 0.002, 1.0e-4));
        CHECK (host.press (up));
        CHECK_THAT (position(), WithinAbs (std::min (1.0, lowered + 0.012), 1.0e-4));
        CHECK (host.press (harness::withShift (left)));
        CHECK_THAT (position(), WithinAbs (std::min (1.0, lowered + 0.012) - 0.002, 1.0e-4));

        // At the top of its range, a step up stays there.
        slider->setValue (slider->getMaximum(), juce::sendNotificationSync);
        host.press (right);
        CHECK_THAT (position(), WithinAbs (1.0, 1.0e-4));
    }
}
} // namespace

TEST_CASE ("Arrow keys step every slider 1% of its range, 0.2% with Shift, within its range")
{
    EveryControl host;
    // Frequency and Gain.
    const auto sliders = host.sliders();
    CHECK (sliders.size() == 2);
    checkArrowSteps (host, sliders);
}

TEST_CASE ("Each arrow press on a slider is one undo step, and so is a held key with its repeats")
{
    EveryControl host;
    auto& history = host.history;
    for (auto* slider : host.sliders())
    {
        CAPTURE (slider->getName());
        slider->grabKeyboardFocus();
        const int steps = history.count();

        host.press (down);
        host.press (down);
        CHECK (history.count() == steps + 2);
        for (int repeat = 0; repeat < 5; ++repeat)
            host.hold (up);
        host.release();
        CHECK (history.count() == steps + 3);
    }
}

TEST_CASE ("A held arrow key's undo step ends when the slider loses focus")
{
    EveryControl host;
    auto& history = host.history;
    auto sliders = host.sliders();
    REQUIRE (sliders.size() == 2);
    sliders[0]->grabKeyboardFocus();
    const int steps = history.count();
    host.hold (down);
    host.hold (down);
    sliders[1]->grabKeyboardFocus();
    CHECK (history.count() == steps + 1);
}

TEST_CASE ("The focus ring is hidden until an arrow key steps a slider, and hides again when told to")
{
    EveryControl host;
    auto* staple = dynamic_cast<staple::LookAndFeel*> (&host.editor->getLookAndFeel());
    REQUIRE (staple != nullptr);
    CHECK_FALSE (staple->isFocusRingShown());

    auto* slider = host.sliders().front();
    slider->grabKeyboardFocus();
    REQUIRE (slider->hasKeyboardFocus (false));
    CHECK_FALSE (staple->isFocusRingShown()); // focus alone, as after a click, doesn't show it
    host.press (up);
    CHECK (staple->isFocusRingShown());

    staple->showFocusRing (false);
    CHECK_FALSE (staple->isFocusRingShown());
}
