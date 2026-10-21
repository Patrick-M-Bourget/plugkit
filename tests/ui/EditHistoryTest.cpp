// Copied from eq1's tests/plugin/EditHistoryTest.cpp (eq1 at dbb1127), its generic cases against plugkit's test
// processor: edits inside gestures on its parameters stand in for eq1's BandEditing (adding, dragging and reshaping
// Bands). Cases for the 500-step limit and for a session restored from another thread while the editor is open are
// added. The Band, Spectrum Grab and wheel cases stay in eq1.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/EditHistory.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <thread>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using plugkit::test::TestProcessor;

namespace
{

// A plugin as a host has it, with its undo history.
struct Host
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
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

    // A drag in the editor: gestures on every id begun first, the values changed, the gestures ended last.
    void beginDrag (std::initializer_list<const char*> ids)
    {
        for (const auto* id : ids)
            parameter (id).beginChangeGesture();
    }
    void dragTo (const juce::String& id, float plain) { parameter (id).setValueNotifyingHost (parameter (id).convertTo0to1 (plain)); }
    void endDrag (std::initializer_list<const char*> ids)
    {
        for (const auto* id : ids)
            parameter (id).endChangeGesture();
    }

    // A change from the host: automation playback or its own parameter view. No gesture.
    void hostSets (const juce::String& id, float plain)
    {
        auto& p = parameter (id);
        p.setValue (p.convertTo0to1 (plain));
        p.sendValueChangedMessageToListeners (p.convertTo0to1 (plain));
    }
};

// Counts the gestures a host sees.
struct GestureLog final : juce::AudioProcessorListener
{
    int begins = 0, ends = 0;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}
    void audioProcessorParameterChangeGestureBegin (juce::AudioProcessor*, int) override { ++begins; }
    void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int) override { ++ends; }
};

} // namespace

TEST_CASE ("The undo history starts empty")
{
    Host host;
    CHECK_FALSE (host.history.canUndo());
    CHECK_FALSE (host.history.canRedo());
    CHECK (host.history.undoSteps() == 0);
}

TEST_CASE ("Each edit is one undo step: undo takes them back in turn, redo brings them back")
{
    Host host;
    host.edit (TestProcessor::gainId, 6.0f);
    host.edit (TestProcessor::shapeId, 3.0f);
    host.edit (TestProcessor::frequencyId, 500.0f);
    CHECK (host.history.undoSteps() == 3);

    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (1000.0f, 1.0e-4f));
    host.history.undo();
    CHECK (host.value (TestProcessor::shapeId) == 0.0f);
    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (0.0, 1.0e-4));
    CHECK_FALSE (host.history.canUndo());

    for (int step = 0; step < 3; ++step)
        host.history.redo();
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
    CHECK (host.value (TestProcessor::shapeId) == 3.0f);
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (500.0f, 1.0e-4f));
    CHECK_FALSE (host.history.canRedo());
}

TEST_CASE ("A drag is one undo step, however far it goes")
{
    Host host;
    host.edit (TestProcessor::gainId, 3.0f);
    host.beginDrag ({ TestProcessor::frequencyId, TestProcessor::gainId });
    for (int step = 1; step <= 10; ++step)
    {
        host.dragTo (TestProcessor::frequencyId, 1000.0f + 100.0f * static_cast<float> (step));
        host.dragTo (TestProcessor::gainId, 3.0f + 0.5f * static_cast<float> (step));
    }
    host.endDrag ({ TestProcessor::frequencyId, TestProcessor::gainId });
    CHECK (host.history.undoSteps() == 2);

    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (1000.0f, 1.0e-4f));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (3.0, 1.0e-4));
    host.history.redo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (2000.0f, 1.0e-4f));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (8.0, 1.0e-4));
}

TEST_CASE ("Edits on a Slider's, a ComboBox's and a ToggleButton's parameters are undo steps")
{
    Host host;
    auto& state = host.processor.parameterState();

    // A Slider, a ComboBox and a ToggleButton, attached as an editor's controls are.
    juce::Slider gain;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment (state, TestProcessor::gainId, gain);
    juce::ComboBox shape;
    shape.addItemList (TestProcessor::shapeNames(), 1);
    juce::AudioProcessorValueTreeState::ComboBoxAttachment shapeAttachment (state, TestProcessor::shapeId, shape);
    juce::ToggleButton invert;
    juce::AudioProcessorValueTreeState::ButtonAttachment invertAttachment (state, TestProcessor::invertId, invert);

    {
        // As a mouse drag, typed value, double-click or wheel on the Slider does.
        const juce::Slider::ScopedDragNotification drag (gain);
        gain.setValue (-6.0, juce::sendNotificationSync);
    }
    shape.setSelectedItemIndex (1, juce::sendNotificationSync);
    invert.setToggleState (true, juce::sendNotificationSync);
    CHECK (host.history.undoSteps() == 3);

    host.history.undo();
    CHECK (host.value (TestProcessor::invertId) == 0.0f);
    host.history.undo();
    CHECK (host.value (TestProcessor::shapeId) == 0.0f);
    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (0.0, 1.0e-3));
}

TEST_CASE ("Host Automation and the host's own parameter view add no undo steps")
{
    Host host;
    host.edit (TestProcessor::gainId, 6.0f);
    REQUIRE (host.history.undoSteps() == 1);

    host.hostSets (TestProcessor::gainId, -12.0f);
    host.hostSets (TestProcessor::frequencyId, 9000.0f);
    host.hostSets (TestProcessor::bypassId, 1.0f);
    CHECK (host.history.undoSteps() == 1);
    // Playing automation back while audio runs, too.
    host.processor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (host.processor.getTotalNumInputChannels(), 512);
    juce::MidiBuffer midi;
    for (int block = 0; block < 4; ++block)
    {
        host.hostSets (TestProcessor::frequencyId, 600.0f + 100.0f * static_cast<float> (block));
        host.processor.processBlock (buffer, midi);
    }
    CHECK (host.history.undoSteps() == 1);
}

TEST_CASE ("Undo and redo reach the host as gestures, and are not themselves undo steps")
{
    Host host;
    host.edit (TestProcessor::gainId, 6.0f);
    host.edit (TestProcessor::shapeId, 5.0f);
    GestureLog log;
    host.processor.addListener (&log);

    host.history.undo();
    CHECK (log.begins >= 1);
    CHECK (log.begins == log.ends);
    CHECK (host.history.undoSteps() == 1);
    CHECK (host.history.canRedo());
    host.history.redo();
    CHECK (host.history.undoSteps() == 2);
    CHECK_FALSE (host.history.canRedo());
    host.processor.removeListener (&log);
}

TEST_CASE ("A new edit after an undo replaces what could be redone")
{
    Host host;
    host.edit (TestProcessor::gainId, 6.0f);
    host.edit (TestProcessor::shapeId, 5.0f);
    host.history.undo();
    REQUIRE (host.history.canRedo());
    host.edit (TestProcessor::frequencyId, 3000.0f);
    CHECK_FALSE (host.history.canRedo());
    CHECK (host.history.undoSteps() == 2);
}

TEST_CASE ("The undo history is empty after a session is reloaded")
{
    Host host;
    host.edit (TestProcessor::gainId, 6.0f);
    host.edit (TestProcessor::shapeId, 5.0f);
    host.history.undo();
    juce::MemoryBlock state;
    host.processor.getStateInformation (state);
    host.processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    CHECK_FALSE (host.history.canUndo());
    CHECK_FALSE (host.history.canRedo());
}

TEST_CASE ("Host Automation during an editor drag stays out of the drag's undo step")
{
    Host host;
    host.beginDrag ({ TestProcessor::frequencyId });
    host.dragTo (TestProcessor::frequencyId, 2000.0f);
    host.hostSets (TestProcessor::gainId, -9.0f);
    host.hostSets (TestProcessor::placementId, 2.0f);
    host.endDrag ({ TestProcessor::frequencyId });
    REQUIRE (host.history.undoSteps() == 1);

    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (1000.0f, 1.0e-4f));
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (-9.0, 1.0e-3));
    CHECK (host.value (TestProcessor::placementId) == 2.0f);
}

TEST_CASE ("A session restored in the middle of a drag leaves the undo history empty")
{
    Host host;
    host.edit (TestProcessor::frequencyId, 100.0f);
    juce::MemoryBlock state;
    host.processor.getStateInformation (state);

    host.beginDrag ({ TestProcessor::frequencyId });
    host.dragTo (TestProcessor::frequencyId, 200.0f);
    host.processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    host.dragTo (TestProcessor::frequencyId, 300.0f);
    host.endDrag ({ TestProcessor::frequencyId });
    CHECK_FALSE (host.history.canUndo());
    CHECK_FALSE (host.history.canRedo());

    // The next edit is recorded as usual.
    host.edit (TestProcessor::shapeId, 5.0f);
    CHECK (host.history.undoSteps() == 1);
}

TEST_CASE ("A session restored from the host's own thread while the editor is open and a drag is under way empties the undo history once the message thread catches up")
{
    harness::OpenEditor<TestProcessor> host;
    auto& history = host.processor.editHistory();
    auto& frequency = host.parameter (TestProcessor::frequencyId);
    frequency.beginChangeGesture();
    host.set (TestProcessor::frequencyId, 500.0f);
    frequency.endChangeGesture();
    juce::MemoryBlock state;
    host.processor.getStateInformation (state);
    REQUIRE (history.undoSteps() == 1);

    frequency.beginChangeGesture();
    host.set (TestProcessor::frequencyId, 2000.0f);
    std::thread hostThread ([&] { host.processor.setStateInformation (state.getData(), static_cast<int> (state.getSize())); });
    hostThread.join();
    // The restore reached the parameters, and the history shows nothing to undo, before the message thread hears of it.
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (500.0f, 1.0e-4f));
    CHECK_FALSE (history.canUndo());
    CHECK_FALSE (history.isEditing());
    host.settle();
    host.set (TestProcessor::frequencyId, 3000.0f);
    frequency.endChangeGesture();
    CHECK_FALSE (history.canUndo());

    // The next edit is recorded as usual.
    frequency.beginChangeGesture();
    host.set (TestProcessor::frequencyId, 4000.0f);
    frequency.endChangeGesture();
    CHECK (history.undoSteps() == 1);
    history.undo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (3000.0f, 1.0e-4f));
}

TEST_CASE ("Undo and redo wait until an edit in progress is finished")
{
    Host host;
    host.edit (TestProcessor::gainId, 3.0f);
    host.beginDrag ({ TestProcessor::frequencyId });
    host.dragTo (TestProcessor::frequencyId, 2000.0f);
    host.history.undo();
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (3.0, 1.0e-4));
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (2000.0f, 1.0e-4f));
    host.endDrag ({ TestProcessor::frequencyId });
    CHECK (host.history.undoSteps() == 2);

    host.history.undo();
    host.beginDrag ({ TestProcessor::frequencyId });
    host.history.redo();
    CHECK_THAT (host.value (TestProcessor::frequencyId), WithinRel (1000.0f, 1.0e-4f));
    host.endDrag ({ TestProcessor::frequencyId });
    CHECK (host.history.canRedo());
}

TEST_CASE ("The undo history keeps the last 500 steps, the oldest going first")
{
    Host host;
    for (int step = 1; step <= 501; ++step)
        host.edit (TestProcessor::gainId, step % 2 == 1 ? 6.0f : -6.0f);
    CHECK (host.history.undoSteps() == 500);
    while (host.history.canUndo())
        host.history.undo();
    // The first edit, from 0 dB to +6 dB, is gone: the oldest kept began at +6 dB.
    CHECK_THAT (host.value (TestProcessor::gainId), WithinAbs (6.0, 1.0e-4));
}
