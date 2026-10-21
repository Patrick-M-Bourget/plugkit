// Copied from eq1's tests/plugin/ABCompareTest.cpp (eq1 at dbb1127), its generic cases against plugkit's test
// processor's Session: its parameters stand in for eq1's Bands and output section. The case on switching sides
// without a click runs eq1's audio, and stays in eq1.

#include "TestProcessor.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using plugkit::test::TestProcessor;
using plugkit::ui::CompareSide;

namespace
{

// A plugin as a host has it.
struct Host
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    plugkit::ui::Session& session = processor.sessionState();

    juce::RangedAudioParameter& parameter (const juce::String& id) { return *processor.parameterState().getParameter (id); }
    float value (const juce::String& id) { return processor.parameterState().getRawParameterValue (id)->load(); }

    // An edit in the editor: inside a gesture.
    void edit (const juce::String& id, float plain)
    {
        auto& p = parameter (id);
        p.beginChangeGesture();
        p.setValueNotifyingHost (p.convertTo0to1 (plain));
        p.endChangeGesture();
    }
};

} // namespace

TEST_CASE ("A/B Compare starts on A, and B starts as a copy of A")
{
    Host host;
    CHECK (host.session.compareSide() == CompareSide::A);
    host.edit (TestProcessor::invertId, 1.0f);
    host.edit (TestProcessor::gainId, 6.0f);

    host.session.selectCompareSide (CompareSide::B);
    CHECK (host.session.compareSide() == CompareSide::B);
    CHECK (host.value (TestProcessor::invertId) == 1.0f);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
}

TEST_CASE ("Each side keeps its own settings")
{
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    host.edit (TestProcessor::gainId, 6.0f);
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::gainId, -3.0f);
    host.edit (TestProcessor::frequencyId, 4000.0f);

    host.session.selectCompareSide (CompareSide::A);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (1000.0, 1.0e-2));

    host.session.selectCompareSide (CompareSide::B);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-3.0, 1.0e-4));
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (4000.0, 1.0e-1));
}

TEST_CASE ("Global Bypass belongs to neither side: switching sides leaves it alone")
{
    Host host;
    host.edit (TestProcessor::globalBypassId, 1.0f);
    host.session.selectCompareSide (CompareSide::B);
    CHECK (host.value (TestProcessor::globalBypassId) == 1.0f);
    host.edit (TestProcessor::globalBypassId, 0.0f);
    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.value (TestProcessor::globalBypassId) == 0.0f);
}

TEST_CASE ("Copy from A overwrites B with A, and leaves you on A")
{
    Host host;
    host.edit (TestProcessor::bypassId, 1.0f);
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::bypassId, 0.0f);
    host.edit (TestProcessor::shapeId, 5.0f);
    host.session.selectCompareSide (CompareSide::A);
    host.edit (TestProcessor::frequencyId, 120.0f);

    host.session.copyToOther();
    CHECK (host.session.compareSide() == CompareSide::A);
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (120.0, 1.0e-3));
    host.session.selectCompareSide (CompareSide::B);
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (120.0, 1.0e-3));
    CHECK (host.value (TestProcessor::shapeId) == 0.0f);
}

TEST_CASE ("Copy from B overwrites A with B, leaving you on B with the parameters unchanged")
{
    Host host;
    host.edit (TestProcessor::bypassId, 1.0f);
    host.edit (TestProcessor::frequencyId, 120.0f);
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::bypassId, 0.0f);
    host.edit (TestProcessor::shapeId, 5.0f);

    struct Changes final : juce::AudioProcessorListener
    {
        int count = 0;
        void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override { ++count; }
        void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}
    } changes;
    host.processor.addListener (&changes);
    host.session.copyToOther();
    host.processor.removeListener (&changes);
    CHECK (changes.count == 0);
    CHECK (host.session.compareSide() == CompareSide::B);
    CHECK (host.value (TestProcessor::bypassId) == 0.0f);
    CHECK (host.value (TestProcessor::shapeId) == 5.0f);

    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.value (TestProcessor::bypassId) == 0.0f);
    CHECK (host.value (TestProcessor::shapeId) == 5.0f);
}

TEST_CASE ("A session saved right after a copy holds both sides whole")
{
    Host host;
    const bool fromB = GENERATE (false, true);
    CAPTURE (fromB);
    host.edit (TestProcessor::gainId, 6.0f);
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::gainId, -3.0f);
    if (! fromB)
        host.session.selectCompareSide (CompareSide::A);
    host.session.copyToOther();
    juce::MemoryBlock saved;
    host.processor.getStateInformation (saved);

    TestProcessor restored;
    restored.setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));
    const float expected = fromB ? -3.0f : 6.0f;
    for (const auto side : { CompareSide::A, CompareSide::B })
    {
        restored.sessionState().selectCompareSide (side);
        CHECK_THAT (restored.parameterState().getRawParameterValue (TestProcessor::gainId)->load(), WithinAbs (expected, 1.0e-4));
    }
}

TEST_CASE ("Undo crosses an A/B switch: one history for both sides, a switch is one step")
{
    Host host;
    auto& history = host.processor.editHistory();
    host.edit (TestProcessor::gainId, 6.0f);
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::gainId, -3.0f);
    REQUIRE (history.undoSteps() == 3);

    history.undo(); // the edit on B
    CHECK (host.session.compareSide() == CompareSide::B);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    history.undo(); // the switch
    CHECK (host.session.compareSide() == CompareSide::A);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    history.undo(); // the edit on A
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (0.0, 1.0e-4));
    CHECK_FALSE (history.canUndo());

    history.redo();
    history.redo();
    CHECK (host.session.compareSide() == CompareSide::B);
    history.redo();
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-3.0, 1.0e-4));

    // B kept its own settings through it all.
    host.session.selectCompareSide (CompareSide::A);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
}

TEST_CASE ("Copy to the other side is one undo step, from either side")
{
    Host host;
    const bool fromB = GENERATE (false, true);
    CAPTURE (fromB);
    auto& history = host.processor.editHistory();
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::gainId, -3.0f);
    host.session.selectCompareSide (CompareSide::A);
    host.edit (TestProcessor::gainId, 6.0f);
    if (fromB)
        host.session.selectCompareSide (CompareSide::B);
    const int steps = history.undoSteps();

    host.session.copyToOther();
    CHECK (history.undoSteps() == steps + 1);
    history.undo();
    // Both sides as they were, and you on the side you copied from.
    CHECK (host.session.compareSide() == (fromB ? CompareSide::B : CompareSide::A));
    host.session.selectCompareSide (CompareSide::A);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    host.session.selectCompareSide (CompareSide::B);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-3.0, 1.0e-4));
}

TEST_CASE ("A side holds the plugin's Preset settings: one its IsPresetSetting leaves out belongs to neither side")
{
    // Built as a plugin would, with its own predicate: here Gain stays out of the sides, as Global Bypass does by
    // default.
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    auto& parameters = processor.parameterState();
    plugkit::ui::EditHistory history { processor };
    plugkit::ui::ABCompare compare { parameters, history, [] (const juce::RangedAudioParameter& p) {
                                        return p.getParameterID() != TestProcessor::gainId;
                                    } };
    const auto set = [&parameters] (const char* id, float plain) {
        auto& p = *parameters.getParameter (id);
        p.setValueNotifyingHost (p.convertTo0to1 (plain));
    };
    const auto value = [&parameters] (const char* id) { return parameters.getRawParameterValue (id)->load(); };
    set (TestProcessor::gainId, 6.0f);
    set (TestProcessor::globalBypassId, 1.0f);
    compare.select (CompareSide::B);
    set (TestProcessor::gainId, -6.0f);
    set (TestProcessor::globalBypassId, 0.0f);
    set (TestProcessor::shapeId, 3.0f);
    compare.select (CompareSide::A);
    CHECK_THAT (value (TestProcessor::gainId), WithinAbs (-6.0, 1.0e-4));
    CHECK (value (TestProcessor::shapeId) == 0.0f);
    // Global Bypass is a side's setting now.
    CHECK (value (TestProcessor::globalBypassId) == 1.0f);
}
