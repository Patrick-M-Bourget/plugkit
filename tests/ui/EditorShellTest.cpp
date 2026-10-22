// Copied from eq1's tests/plugin/EditorLayoutTest.cpp (eq1 at dbb1127), its case on the window's areas, against
// plugkit's test editor (TestProcessor.h), an EditorShell whose body stands where eq1's display and Output Meter rail
// do, and from eq1's tests/plugin/FooterTest.cpp (eq1 at dbb1127), its Cmd/Ctrl+B case, on the test processor's Global
// Bypass. The layout case at every UI Scale and size, and the undo and redo keys' cases, are new: eq1 has none. eq1's
// cases on its display, Output Meter rail and Band panel stay in eq1.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/EditorShell.h"
#include "plugkit/ui/HeaderBar.h"
#include "plugkit/ui/UiScaleMenu.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>

using Catch::Matchers::WithinAbs;
using plugkit::test::TestProcessor;

namespace
{

// The editor's areas, in the content's logical pixels (as at 100%).
struct Areas
{
    juce::Rectangle<int> header, body, footer;
};

juce::Component& titledComponent (juce::Component& editor, const juce::String& title)
{
    auto* found = harness::findChild<juce::Component> (editor, [&title] (juce::Component& c) { return c.getTitle() == title; });
    REQUIRE (found != nullptr);
    return *found;
}

Areas areasOf (juce::AudioProcessorEditor& editor)
{
    auto* header = harness::findChild<plugkit::ui::HeaderBar> (editor);
    REQUIRE (header != nullptr);
    auto& content = *header->getParentComponent();
    const auto in = [&content] (juce::Component& c) { return content.getLocalArea (c.getParentComponent(), c.getBounds()); };
    return { in (*header), in (titledComponent (editor, "Body")), in (titledComponent (editor, "Footer")) };
}

const auto command = juce::ModifierKeys::commandModifier;

} // namespace

TEST_CASE ("The window is a 52 px header, the plugin's body and a 44 px footer, and only the body resizes")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    REQUIRE (editor->getWidth() == 1200);
    REQUIRE (editor->getHeight() == 760);

    // 14 px padding and 12 px gaps.
    auto areas = areasOf (*editor);
    CHECK (areas.header == juce::Rectangle<int> (14, 14, 1172, 52));
    CHECK (areas.body == juce::Rectangle<int> (14, 78, 1172, 612));
    CHECK (areas.footer == juce::Rectangle<int> (14, 702, 1172, 44));

    // Larger: the header and footer keep their height.
    editor->setSize (1500, 900);
    areas = areasOf (*editor);
    CHECK (areas.header == juce::Rectangle<int> (14, 14, 1472, 52));
    CHECK (areas.body == juce::Rectangle<int> (14, 78, 1472, 752));
    CHECK (areas.footer == juce::Rectangle<int> (14, 842, 1472, 44));
}

TEST_CASE ("At every UI Scale, and at the window's minimum, default and maximum sizes, the header, body and footer fill the window without overlapping")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto* menu = harness::findChild<plugkit::ui::UiScaleMenu> (*editor);
    REQUIRE (menu != nullptr);

    const int percent = GENERATE (75, 100, 125, 150, 200);
    const auto size = GENERATE (TestProcessor::window.minimum, TestProcessor::window.defaultSize, TestProcessor::window.maximum);
    CAPTURE (percent, size.x, size.y);
    menu->pick (percent);
    const auto scaled = [percent] (int logical) { return juce::roundToInt (logical * percent / 100.0); };
    editor->setSize (scaled (size.x), scaled (size.y));

    const auto areas = areasOf (*editor);
    CHECK (areas.header == juce::Rectangle<int> (14, 14, size.x - 28, 52));
    CHECK (areas.footer == juce::Rectangle<int> (14, size.y - 58, size.x - 28, 44));
    CHECK (areas.body == juce::Rectangle<int> (14, 78, size.x - 28, size.y - 148));
    CHECK_FALSE (areas.header.intersects (areas.body));
    CHECK_FALSE (areas.body.intersects (areas.footer));
}

TEST_CASE ("Cmd-Z undoes, and Shift-Cmd-Z and Cmd-Y redo, from a control or with nothing focused")
{
    harness::OpenEditor<TestProcessor> host;
    auto& history = host.processor.editHistory();
    auto& gain = host.parameter (TestProcessor::gainId);
    plugkit::ui::setTogether ({ { &gain, gain.convertTo0to1 (6.0f) } });
    REQUIRE (history.canUndo());

    const bool focused = GENERATE (true, false);
    CAPTURE (focused);
    if (focused)
        host.showing<juce::Slider> ("Frequency").grabKeyboardFocus();
    else
        juce::Component::unfocusAllComponents();

    CHECK (host.press (juce::KeyPress ('z', command, 0)));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (0.0, 1.0e-4));
    CHECK (host.press (juce::KeyPress ('z', command | juce::ModifierKeys::shiftModifier, 0)));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    CHECK (host.press (juce::KeyPress ('z', command, 0)));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (0.0, 1.0e-4));
    CHECK (host.press (juce::KeyPress ('y', command, 0)));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    // And the header's Undo follows at once.
    CHECK (host.showing<juce::Button> ("Undo").isEnabled());
    CHECK_FALSE (host.showing<juce::Button> ("Redo").isEnabled());
}

TEST_CASE ("Cmd/Ctrl+B toggles Global Bypass, each press one undo step, leaving the other settings alone")
{
    harness::OpenEditor<TestProcessor> host;
    host.set (TestProcessor::bypassId, 1.0f);
    host.showing<juce::Slider> ("Gain").grabKeyboardFocus();
    auto& history = host.processor.editHistory();
    const int steps = history.undoSteps();

    CHECK (host.press (juce::KeyPress ('b', command, 0)));
    CHECK (host.value (TestProcessor::globalBypassId) == 1.0f);
    CHECK (history.undoSteps() == steps + 1);
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);

    CHECK (host.press (juce::KeyPress ('b', command, 0)));
    CHECK (host.value (TestProcessor::globalBypassId) == 0.0f);
    CHECK (history.undoSteps() == steps + 2);
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);
}
