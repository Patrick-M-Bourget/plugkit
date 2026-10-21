#pragma once

// plugkit's stand-in for a plugin's processor in plugkit-ui's tests: host parameters of each kind the kit's controls
// attach to, and an editor with Staple's look, its tick and a Knob for each number. Its ranges and text are eq1's Band Frequency, Gain, Shape and Stereo Placement ones
// (eq1's plugin/Parameters.cpp), so the tests copied from eq1 check the same values.

#include "plugkit/ui/EditHistory.h"
#include "plugkit/ui/EditorTick.h"
#include "plugkit/ui/staple/LookAndFeel.h"
#include "plugkit/ui/staple/controls/Knob.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <memory>

namespace plugkit::test
{

class TestProcessor : public juce::AudioProcessor
{
public:
    // Frequency on a log scale, 10 Hz to 30 kHz from 1 kHz, in whole tenths; Gain linear, -30 to +30 dB from 0, to
    // 2 decimals; two switches; and two choices.
    static constexpr const char* frequencyId = "frequency";
    static constexpr const char* gainId = "gain";
    static constexpr const char* invertId = "invert";
    static constexpr const char* bypassId = "bypass";
    static constexpr const char* shapeId = "shape";
    static constexpr const char* placementId = "placement";

    static const juce::StringArray& shapeNames()
    {
        static const juce::StringArray names { "Bell",      "Low Shelf", "Low Cut",    "High Shelf", "High Cut",
                                               "Notch",     "Band Pass", "Tilt Shelf", "Flat Tilt",  "All Pass" };
        return names;
    }

    static const juce::StringArray& placementNames()
    {
        static const juce::StringArray names { "Stereo", "Left", "Right", "Mid", "Side" };
        return names;
    }

    TestProcessor()
        : AudioProcessor (BusesProperties()
                              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    juce::AudioProcessorValueTreeState& parameterState() { return parameters; }
    // The undo history of the editor's edits.
    plugkit::ui::EditHistory& editHistory() { return history; }

    const juce::String getName() const override { return "plugkit test"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

private:
    static juce::AudioParameterFloatAttributes withText (int decimals, const juce::String& label)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (label)
            .withStringFromValueFunction ([decimals] (float value, int) { return juce::String (value, decimals); })
            .withValueFromStringFunction ([] (const juce::String& text) { return text.getFloatValue(); });
    }

    static juce::NormalisableRange<float> logRange (float low, float high)
    {
        return { low,
                 high,
                 [] (float start, float end, float normalised) {
                     return static_cast<float> (start * std::pow (double { end } / start, double { normalised }));
                 },
                 [] (float start, float end, float value) {
                     return static_cast<float> (std::log (double { value } / start) / std::log (double { end } / start));
                 },
                 [] (float, float, float value) { return value; } };
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { frequencyId, 1 }, "Frequency",
                                                                 logRange (10.0f, 30000.0f), 1000.0f, withText (1, "Hz")),
                    std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { gainId, 1 }, "Gain",
                                                                 juce::NormalisableRange<float> { -30.0f, 30.0f }, 0.0f,
                                                                 withText (2, "dB")),
                    std::make_unique<juce::AudioParameterBool> (juce::ParameterID { invertId, 1 }, "Phase Invert", false),
                    std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bypassId, 1 }, "Bypass", false),
                    std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { shapeId, 1 }, "Shape", shapeNames(), 0),
                    std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { placementId, 1 }, "Stereo Placement",
                                                                  placementNames(), 0));
        return layout;
    }

    juce::AudioProcessorValueTreeState parameters { *this, nullptr, "plugkit_test", layout() };
    plugkit::ui::EditHistory history { *this };
};

// The test processor's editor: Staple's LookAndFeel, the editor's tick (EditorTick), and a Knob for Frequency and one
// for Gain, each described by and attached to its parameter, as a plugin's editor has them.
class TestEditor final : public juce::AudioProcessorEditor, public plugkit::ui::EditorTick::Host
{
public:
    explicit TestEditor (TestProcessor& p) : AudioProcessorEditor (p), tick (p)
    {
        setLookAndFeel (&lookAndFeel);
        for (auto* knob : { &frequency, &gain })
            addAndMakeVisible (*knob);
        frequency.describe (frequencyParameter);
        gain.describe (gainParameter);
        setSize (300, 200);
    }

    ~TestEditor() override { setLookAndFeel (nullptr); }

    plugkit::ui::EditorTick& editorTick() override { return tick; }

    void resized() override
    {
        frequency.setBounds (20, 40, frequency.getIdealSize(), frequency.getIdealSize());
        gain.setBounds (160, 40, gain.getIdealSize(), gain.getIdealSize());
    }

private:
    staple::LookAndFeel lookAndFeel;
    plugkit::ui::EditorTick tick;
    juce::RangedAudioParameter& frequencyParameter = *dynamic_cast<TestProcessor&> (processor).parameterState().getParameter (TestProcessor::frequencyId);
    juce::RangedAudioParameter& gainParameter = *dynamic_cast<TestProcessor&> (processor).parameterState().getParameter (TestProcessor::gainId);
    staple::Knob frequency { staple::tokens::knob::frequency, "Frequency" }, gain { staple::tokens::knob::gain, "Gain" };
    juce::SliderParameterAttachment frequencyAttachment { frequencyParameter, frequency }, gainAttachment { gainParameter, gain };
};

inline juce::AudioProcessorEditor* TestProcessor::createEditor() { return new TestEditor (*this); }

} // namespace plugkit::test
