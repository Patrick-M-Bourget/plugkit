// Copied from eq1's tests/plugin/KeyboardControlTest.cpp (eq1 at dbb1127), its cases on sliders' keys
// (KeyboardSlider) and its generic cases on the editor-wide keys (KeyboardControl), against plugkit's test editor
// (TestProcessor.h): its toggle (Phase Invert), button (Reset Gain) and ComboBox (Shape) stand in for eq1's Band
// Bypass, Delete, Detection Audition and Shape, and its Gain Knob's focus for the EQ display's. The cases on
// Threshold, Gain Scale, the popovers, the Bands, the Output Meter and the Tab order of eq1's editor stay in eq1;
// listed in eq1#231.

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

// The test editor.
struct EveryControl : OpenEditor
{
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
    auto& history = host.processor.editHistory();
    for (auto* slider : host.sliders())
    {
        CAPTURE (slider->getName());
        slider->grabKeyboardFocus();
        const double start = slider->getValue();
        const int steps = history.undoSteps();

        host.press (down);
        host.press (down);
        CHECK (history.undoSteps() == steps + 2);
        for (int repeat = 0; repeat < 5; ++repeat)
            host.hold (up);
        host.release();
        CHECK (history.undoSteps() == steps + 3);

        history.undo();
        history.undo();
        history.undo();
        CHECK_THAT (slider->getValue(), WithinAbs (start, 1.0e-3 * (slider->getMaximum() - slider->getMinimum())));
    }
}

TEST_CASE ("A held arrow key's undo step ends when the slider loses focus")
{
    EveryControl host;
    auto& history = host.processor.editHistory();
    auto sliders = host.sliders();
    REQUIRE (sliders.size() == 2);
    sliders[0]->grabKeyboardFocus();
    const int steps = history.undoSteps();
    host.hold (down);
    host.hold (down);
    sliders[1]->grabKeyboardFocus();
    CHECK (history.undoSteps() == steps + 1);
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

namespace
{
const juce::KeyPress tab (juce::KeyPress::tabKey), space (juce::KeyPress::spaceKey), returnKey (juce::KeyPress::returnKey),
    escape (juce::KeyPress::escapeKey);

staple::LookAndFeel& stapleOf (OpenEditor& host) { return dynamic_cast<staple::LookAndFeel&> (host.editor->getLookAndFeel()); }

// The first T named or titled name.
template <typename T>
T& named (OpenEditor& host, const juce::String& name)
{
    auto found = host.findAll<T> ([&name] (T& c) { return c.getName() == name || c.getTitle() == name; });
    INFO ("Nothing is named " << name);
    REQUIRE_FALSE (found.empty());
    return *found.front();
}

juce::Component& gainKnob (OpenEditor& host)
{
    auto sliders = host.findAll<juce::Slider> ([] (juce::Slider& s) { return s.getName() == "Gain"; });
    REQUIRE (sliders.size() == 1);
    return *sliders.front();
}

// What can be undone and redone, so Undo and Redo are enabled and Tab reaches them.
void undoAndRedoEnabled (OpenEditor& host)
{
    auto& gain = host.parameter (plugkit::test::TestProcessor::gainId);
    for (float value : { 0.4f, 0.6f })
    {
        gain.beginChangeGesture();
        gain.setValueNotifyingHost (value);
        gain.endChangeGesture();
    }
    host.processor.editHistory().undo();
    host.settle (300);
}
} // namespace

TEST_CASE ("Every control Tab reaches has a focus ring, which Tab shows")
{
    EveryControl host;
    undoAndRedoEnabled (host);
    const auto reached = juce::KeyboardFocusTraverser().getAllComponents (host.editor.get());
    CHECK (reached.size() >= 13u); // the Preset group's 3, the A/B group's 4, the 2 Knobs, the row's 3 and UI Scale
    for (auto* component : reached)
    {
        CAPTURE (component->getTitle(), component->getName());
        CHECK (component->hasFocusOutline());
    }
    gainKnob (host).grabKeyboardFocus();
    CHECK_FALSE (stapleOf (host).isFocusRingShown());
    CHECK (host.press (tab));
    CHECK (stapleOf (host).isFocusRingShown());
    CHECK (named<juce::Button> (host, "Phase Invert").hasKeyboardFocus (false));
    host.press (harness::withShift (tab));
    CHECK (gainKnob (host).hasKeyboardFocus (false));
}

TEST_CASE ("With nothing focused, Tab focuses the first control and Shift+Tab the last")
{
    // The editor's own keyPressed, as eq1's PluginEditor's (TestEditor's copy of it), not KeyboardControl's.
    EveryControl host;
    juce::Component::unfocusAllComponents();
    host.press (tab);
    CHECK (named<juce::Button> (host, "Previous Preset").hasKeyboardFocus (false));
    juce::Component::unfocusAllComponents();
    host.press (harness::withShift (tab));
    CHECK (named<juce::Button> (host, "UI Scale").hasKeyboardFocus (false));
}

TEST_CASE ("Space or Return toggles a toggle and presses a button, each press one undo step")
{
    using plugkit::test::TestProcessor;
    EveryControl host;
    auto& history = host.processor.editHistory();
    const int steps = history.undoSteps();
    auto& invert = named<juce::Button> (host, "Phase Invert");
    invert.grabKeyboardFocus();
    REQUIRE (invert.hasKeyboardFocus (false));

    CHECK (host.press (space));
    CHECK (host.value (TestProcessor::invertId) == 1.0f);
    CHECK (host.press (returnKey));
    CHECK (host.value (TestProcessor::invertId) == 0.0f);
    CHECK (history.undoSteps() == steps + 2);

    host.set (TestProcessor::gainId, 15.0f);
    auto& reset = named<juce::Button> (host, "Reset Gain");
    reset.grabKeyboardFocus();
    CHECK (host.press (returnKey));
    CHECK (host.value (TestProcessor::gainId) == 0.0f);
    CHECK (history.undoSteps() == steps + 3);
}

TEST_CASE ("A button stays down while Space is held on it, and acts on its release")
{
    using plugkit::test::TestProcessor;
    EveryControl host;
    host.set (TestProcessor::gainId, 15.0f);
    auto& reset = named<juce::Button> (host, "Reset Gain");
    reset.grabKeyboardFocus();
    REQUIRE (reset.hasKeyboardFocus (false));
    host.hold (space);
    host.hold (space);
    CHECK (reset.isDown());
    CHECK (host.value (TestProcessor::gainId) == 15.0f);
    host.release();
    CHECK_FALSE (reset.isDown());
    CHECK (host.value (TestProcessor::gainId) == 0.0f);
    CHECK (host.processor.editHistory().undoSteps() == 1);
}

TEST_CASE ("Up and down step a ComboBox to the previous or next item; a held key is one undo step")
{
    using plugkit::test::TestProcessor;
    EveryControl host;
    auto& history = host.processor.editHistory();
    auto& shape = named<juce::ComboBox> (host, "Shape");
    host.set (TestProcessor::shapeId, 1.0f); // Low Shelf
    host.settle();
    shape.grabKeyboardFocus();
    REQUIRE (shape.hasKeyboardFocus (false));
    const int steps = history.undoSteps();

    CHECK (host.press (down));
    CHECK (host.value (TestProcessor::shapeId) == 2.0f);
    CHECK (host.press (up));
    CHECK (host.value (TestProcessor::shapeId) == 1.0f);
    CHECK (history.undoSteps() == steps + 2);
    for (int repeat = 0; repeat < 3; ++repeat)
        host.hold (down);
    host.release();
    CHECK (host.value (TestProcessor::shapeId) == 4.0f);
    CHECK (history.undoSteps() == steps + 3);
    CHECK (stapleOf (host).isFocusRingShown());
    history.undo();
    CHECK (host.value (TestProcessor::shapeId) == 1.0f);
}

TEST_CASE ("Clicking Undo, Redo, A/B Compare, Copy or Presets leaves keyboard focus where it was")
{
    EveryControl host;
    undoAndRedoEnabled (host);
    auto& gain = gainKnob (host);
    gain.grabKeyboardFocus();
    const auto button = [&host] (const juce::String& name) {
        return host.findAll<juce::Button> ([&name] (juce::Button& b) { return b.getName() == name; }).front();
    };
    // A click runs a button's onClick, and takes keyboard focus only if the button lets it (a real
    // click, through the OS window, can't be made here). Presets opens the browser, whose search takes
    // focus, then closes it, giving focus back.
    for (const juce::String name : { "Undo", "Redo", "A/B Compare", "A/B Compare", "Copy", "Presets", "Presets" })
    {
        CAPTURE (name);
        auto* clicked = button (name);
        CHECK (clicked->getWantsKeyboardFocus());
        CHECK_FALSE (clicked->getMouseClickGrabsKeyboardFocus());
        clicked->onClick();
    }
    CHECK (gain.hasKeyboardFocus (false));
}

TEST_CASE ("The Preset browser's list takes Tab, moves with the arrow keys, loads with Return and closes with Escape")
{
    EveryControl host;
    auto& presets = named<juce::Button> (host, "Presets");
    presets.grabKeyboardFocus();
    host.press (returnKey);
    auto& browser = named<juce::Component> (host, "Preset Browser");
    REQUIRE (browser.isVisible());
    host.press (tab); // from the search to the list
    auto& list = named<juce::ListBox> (host, "Presets List");
    REQUIRE (list.hasKeyboardFocus (true));
    host.press (down); // the first folder's name
    host.press (down); // its first Preset
    CHECK (host.press (returnKey));
    CHECK (host.processor.sessionState().loadedPresetName().isNotEmpty());
    CHECK (browser.isVisible());
    host.press (escape);
    CHECK_FALSE (browser.isVisible());
    CHECK (presets.hasKeyboardFocus (false));
}
