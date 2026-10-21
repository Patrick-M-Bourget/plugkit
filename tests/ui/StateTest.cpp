// Copied from eq1's tests/plugin/StateTest.cpp (eq1 at dbb1127), its generic cases against plugkit's test processor,
// which saves and restores through a Session: its zoom stands in for eq1's Display Range and Output Meter as the
// plugin's own view, its versions 0 and 1 for eq1's fixtures, and the check of every parameter for eq1's of the sound
// (the test processor makes none). The cases on what Session's restore rule changes from eq1's say so. The Analyzer,
// Display Range and Output Meter cases, and the version 2 fixture's, stay in eq1.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>

using Catch::Matchers::WithinAbs;
using plugkit::test::TestProcessor;
using plugkit::ui::CompareSide;

namespace
{

constexpr double inf = std::numeric_limits<double>::infinity();

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

// A state as an older version saved it: version 0 had none, and neither had A/B Compare. Gain -4.5 dB, Frequency
// 250 Hz, Shape High Cut, Placement Mid, Ratio 8:1 and Global Bypass on, the others at their defaults; zoom 3.
std::unique_ptr<juce::XmlElement> olderState (int version)
{
    auto xml = juce::parseXML (R"(<plugkit_test zoom="3">
        <PARAM id="frequency" value="250.0"/><PARAM id="gain" value="-4.5"/><PARAM id="invert" value="0.0"/>
        <PARAM id="bypass" value="0.0"/><PARAM id="shape" value="4.0"/><PARAM id="placement" value="3.0"/>
        <PARAM id="global_bypass" value="1.0"/><PARAM id="ratio" value="8.0"/></plugkit_test>)");
    REQUIRE (xml != nullptr);
    if (version > 0)
        xml->setAttribute ("version", version);
    return xml;
}

// Every host parameter's normalised value.
std::vector<float> valuesOf (juce::AudioProcessor& processor)
{
    std::vector<float> values;
    for (auto* p : processor.getParameters())
        values.push_back (p->getValue());
    return values;
}

} // namespace

TEST_CASE ("Saved state carries the state version")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    const auto xml = harness::savedXml (processor);
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version", -1) == TestProcessor::stateVersion);
    CHECK (TestProcessor::stateVersion == 2);
}

TEST_CASE ("Save and reload restore every host parameter exactly")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    const std::pair<const char*, float> values[] = { { TestProcessor::frequencyId, 250.0f }, { TestProcessor::gainId, -4.5f },
                                                     { TestProcessor::invertId, 1.0f },      { TestProcessor::bypassId, 1.0f },
                                                     { TestProcessor::shapeId, 4.0f },       { TestProcessor::placementId, 3.0f },
                                                     { TestProcessor::globalBypassId, 1.0f }, { TestProcessor::ratioId, 2.5f } };
    for (const auto& [id, v] : values)
        set (saved, id, v);
    const auto xml = harness::savedXml (saved);
    REQUIRE (xml != nullptr);

    TestProcessor restored;
    harness::restoreState (restored, *xml);
    CHECK (valuesOf (restored) == valuesOf (saved));
}

TEST_CASE ("A/B Compare survives save and reload: the side you're on and both sides' settings")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    set (saved, TestProcessor::frequencyId, 250.0f);
    set (saved, TestProcessor::gainId, -4.5f);
    saved.sessionState().selectCompareSide (CompareSide::B);
    set (saved, TestProcessor::gainId, 9.0f);
    set (saved, TestProcessor::shapeId, 2.0f);
    const auto xml = harness::savedXml (saved);
    REQUIRE (xml != nullptr);

    TestProcessor restored;
    harness::restoreState (restored, *xml);
    CHECK (restored.sessionState().compareSide() == CompareSide::B);
    CHECK (valuesOf (restored) == valuesOf (saved));

    saved.sessionState().selectCompareSide (CompareSide::A);
    restored.sessionState().selectCompareSide (CompareSide::A);
    CHECK_THAT (value (restored, TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (value (restored, TestProcessor::shapeId) == 0.0f);
    CHECK (valuesOf (restored) == valuesOf (saved));
}

TEST_CASE ("A version 1 session loads on side A, with B a copy of it and no Loaded Preset on either")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor maker;
    set (maker, TestProcessor::shapeId, 1.0f);
    TestProcessor processor;
    processor.sessionState().loadPreset (maker.sessionState().presetState(), "Before");
    processor.sessionState().selectCompareSide (CompareSide::B);
    harness::restoreState (processor, *olderState (1));
    CHECK (processor.sessionState().compareSide() == CompareSide::A);
    CHECK_THAT (value (processor, TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (processor.sessionState().loadedPresetName().isEmpty());

    processor.sessionState().selectCompareSide (CompareSide::B);
    CHECK_THAT (value (processor, TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (value (processor, TestProcessor::shapeId) == 4.0f);
    CHECK (processor.sessionState().loadedPresetName().isEmpty());
}

TEST_CASE ("A session saved before the state had a version (version 0) loads every setting")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    harness::restoreState (processor, *olderState (0));

    CHECK_THAT (value (processor, TestProcessor::frequencyId), WithinAbs (250.0, 1.0e-3));
    CHECK_THAT (value (processor, TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (value (processor, TestProcessor::shapeId) == 4.0f);
    CHECK (value (processor, TestProcessor::placementId) == 3.0f);
    CHECK (value (processor, TestProcessor::globalBypassId) == 1.0f);
    CHECK_THAT (value (processor, TestProcessor::ratioId), WithinAbs (8.0, 1.0e-4));
    CHECK (processor.zoom() == 3);

    // Saved again, it is the current version.
    CHECK (harness::savedXml (processor)->getIntAttribute ("version", -1) == TestProcessor::stateVersion);
}

TEST_CASE ("A session from a newer version loads the settings this version knows")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto xml = olderState (0);
    xml->setAttribute ("version", TestProcessor::stateVersion + 1);
    auto* unknown = xml->createNewChildElement ("PARAM");
    unknown->setAttribute ("id", "gain_from_the_future");
    unknown->setAttribute ("value", 7.0);
    xml->createNewChildElement ("SomethingNew")->setAttribute ("x", 1);

    TestProcessor processor;
    processor.sessionState().selectCompareSide (CompareSide::B);
    harness::restoreState (processor, *xml);
    CHECK_THAT (value (processor, TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (processor.zoom() == 3);
    // It saves no A/B Compare this version knows: its settings are side A's.
    CHECK (processor.sessionState().compareSide() == CompareSide::A);
}

TEST_CASE ("A session saved in the middle of an A/B switch or a Preset load holds both sides whole")
{
    // A host saving from its own thread can save while the editor is switching sides. A listener on
    // the parameters saves at the first change the switch makes, as such a host might.
    struct SaveDuringChange final : juce::AudioProcessorListener
    {
        juce::AudioProcessor& processor;
        std::unique_ptr<juce::XmlElement> saved;
        explicit SaveDuringChange (juce::AudioProcessor& p) : processor (p) { processor.addListener (this); }
        ~SaveDuringChange() override { processor.removeListener (this); }
        void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override
        {
            if (saved == nullptr)
                saved = harness::savedXml (processor);
        }
        void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}
    };

    juce::ScopedJuceInitialiser_GUI juce;
    const auto change = GENERATE (Catch::Generators::as<std::string> {}, "switch", "load");
    CAPTURE (change);
    const bool loadOnB = change == "load";
    TestProcessor maker;
    set (maker, TestProcessor::gainId, 6.0f);
    set (maker, TestProcessor::frequencyId, 2000.0f);
    const auto preset = maker.sessionState().presetState();
    TestProcessor processor;
    set (processor, TestProcessor::gainId, 6.0f);
    set (processor, TestProcessor::frequencyId, 2000.0f);
    processor.sessionState().selectCompareSide (CompareSide::B);
    set (processor, TestProcessor::gainId, -3.0f);
    set (processor, TestProcessor::frequencyId, 500.0f);
    std::unique_ptr<juce::XmlElement> xml;
    {
        SaveDuringChange host (processor);
        if (loadOnB)
            processor.sessionState().loadPreset (preset, "Preset");
        else
            processor.sessionState().selectCompareSide (CompareSide::A);
        xml = std::move (host.saved);
    }
    REQUIRE (xml != nullptr);

    // Reloaded, the side being switched to is whole on the parameters, and the other side whole too.
    TestProcessor restored;
    harness::restoreState (restored, *xml);
    // A's settings are on the side the change ends on, and on the other side too after a load.
    const auto ending = loadOnB ? CompareSide::B : CompareSide::A;
    auto& session = restored.sessionState();
    CHECK (session.compareSide() == ending);
    CHECK_THAT (value (restored, TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    CHECK_THAT (value (restored, TestProcessor::frequencyId), WithinAbs (2000.0, 1.0e-2));
    if (loadOnB)
    {
        CHECK (session.loadedPresetName() == "Preset");
        CHECK_FALSE (session.isLoadedPresetModified());
    }
    session.selectCompareSide (ending == CompareSide::B ? CompareSide::A : CompareSide::B);
    CHECK_THAT (value (restored, TestProcessor::gainId), WithinAbs (change == "switch" ? -3.0 : 6.0, 1.0e-4));
    CHECK_THAT (value (restored, TestProcessor::frequencyId), WithinAbs (change == "switch" ? 500.0 : 2000.0, 1.0e-2));
}

TEST_CASE ("Save and reload keep each side's Loaded Preset and Modified state, even after its file is deleted")
{
    juce::ScopedJuceInitialiser_GUI juce;
    juce::TemporaryFile folder;
    REQUIRE (folder.getFile().createDirectory());
    TestProcessor saved ({}, folder.getFile());
    const auto& library = saved.sessionState().presetLibrary();
    const auto savePreset = [&] (const juce::String& name, const std::initializer_list<std::pair<const char*, float>> values) {
        TestProcessor maker;
        for (const auto& [id, v] : values)
            set (maker, id, v);
        const auto file = library.save (name, maker.sessionState().presetState());
        REQUIRE (file.has_value());
        return *file;
    };
    const auto vocal = savePreset ("Vocal", { { TestProcessor::frequencyId, 3456.7f }, { TestProcessor::placementId, 2.0f } });
    const auto kick = savePreset ("Kick", { { TestProcessor::gainId, 4.0f }, { TestProcessor::shapeId, 1.0f } });

    auto& session = saved.sessionState();
    session.loadPreset (library.read (vocal), "Vocal");
    set (saved, TestProcessor::gainId, 3.0f);
    session.selectCompareSide (CompareSide::B);
    session.loadPreset (library.read (kick), "Kick");
    const auto xml = harness::savedXml (saved);
    REQUIRE (xml != nullptr);
    folder.getFile().deleteRecursively();

    TestProcessor restored;
    harness::restoreState (restored, *xml);
    auto& restoredSession = restored.sessionState();
    CHECK (restoredSession.loadedPresetName() == "Kick");
    CHECK_FALSE (restoredSession.isLoadedPresetModified());
    restoredSession.selectCompareSide (CompareSide::A);
    CHECK (restoredSession.loadedPresetName() == "Vocal");
    CHECK (restoredSession.isLoadedPresetModified());
    set (restored, TestProcessor::gainId, 0.0f);
    CHECK_FALSE (restoredSession.isLoadedPresetModified());
}

TEST_CASE ("The plugin's own view setting is saved with the session, isn't undoable and doesn't make a side Modified")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    auto& session = saved.sessionState();
    // With a Loaded Preset, a change to any sound setting would make the side Modified.
    REQUIRE (session.loadPreset (session.presetState(), "Current"));
    REQUIRE_FALSE (session.isLoadedPresetModified());
    const int steps = saved.editHistory().undoSteps(); // loading the Preset is one
    saved.setZoom (4);
    CHECK_FALSE (session.isLoadedPresetModified());
    CHECK (saved.editHistory().undoSteps() == steps);
    const auto xml = harness::savedXml (saved);
    REQUIRE (xml != nullptr);
    CHECK (xml->getIntAttribute ("version", -1) == TestProcessor::stateVersion);

    TestProcessor restored;
    harness::restoreState (restored, *xml);
    CHECK (restored.zoom() == 4);
    // Its reader took it off the state: nothing of it reached the parameters' state, to be saved twice.
    CHECK_FALSE (restored.parameterState().copyState().hasProperty ("zoom"));
}

TEST_CASE ("The saved names a test checks include nested properties and host parameter ids")
{
    juce::ValueTree state ("plugkit_test");
    state.setProperty ("displayRangeDb", 12, nullptr);
    juce::ValueTree side ("A");
    juce::ValueTree parameter ("PARAM");
    parameter.setProperty ("id", "band3_solo_slot", nullptr);
    side.appendChild (parameter, nullptr);
    juce::ValueTree held ("Held");
    held.setProperty ("auditionSlot", 3, nullptr);
    side.appendChild (held, nullptr);
    state.appendChild (side, nullptr);

    const auto names = harness::savedNames (state);
    CHECK (names.contains ("displayRangeDb"));
    CHECK (names.contains ("band3_solo_slot"));
    CHECK (names.contains ("auditionSlot"));
    CHECK (names.contains ("Held"));
}

namespace
{

// Sets a saved setting's value as written in the file, which may be anything.
void setSaved (juce::XmlElement& state, const juce::String& id, const juce::String& text)
{
    for (auto* child : state.getChildWithTagNameIterator ("PARAM"))
        if (child->getStringAttribute ("id") == id)
        {
            child->setAttribute ("value", text);
            return;
        }
    FAIL ("No saved setting " << id);
}

} // namespace

TEST_CASE ("A state with nan, inf and out-of-range values restores to in-range parameters: defaults where a value isn't a number or is an infinity its range doesn't end at, the range's end where it is outside")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor maker;
    set (maker, TestProcessor::invertId, 1.0f);
    const auto xml = harness::savedXml (maker);
    REQUIRE (xml != nullptr);
    setSaved (*xml, TestProcessor::frequencyId, "nan");
    setSaved (*xml, TestProcessor::gainId, "inf");
    setSaved (*xml, TestProcessor::ratioId, "-inf");
    setSaved (*xml, TestProcessor::shapeId, "42");
    setSaved (*xml, TestProcessor::placementId, "-7");
    setSaved (*xml, TestProcessor::bypassId, "not a number");

    TestProcessor restored;
    harness::restoreState (restored, *xml);
    REQUIRE (harness::firstParameterOutOfRange (restored).isEmpty());
    // Not a number, or an infinity the range doesn't end at: the default.
    CHECK_THAT (value (restored, TestProcessor::frequencyId), WithinAbs (1000.0, 1.0e-3));
    CHECK (value (restored, TestProcessor::gainId) == 0.0f);
    CHECK_THAT (value (restored, TestProcessor::ratioId), WithinAbs (4.0, 1.0e-4));
    // Finite and outside the range: the range's end.
    CHECK (juce::exactlyEqual (value (restored, TestProcessor::shapeId), static_cast<float> (TestProcessor::shapeNames().size() - 1)));
    CHECK (value (restored, TestProcessor::placementId) == 0.0f);
    // Text that isn't a number reads as 0.
    CHECK (value (restored, TestProcessor::bypassId) == 0.0f);
    // The rest restored as saved.
    CHECK (value (restored, TestProcessor::invertId) == 1.0f);
}

TEST_CASE ("A restored number is itself within its range, the nearer end outside it, an infinity its range ends at, and the default otherwise")
{
    using plugkit::ui::restoredNumber;
    CHECK (restoredNumber (juce::var (2.5), 1.0, 0.0, 6.0) == 2.5);
    CHECK (restoredNumber (juce::var (0.0), 1.0, 0.0, 6.0) == 0.0);
    CHECK (restoredNumber (juce::var (6.0), 1.0, 0.0, 6.0) == 6.0);
    CHECK (restoredNumber (juce::var (99.0), 1.0, 0.0, 6.0) == 6.0);
    CHECK (restoredNumber (juce::var (-7.0), 1.0, 0.0, 6.0) == 0.0);
    CHECK (restoredNumber (juce::var ("1e39"), 1.0, 0.0, 6.0) == 6.0);
    CHECK (restoredNumber (juce::var (std::numeric_limits<double>::quiet_NaN()), 1.0, 0.0, 6.0) == 1.0);
    CHECK (restoredNumber (juce::var ("nan"), 1.0, 0.0, 6.0) == 1.0);
    CHECK (restoredNumber (juce::var (inf), 1.0, 0.0, 6.0) == 1.0);
    CHECK (restoredNumber (juce::var (-inf), 1.0, 0.0, 6.0) == 1.0);
    CHECK (restoredNumber (juce::var ("inf"), 1.0, 0.0, 6.0) == 1.0);
    CHECK (restoredNumber (juce::var(), 1.0, 0.0, 6.0) == 0.0); // no value reads as 0, which is in range
    // Session's rule, not eq1's: an infinity the range ends at is itself, and only that one.
    CHECK (juce::exactlyEqual (restoredNumber (juce::var (inf), 4.0, 1.0, inf), inf));
    CHECK (juce::exactlyEqual (restoredNumber (juce::var ("inf"), 4.0, 1.0, inf), inf));
    CHECK (restoredNumber (juce::var (-inf), 4.0, 1.0, inf) == 4.0);
    CHECK (juce::exactlyEqual (restoredNumber (juce::var (-inf), 0.0, -inf, 0.0), -inf));
    CHECK (juce::exactlyEqual (restoredNumber (juce::var (1.0e39), 4.0, 1.0, inf), 1.0e39));
}

TEST_CASE ("A state whose window size isn't a finite number restores the default size; a finite one outside its limits, the nearer limit")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor maker;
    maker.sessionState().setEditorSize ({ 700, 500 });
    const auto xml = harness::savedXml (maker);
    REQUIRE (xml != nullptr);
    struct Junk
    {
        const char* text;
        juce::Point<int> editorSize;
    };
    const auto& window = TestProcessor::window;
    // Not finite: the default. Finite but outside the limits: the nearer one. Text that isn't a number reads as 0.
    const auto junk = GENERATE_COPY (Junk { "nan", window.defaultSize },
                                     Junk { "inf", window.defaultSize },
                                     Junk { "-inf", window.defaultSize },
                                     Junk { "1e30", window.maximum },
                                     Junk { "-5", window.minimum },
                                     Junk { "text", window.minimum });
    CAPTURE (junk.text);
    xml->setAttribute ("editorWidth", junk.text);
    xml->setAttribute ("editorHeight", junk.text);

    TestProcessor restored;
    restored.sessionState().setEditorSize ({ 1000, 700 });
    harness::restoreState (restored, *xml);
    CHECK (restored.sessionState().editorSize() == junk.editorSize);
}
