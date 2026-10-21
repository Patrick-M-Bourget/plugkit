// Copied from eq1's tests/plugin/PresetTest.cpp (eq1 at dbb1127), its cases against plugkit's test processor's
// Session: its parameters stand in for eq1's Bands and output section, its zoom for eq1's Analyzer and Display Range
// as the plugin's own view, and a version 1 Preset written here for eq1's state-v1.xml fixture. The Preset file with
// nan, inf and out-of-range values loads under Session's restore rule, Ratio's inf kept.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

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
    plugkit::ui::EditHistory& history = processor.editHistory();

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

// A Preset made in another plugin instance.
juce::ValueTree presetWith (const std::initializer_list<std::pair<const char*, float>> values)
{
    Host maker;
    for (const auto& [id, v] : values)
        maker.edit (id, v);
    return maker.session.presetState();
}

} // namespace

TEST_CASE ("Loading a Preset sets every sound setting, those it doesn't change back to their defaults")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::gainId, -7.0f }, { TestProcessor::placementId, 1.0f } });
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    host.edit (TestProcessor::frequencyId, 5000.0f);

    host.session.loadPreset (preset, "Preset");
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-7.0, 1.0e-4));
    CHECK (host.value (TestProcessor::placementId) == 1.0f);
    CHECK (host.value (TestProcessor::invertId) == 0.0f);
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (1000.0, 1.0e-2));
}

TEST_CASE ("Loading a Preset is one undo step")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::gainId, -7.0f } });
    Host host;
    host.edit (TestProcessor::frequencyId, 5000.0f);
    const int steps = host.history.undoSteps();

    host.session.loadPreset (preset, "Preset");
    CHECK (host.history.undoSteps() == steps + 1);
    host.history.undo();
    CHECK (host.value (TestProcessor::bypassId) == 0.0f);
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (5000.0, 1.0e-1));
}

TEST_CASE ("Loading a Preset doesn't change Global Bypass or the plugin's own view")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::globalBypassId, 0.0f } });
    Host host;
    host.edit (TestProcessor::globalBypassId, 1.0f);
    host.processor.setZoom (3);
    host.session.setEditorSize ({ 700, 500 });

    host.session.loadPreset (preset, "Preset");
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);
    CHECK (host.value (TestProcessor::globalBypassId) == 1.0f);
    CHECK (host.processor.zoom() == 3);
    CHECK (host.session.editorSize() == juce::Point<int> { 700, 500 });
}

TEST_CASE ("Loading a Preset on B leaves A unchanged")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::gainId, -7.0f } });
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    host.session.selectCompareSide (CompareSide::B);

    host.session.loadPreset (preset, "Preset");
    CHECK (host.session.compareSide() == CompareSide::B);
    CHECK (host.value (TestProcessor::invertId) == 0.0f);
    CHECK (host.value (TestProcessor::bypassId) == 1.0f);

    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.value (TestProcessor::invertId) == 1.0f);
    CHECK (host.value (TestProcessor::bypassId) == 0.0f);
}

TEST_CASE ("A Preset holds the sound settings in the versioned state format, and nothing else")
{
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    host.edit (TestProcessor::globalBypassId, 1.0f);
    host.processor.setZoom (3);
    host.session.pickUiScale (150);
    const auto preset = host.session.presetState();
    CHECK (preset.hasType (TestProcessor::stateType));
    CHECK (static_cast<int> (preset.getProperty ("version")) == TestProcessor::stateVersion);
    CHECK (preset.getChildWithProperty ("id", TestProcessor::invertId).isValid());
    CHECK (preset.getChildWithProperty ("id", TestProcessor::ratioId).isValid());
    CHECK_FALSE (preset.getChildWithProperty ("id", TestProcessor::globalBypassId).isValid());
    for (const auto& child : preset)
        CHECK (child.hasType ("PARAM"));
    CHECK (preset.getNumProperties() == 1);
}

TEST_CASE ("A Preset in an older version of the format loads")
{
    // A version 1 session's settings, as a version 1 Preset would hold them: before A/B Compare.
    const auto preset = juce::parseXML (R"(<plugkit_test version="1"><PARAM id="gain" value="-4.5"/><PARAM id="shape" value="4.0"/></plugkit_test>)");
    REQUIRE (preset != nullptr);
    Host host;
    host.processor.setZoom (3);
    REQUIRE (host.session.loadPreset (juce::ValueTree::fromXml (*preset), "State v1"));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-4.5, 1.0e-4));
    CHECK (host.value (TestProcessor::shapeId) == 4.0f);
    // The view is not a Preset's, and its migration's A/B Compare child is no setting.
    CHECK (host.processor.zoom() == 3);
    CHECK (host.session.compareSide() == CompareSide::A);
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("Something that isn't a Preset changes nothing")
{
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    CHECK_FALSE (host.session.loadPreset (juce::ValueTree ("SomethingElse"), "Something Else"));
    CHECK_FALSE (host.session.loadPreset ({}, "Nothing"));
    CHECK (host.value (TestProcessor::invertId) == 1.0f);
    CHECK (host.history.undoSteps() == 1);
}

TEST_CASE ("Loading a Preset makes it the side's Loaded Preset, unmodified; the other side's is unchanged")
{
    const auto vocal = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::gainId, -7.0f } });
    const auto kick = presetWith ({ { TestProcessor::invertId, 1.0f }, { TestProcessor::gainId, 4.0f } });
    Host host;
    CHECK (host.session.loadedPresetName().isEmpty());
    CHECK_FALSE (host.session.isLoadedPresetModified());

    host.session.loadPreset (vocal, "Vocal");
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK_FALSE (host.session.isLoadedPresetModified());

    host.session.selectCompareSide (CompareSide::B);
    host.session.loadPreset (kick, "Kick");
    CHECK (host.session.loadedPresetName() == "Kick");
    CHECK_FALSE (host.session.isLoadedPresetModified());
    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("Changing a setting a Preset holds marks the side Modified; changing it back, or undoing, clears the mark")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f }, { TestProcessor::gainId, -7.0f } });
    Host host;
    host.session.loadPreset (preset, "Vocal");

    host.edit (TestProcessor::gainId, 3.0f);
    CHECK (host.session.isLoadedPresetModified());
    host.edit (TestProcessor::gainId, -7.0f);
    CHECK_FALSE (host.session.isLoadedPresetModified());

    // A setting the Preset left out counts as its default.
    host.edit (TestProcessor::frequencyId, 2000.0f);
    CHECK (host.session.isLoadedPresetModified());
    host.history.undo();
    CHECK_FALSE (host.session.isLoadedPresetModified());
    CHECK (host.session.loadedPresetName() == "Vocal");
}

TEST_CASE ("Global Bypass, the plugin's view, the window's size and the UI Scale never make a side Modified")
{
    const auto preset = presetWith ({ { TestProcessor::bypassId, 1.0f } });
    Host host;
    host.session.loadPreset (preset, "Vocal");
    host.edit (TestProcessor::globalBypassId, 1.0f);
    host.processor.setZoom (3);
    host.session.setEditorSize ({ 700, 500 });
    host.session.pickUiScale (150);
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("A side with no Loaded Preset is never Modified")
{
    Host host;
    host.edit (TestProcessor::invertId, 1.0f);
    CHECK (host.session.loadedPresetName().isEmpty());
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("A side switched away from and back to stays unmodified")
{
    // Switching puts the side's settings back on the parameters from their plain values.
    const auto preset = presetWith ({ { TestProcessor::frequencyId, 1234.5f }, { TestProcessor::gainId, 3.3f }, { TestProcessor::ratioId, 7.7f } });
    Host host;
    host.session.loadPreset (preset, "Odd");
    host.session.selectCompareSide (CompareSide::B);
    host.edit (TestProcessor::gainId, 5.0f);
    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.session.loadedPresetName() == "Odd");
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("Undo of a Preset load puts back the side's previous Loaded Preset, or none; redo restores it")
{
    const auto vocal = presetWith ({ { TestProcessor::bypassId, 1.0f } });
    const auto kick = presetWith ({ { TestProcessor::invertId, 1.0f } });
    Host host;
    host.session.loadPreset (vocal, "Vocal");
    host.edit (TestProcessor::frequencyId, 3000.0f);
    host.session.loadPreset (kick, "Kick");

    host.history.undo();
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK (host.session.isLoadedPresetModified());
    host.history.undo();
    host.history.undo();
    CHECK (host.session.loadedPresetName().isEmpty());

    host.history.redo();
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK_FALSE (host.session.isLoadedPresetModified());
    host.history.redo();
    host.history.redo();
    CHECK (host.session.loadedPresetName() == "Kick");
    CHECK_FALSE (host.session.isLoadedPresetModified());
}

TEST_CASE ("Copy to the other side, and B's first selection, give the other side the Loaded Preset and Modified state")
{
    const auto vocal = presetWith ({ { TestProcessor::bypassId, 1.0f } });
    const auto kick = presetWith ({ { TestProcessor::invertId, 1.0f } });
    const bool modified = GENERATE (false, true);
    const auto how = GENERATE (Catch::Generators::as<std::string> {}, "first selection", "copy from A", "copy from B");
    CAPTURE (modified, how);
    Host host;
    // The side copied from holds Vocal; the other side Kick, until the copy.
    const auto from = how == "copy from B" ? CompareSide::B : CompareSide::A;
    const auto to = from == CompareSide::A ? CompareSide::B : CompareSide::A;
    if (how != "first selection")
    {
        host.session.selectCompareSide (to);
        host.session.loadPreset (kick, "Kick");
    }
    host.session.selectCompareSide (from);
    host.session.loadPreset (vocal, "Vocal");
    if (modified)
        host.edit (TestProcessor::gainId, 4.0f);

    if (how != "first selection")
        host.session.copyToOther();
    host.session.selectCompareSide (to);
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK (host.session.isLoadedPresetModified() == modified);
}

TEST_CASE ("Settings saved as a Preset make it the side's Loaded Preset, unmodified, as one undo step")
{
    const auto vocal = presetWith ({ { TestProcessor::bypassId, 1.0f } });
    Host host;
    host.session.loadPreset (vocal, "Vocal");
    host.edit (TestProcessor::gainId, 4.0f);
    REQUIRE (host.session.isLoadedPresetModified());
    const int steps = host.history.undoSteps();

    host.session.presetSaved (host.session.presetState(), "Vocal Louder");
    CHECK (host.session.loadedPresetName() == "Vocal Louder");
    CHECK_FALSE (host.session.isLoadedPresetModified());
    host.edit (TestProcessor::gainId, 5.0f);
    CHECK (host.session.isLoadedPresetModified());

    CHECK (host.history.undoSteps() == steps + 2);
    host.history.undo();
    host.history.undo();
    CHECK (host.session.loadedPresetName() == "Vocal");
    CHECK (host.session.isLoadedPresetModified());
}

TEST_CASE ("Each side remembers the folder its Loaded Preset came from, while it keeps that name, without saving it")
{
    const auto x = presetWith ({ { TestProcessor::bypassId, 1.0f } });
    Host host;
    host.session.loadPreset (x, "X", "Factory");
    host.session.selectCompareSide (CompareSide::B);
    // B, selected first, takes A's Loaded Preset and where it came from.
    CHECK (host.session.loadedPresetFolder() == "Factory");
    host.session.loadPreset (x, "X", "User");
    CHECK (host.session.loadedPresetFolder() == "User");
    host.session.selectCompareSide (CompareSide::A);
    CHECK (host.session.loadedPresetFolder() == "Factory");

    // Copy gives the other side where it came from too.
    host.session.copyToOther();
    host.session.selectCompareSide (CompareSide::B);
    CHECK (host.session.loadedPresetFolder() == "Factory");

    // Saving names the folder saved to; a load from a file, none.
    host.session.presetSaved (host.session.presetState(), "Y", "User/Drums");
    CHECK (host.session.loadedPresetFolder() == "User/Drums");
    host.session.loadPreset (x, "Z");
    CHECK (host.session.loadedPresetFolder().isEmpty());

    // An undo to another name leaves none; nothing of it is in the saved state.
    host.history.undo();
    CHECK (host.session.loadedPresetName() == "Y");
    CHECK (host.session.loadedPresetFolder().isEmpty());

    Host plain;
    plain.session.loadPreset (x, "X");
    Host remembering;
    remembering.session.loadPreset (x, "X", "Factory");
    juce::MemoryBlock withFolder, without;
    remembering.processor.getStateInformation (withFolder);
    plain.processor.getStateInformation (without);
    CHECK (withFolder == without);
}

TEST_CASE ("A Preset file with nan, inf and out-of-range values loads in-range settings: defaults where a value isn't a number or is an infinity its range doesn't end at, the range's end where it is outside")
{
    Host maker;
    maker.edit (TestProcessor::invertId, 1.0f);
    const auto xml = maker.session.presetState().createXml();
    REQUIRE (xml != nullptr);
    const auto setSaved = [&xml] (const juce::String& id, const juce::String& text) {
        for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
            if (child->getStringAttribute ("id") == id)
            {
                child->setAttribute ("value", text);
                return;
            }
        FAIL ("No saved setting " << id);
    };
    setSaved (TestProcessor::frequencyId, "nan");
    setSaved (TestProcessor::gainId, "inf");
    setSaved (TestProcessor::shapeId, "42");
    setSaved (TestProcessor::placementId, "-1000");
    setSaved (TestProcessor::ratioId, "inf");
    juce::TemporaryFile folder;
    REQUIRE (folder.getFile().createDirectory());
    const auto& library = maker.session.presetLibrary();
    const auto file = folder.getFile().getChildFile ("Odd" + library.fileExtension());
    REQUIRE (xml->writeTo (file));
    const auto preset = library.read (file);
    REQUIRE (preset.isValid());

    Host host;
    REQUIRE (host.session.loadPreset (preset, "Odd"));
    REQUIRE (harness::firstParameterOutOfRange (host.processor).isEmpty());
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinAbs (1000.0, 1.0e-3));
    CHECK (host.value (TestProcessor::gainId) == 0.0f);
    CHECK (juce::exactlyEqual (host.value (TestProcessor::shapeId), static_cast<float> (TestProcessor::shapeNames().size() - 1)));
    CHECK (host.value (TestProcessor::placementId) == 0.0f);
    CHECK (std::isinf (host.value (TestProcessor::ratioId)));
    CHECK (host.value (TestProcessor::invertId) == 1.0f);
    // Loaded as sanitised, the side is not Modified.
    CHECK_FALSE (host.session.isLoadedPresetModified());
    folder.getFile().deleteRecursively();
}
