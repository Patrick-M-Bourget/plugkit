// Copied from eq1's tests/plugin/EditorTickTest.cpp (eq1 at dbb1127), without its case on eq1's Analyzer, "The
// Analyzer keeps the tick at 60 Hz while it shows, and lets it idle once off", against plugkit's test editor, which has
// no Analyzer to turn off; listed in eq1#231.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/EditorTick.h"

#include <catch2/catch_test_macros.hpp>

#include <thread>

using Host = harness::OpenEditor<plugkit::test::TestProcessor>;
using plugkit::test::TestProcessor;

TEST_CASE ("An idle editor ticks at 10 Hz, and a parameter change wakes the tick to 60 Hz at once")
{
    Host host;
    auto& tick = host.editorTick();
    // Nothing animating: idle, once what the editor opened with has settled.
    REQUIRE (host.settleUntil ([&] { return tick.rateHz() == plugkit::ui::EditorTick::idleHz; }));
    const int ticked = tick.count();
    host.settle (250);
    CHECK (tick.count() > ticked);
    CHECK (tick.rateHz() == plugkit::ui::EditorTick::idleHz);

    // A parameter change, as an edit or Host Automation makes one.
    host.set (TestProcessor::gainId, 15.0f);
    CHECK (tick.rateHz() == plugkit::ui::EditorTick::activeHz);
    // Nothing animating or metering, the tick settles back to idle.
    CHECK (host.settleUntil ([&] { return tick.rateHz() == plugkit::ui::EditorTick::idleHz; }));
}

TEST_CASE ("Host Automation from another thread wakes the tick on its next callback, with no message posted")
{
    Host host;
    auto& tick = host.editorTick();
    REQUIRE (host.settleUntil ([&] { return tick.rateHz() == plugkit::ui::EditorTick::idleHz; }));

    // A Client that notes the change on the tick that first shows it, and the rate of the tick after.
    struct Probe final : juce::Component, plugkit::ui::EditorTick::Client
    {
        explicit Probe (Host& h) : Client (static_cast<juce::Component&> (*this)), host (h) {}
        void tick (double) override
        {
            if (saw && rateAfter == 0)
                rateAfter = host.editorTick().rateHz();
            saw = saw || host.value (TestProcessor::gainId) == 15.0f;
        }
        Host& host;
        bool saw = false;
        int rateAfter = 0;
    } probe { host };
    host.editor->addChildComponent (probe);

    // The harness runs timers but never dispatches posted messages, so only what the tick's timer
    // reads can wake it: a change that posts a message instead is never seen to wake it.
    std::thread automation ([&] { host.set (TestProcessor::gainId, 15.0f); });
    automation.join();
    REQUIRE (host.settleUntil ([&] { return probe.rateAfter != 0; }));
    CHECK (probe.saw);
    CHECK (probe.rateAfter == plugkit::ui::EditorTick::activeHz);
    host.editor->removeChildComponent (&probe);
}
