// Copied from eq1's tests/plugin/AccessibilityTest.cpp (eq1 at dbb1127), its one case on the kit alone, "A Staple
// Knob described by its parameter reads, and shows in its tooltip, the name and the value with its unit", against
// plugkit's test processor in place of eq1's editor; listed in eq1#231.

#include "TestProcessor.h"
#include "plugkit/ui/staple/controls/Knob.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE ("A Staple Knob described by its parameter reads, and shows in its tooltip, the name and the value with its unit")
{
    juce::ScopedJuceInitialiser_GUI juce;
    plugkit::test::TestProcessor processor;
    auto& gain = *processor.parameterState().getParameter (plugkit::test::TestProcessor::gainId);
    staple::Knob knob (staple::tokens::knob::gain);
    juce::SliderParameterAttachment attachment (gain, knob);
    knob.describe (gain);
    juce::Component window;
    window.setBounds (0, 0, 200, 200);
    window.addAndMakeVisible (knob);
    knob.setBounds (10, 10, knob.getIdealSize(), knob.getIdealSize());
    window.addToDesktop (0);
    gain.setValueNotifyingHost (gain.convertTo0to1 (3.5f));

    auto* handler = knob.getAccessibilityHandler();
    REQUIRE (handler != nullptr);
    CHECK (handler->getTitle() == "Gain");
    CHECK (handler->getValueInterface()->getCurrentValueAsString() == "+3.50 dB");
    CHECK (knob.tooltipTitle() == "Gain");
    CHECK (knob.tooltipValue() == "+3.50 dB");
}
