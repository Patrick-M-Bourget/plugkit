// Copied from comp1's tests/plugin/StateTest.cpp (comp1 at fd63c2c), against plugkit's test processor, which
// restores through a Session: its Gain stands in for comp1's Output Gain, and its Ratio is comp1's, so the cases pin
// the restore rule Session shares with comp1. Saving and restoring go through the editor harness's helpers rather
// than comp1's tests/plugin/Host.h. The cases on a Preset file's and a B side's infinity are added.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <limits>

using plugkit::test::TestProcessor;
using plugkit::ui::CompareSide;

namespace
{

juce::RangedAudioParameter& parameter (juce::AudioProcessor& processor, const juce::String& id)
{
    for (auto* p : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && ranged->getParameterID() == id)
            return *ranged;
    FAIL ("No parameter " << id);
    throw std::logic_error ("unreachable");
}

float value (juce::AudioProcessor& processor, const juce::String& id)
{
    auto& p = parameter (processor, id);
    return p.convertFrom0to1 (p.getValue());
}

void set (juce::AudioProcessor& processor, const juce::String& id, float plain)
{
    auto& p = parameter (processor, id);
    p.setValueNotifyingHost (p.convertTo0to1 (plain));
}

constexpr float infinity = std::numeric_limits<float>::infinity();

} // namespace

TEST_CASE ("Save, load and save again gives the same bytes")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    set (saved, TestProcessor::gainId, -7.3f);
    const auto first = harness::savedBlock (saved);

    TestProcessor restored;
    harness::restoreState (restored, first);
    CHECK (juce::exactlyEqual (value (restored, TestProcessor::gainId), value (saved, TestProcessor::gainId)));
    CHECK (harness::savedBlock (restored) == first);
}

TEST_CASE ("A session from a newer version keeps what this version doesn't know, and saves it again")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor source;
    set (source, TestProcessor::gainId, 15.0f);
    auto xml = harness::savedXml (source);
    REQUIRE (xml != nullptr);
    xml->setAttribute ("version", TestProcessor::stateVersion + 1);
    auto* unknown = xml->createNewChildElement ("PARAM");
    unknown->setAttribute ("id", "from_the_future");
    unknown->setAttribute ("value", 7.0);
    xml->createNewChildElement ("SomethingNew")->setAttribute ("x", 1);

    TestProcessor processor;
    harness::restoreState (processor, *xml);
    CHECK (value (processor, TestProcessor::gainId) == 15.0f);
    // Saved again, it carries this version's number, and keeps what this version doesn't know.
    const auto resaved = harness::savedXml (processor);
    CHECK (resaved->getIntAttribute ("version", -1) == TestProcessor::stateVersion);
    CHECK (resaved->getChildByName ("SomethingNew") != nullptr);
    CHECK (resaved->getChildByAttribute ("id", "from_the_future") != nullptr);
    // Once: the A/B Compare child is restored, not kept as something unknown.
    int compares = 0;
    for (auto* child : resaved->getChildWithTagNameIterator ("ABCompare"))
        compares += child != nullptr ? 1 : 0;
    CHECK (compares == 1);
}

TEST_CASE ("A saved value outside its range, or not a number, restores clamped, or as the default")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor source;
    auto xml = harness::savedXml (source);
    REQUIRE (xml != nullptr);
    auto* gain = xml->getChildByAttribute ("id", TestProcessor::gainId);
    REQUIRE (gain != nullptr);

    TestProcessor processor;
    gain->setAttribute ("value", 1000.0);
    harness::restoreState (processor, *xml);
    CHECK (value (processor, TestProcessor::gainId) == 30.0f);
    gain->setAttribute ("value", "nan");
    harness::restoreState (processor, *xml);
    CHECK (value (processor, TestProcessor::gainId) == 0.0f);
    gain->setAttribute ("value", -std::numeric_limits<double>::infinity());
    harness::restoreState (processor, *xml);
    CHECK (value (processor, TestProcessor::gainId) == 0.0f);
}

TEST_CASE ("Every setting survives save and load, Ratio infinity:1 included")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    const auto ratio = GENERATE (2.5f, infinity);
    CAPTURE (ratio);
    set (saved, TestProcessor::frequencyId, 440.0f);
    set (saved, TestProcessor::gainId, -3.25f);
    set (saved, TestProcessor::shapeId, 6.0f);
    set (saved, TestProcessor::ratioId, ratio);
    const auto first = harness::savedBlock (saved);

    TestProcessor restored;
    harness::restoreState (restored, first);
    for (const auto* id : { TestProcessor::frequencyId, TestProcessor::gainId, TestProcessor::shapeId, TestProcessor::ratioId })
    {
        CAPTURE (id);
        CHECK (juce::exactlyEqual (value (restored, id), value (saved, id)));
    }
    CHECK (juce::exactlyEqual (value (restored, TestProcessor::ratioId), ratio));
    CHECK (harness::savedBlock (restored) == first);
}

TEST_CASE ("A session missing a parameter restores it to its default")
{
    // Session adds it at its default itself (sanitiseSavedSettings), as comp1's restore does. JUCE 9.0.3's replaceState
    // gives it its default too, so eq1's restore does the same, but the rule doesn't rest on that.
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    set (processor, TestProcessor::gainId, 12.0f);
    set (processor, TestProcessor::ratioId, infinity);
    juce::XmlElement empty (TestProcessor::stateType);
    empty.setAttribute ("version", TestProcessor::stateVersion);
    harness::restoreState (processor, empty);
    CHECK (value (processor, TestProcessor::gainId) == 0.0f);
    CHECK (value (processor, TestProcessor::ratioId) == 4.0f);
}

TEST_CASE ("Anything that isn't the plugin's state changes nothing")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    set (processor, TestProcessor::gainId, 18.0f);
    processor.setZoom (5);
    juce::XmlElement other ("eq1");
    other.setAttribute ("version", 1);
    other.setAttribute ("zoom", 2);
    harness::restoreState (processor, other);
    const char garbage[] = "not a state";
    processor.setStateInformation (garbage, static_cast<int> (sizeof (garbage)));
    CHECK (value (processor, TestProcessor::gainId) == 18.0f);
    CHECK (processor.zoom() == 5);
}

TEST_CASE ("A Preset file holding Ratio infinity:1 loads at infinity:1")
{
    juce::ScopedJuceInitialiser_GUI juce;
    juce::TemporaryFile folder;
    REQUIRE (folder.getFile().createDirectory());
    TestProcessor maker ({}, folder.getFile());
    set (maker, TestProcessor::ratioId, infinity);
    const auto& library = maker.sessionState().presetLibrary();
    const auto file = library.save ("Limiter", maker.sessionState().presetState());
    REQUIRE (file.has_value());
    REQUIRE (file->loadFileAsString().contains ("inf"));

    TestProcessor processor;
    REQUIRE (processor.sessionState().loadPreset (library.read (*file), "Limiter"));
    CHECK (juce::exactlyEqual (value (processor, TestProcessor::ratioId), infinity));
    CHECK_FALSE (processor.sessionState().isLoadedPresetModified());
    folder.getFile().deleteRecursively();
}

TEST_CASE ("A B side holding Ratio infinity:1, saved with the session, switches to infinity:1")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    saved.sessionState().selectCompareSide (CompareSide::B);
    set (saved, TestProcessor::ratioId, infinity);
    saved.sessionState().selectCompareSide (CompareSide::A);
    REQUIRE (value (saved, TestProcessor::ratioId) == 4.0f);

    TestProcessor restored;
    harness::restoreState (restored, harness::savedBlock (saved));
    restored.sessionState().selectCompareSide (CompareSide::B);
    CHECK (juce::exactlyEqual (value (restored, TestProcessor::ratioId), infinity));
}
