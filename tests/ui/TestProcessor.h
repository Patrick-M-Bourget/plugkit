#pragma once

// plugkit's stand-in for a plugin's processor in plugkit-ui's tests: host parameters of each kind the kit's controls
// attach to, its session state saved and restored through a Session, as a plugin's is, and an editor with the
// session's widgets and a control of each kind (TestEditor). Its ranges and text are eq1's Band Frequency, Gain, Shape and Stereo
// Placement ones (eq1's plugin/Parameters.cpp), so the tests copied from eq1 check the same values; its Global Bypass
// is eq1's, and its Ratio is comp1's, whose range ends at +inf.

#include "plugkit/ui/CompareGroup.h"
#include "plugkit/ui/EditorTick.h"
#include "plugkit/ui/KeyboardControl.h"
#include "plugkit/ui/PresetBar.h"
#include "plugkit/ui/Session.h"
#include "plugkit/ui/UiScaleMenu.h"
#include "plugkit/ui/staple/LookAndFeel.h"
#include "plugkit/ui/staple/controls/Knob.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace plugkit::test
{

class TestProcessor : public juce::AudioProcessor
{
public:
    // Frequency on a log scale, 10 Hz to 30 kHz from 1 kHz, in whole tenths; Gain linear, -30 to +30 dB from 0, to
    // 2 decimals; two switches; two choices; Global Bypass, in no Preset or A/B side; and Ratio, 1:1 to inf:1 from
    // 4:1. Each comes after the ones before it, so their indexes stay.
    static constexpr const char* frequencyId = "frequency";
    static constexpr const char* gainId = "gain";
    static constexpr const char* invertId = "invert";
    static constexpr const char* bypassId = "bypass";
    static constexpr const char* shapeId = "shape";
    static constexpr const char* placementId = "placement";
    static constexpr const char* globalBypassId = "global_bypass";
    static constexpr const char* ratioId = "ratio";

    // The saved state's type, and the version of its format: version 1 is the state with its version, and version 2
    // added A/B Compare.
    static constexpr const char* stateType = "plugkit_test";
    static constexpr int stateVersion = 2;
    // The window's logical size, and its limits.
    static constexpr plugkit::ui::Session::Window window { { 640, 400 }, { 480, 300 }, { 1280, 800 } };

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

    // The Factory Presets, as a plugin's binary data holds them: Warm and Bright.
    static const std::vector<plugkit::ui::PresetLibrary::FactoryPresetData>& factoryPresets()
    {
        static const char warm[] = R"(<plugkit_test version="2"><PARAM id="gain" value="3.0"/></plugkit_test>)";
        static const char bright[] = R"(<plugkit_test version="2"><PARAM id="frequency" value="8000.0"/></plugkit_test>)";
        static const std::vector<plugkit::ui::PresetLibrary::FactoryPresetData> presets {
            { "Warm", warm, static_cast<int> (sizeof (warm) - 1) },
            { "Bright", bright, static_cast<int> (sizeof (bright) - 1) },
        };
        return presets;
    }

    // Its Preset Library's config, as its Session builds the library.
    static plugkit::ui::PresetLibrary::Config libraryConfig()
    {
        return { "plugkit test", ".plugkittest", stateType, factoryPresets() };
    }

    // Brings a saved state or a Preset from an older version up to stateVersion, one version at a time.
    static void migrate (juce::ValueTree& state)
    {
        const int version = state.getProperty ("version", 0);
        if (version < 1)
            state.setProperty ("version", 1, nullptr);
        // 1 to 2: A/B Compare. The settings are side A's, and B is a copy of them.
        if (version < 2)
        {
            state.appendChild (plugkit::ui::ABCompare::initialState(), nullptr);
            state.setProperty ("version", 2, nullptr);
        }
    }

    // Its per-user settings in userSettingsFile (none kept without one), and its User Presets in presetFolder
    // (Documents/plugkit test/Presets without one).
    explicit TestProcessor (juce::File userSettingsFile = {}, juce::File presetFolder = {})
        : AudioProcessor (BusesProperties()
                              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          session (*this, parameters, sessionConfig (std::move (userSettingsFile), std::move (presetFolder)))
    {
    }

    juce::AudioProcessorValueTreeState& parameterState() { return parameters; }
    plugkit::ui::Session& sessionState() { return session; }
    // The undo history of the editor's edits.
    plugkit::ui::EditHistory& editHistory() { return session.editHistory(); }

    // The plugin's own view setting, as eq1's Display Range is: saved with the session, read back by its view
    // reader, and in no Preset. 1 in a session saved without it.
    int zoom() const { return zoomLevel.load(); }
    void setZoom (int level) { zoomLevel = level; }

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

    // As a plugin saves: the Session's state with its own view added.
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        auto state = session.saved();
        state.setProperty (zoomProperty, zoom(), nullptr);
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }

    // As a plugin restores: through the Session, its view read back by its own reader.
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            session.restore (juce::ValueTree::fromXml (*xml), [this] (juce::ValueTree& state) {
                setZoom (state.getProperty (zoomProperty, 1));
                state.removeProperty (zoomProperty, nullptr);
            });
    }

private:
    static inline const juce::Identifier zoomProperty { "zoom" };

    static plugkit::ui::Session::Config sessionConfig (juce::File userSettingsFile, juce::File presetFolder)
    {
        const auto library = libraryConfig();
        plugkit::ui::Session::Config config;
        config.productName = library.productName;
        config.presetFileExtension = library.fileExtension;
        config.presetType = library.rootType;
        config.factoryPresets = library.factoryPresets;
        config.stateVersion = stateVersion;
        config.migrate = migrate;
        config.window = window;
        config.presetFolder = std::move (presetFolder);
        config.userSettingsFile = std::move (userSettingsFile);
        return config;
    }

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

    // comp1's Ratio (its plugin/Parameters.cpp): 1:1 to inf:1, the travel the square of 1 - 1/Ratio, so inf:1 is the
    // top, reached exactly.
    static juce::NormalisableRange<float> ratioRange()
    {
        return { 1.0f,
                 std::numeric_limits<float>::infinity(),
                 [] (float, float, float normalised) { return static_cast<float> (1.0 / (1.0 - std::sqrt (double { normalised }))); },
                 [] (float, float, float value) {
                     const double taken = 1.0 - 1.0 / double { value };
                     return static_cast<float> (taken * taken);
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
                                                                  placementNames(), 0),
                    std::make_unique<juce::AudioParameterBool> (juce::ParameterID { globalBypassId, 1 }, "Global Bypass", false),
                    std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ratioId, 1 }, "Ratio", ratioRange(), 4.0f,
                                                                 withText (2, ":1")));
        return layout;
    }

    juce::AudioProcessorValueTreeState parameters { *this, nullptr, stateType, layout() };
    plugkit::ui::Session session;
    std::atomic<int> zoomLevel { 1 };
};

// The test processor's editor, as a plugin's has its parts: Staple's LookAndFeel and the editor's tick (EditorTick);
// along its top the Preset group (PresetBar) and Undo, Redo, A/B Compare and Copy (CompareGroup); a Knob for Frequency
// and one for Gain, each described by and attached to its parameter; a row of a toggle (Phase Invert), a button
// (Reset Gain, one undo step) and a ComboBox (Shape); the UI Scale menu at the bottom right; and the Preset browser
// over it all. The editor-wide keys (KeyboardControl) take in every control. It opens at the session's window size,
// and doesn't scale itself: scaling the window is each plugin's editor's.
class TestEditor final : public juce::AudioProcessorEditor, public plugkit::ui::EditorTick::Host
{
public:
    explicit TestEditor (TestProcessor& p)
        : AudioProcessorEditor (p),
          tick (p),
          session (p.sessionState()),
          presetBar (session),
          compareGroup (session),
          uiScale (session),
          keyboard (session.editHistory())
    {
        setLookAndFeel (&lookAndFeel);
        presetBar.onEdit = [this] { compareGroup.showUndoState(); };
        presetBar.onIdealWidthChange = [this] { resized(); };
        shape.addItemList (TestProcessor::shapeNames(), 1);
        shape.setTitle ("Shape");
        invert.setTitle ("Phase Invert");
        resetGain.setTitle ("Reset Gain");
        resetGain.onClick = [this] { plugkit::ui::setTogether ({ { &gainParameter, gainParameter.getDefaultValue() } }); };
        for (juce::Component* child : std::initializer_list<juce::Component*> { &presetBar, &compareGroup, &frequency, &gain, &invert,
                                                                                 &resetGain, &shape, &uiScale })
            addAndMakeVisible (*child);
        addChildComponent (presetBar.browserPanel());
        // Tab's order: the header (and the Preset browser while open), the Knobs, the row and the footer.
        int order = 0;
        for (juce::Component* child : std::initializer_list<juce::Component*> { &presetBar, &presetBar.browserPanel(), &compareGroup, &frequency,
                                                                                 &gain, &invert, &resetGain, &shape, &uiScale })
            child->setExplicitFocusOrder (++order);
        frequency.describe (frequencyParameter);
        gain.describe (gainParameter);
        keyboard.adopt (*this);
        const auto size = session.editorSize();
        setSize (size.x, size.y);
    }

    ~TestEditor() override { setLookAndFeel (nullptr); }

    plugkit::ui::EditorTick& editorTick() override { return tick; }

    // With nothing in the editor focused, Tab focuses its first control and Shift+Tab its last, showing the focus
    // ring. As eq1's PluginEditor::keyPressed (eq1 at dbb1127), without its Global Bypass, undo and redo keys.
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key.isKeyCode (juce::KeyPress::tabKey) && ! isParentOf (getCurrentlyFocusedComponent()))
        {
            const auto all = createKeyboardFocusTraverser()->getAllComponents (this);
            if (all.empty())
                return false;
            lookAndFeel.showFocusRing (true);
            (key.getModifiers().isShiftDown() ? all.back() : all.front())->grabKeyboardFocus();
            return true;
        }
        return false;
    }

    void resized() override
    {
        auto header = getLocalBounds().removeFromTop (headerHeight).reduced (14, 0);
        presetBar.setBounds (header.removeFromLeft (presetBar.getIdealWidth()));
        compareGroup.setBounds (header.removeFromRight (compareGroup.getIdealWidth()));
        frequency.setBounds (20, 70, frequency.getIdealSize(), frequency.getIdealSize());
        gain.setBounds (160, 70, gain.getIdealSize(), gain.getIdealSize());
        invert.setBounds (20, 220, 120, 28);
        resetGain.setBounds (150, 220, 100, 28);
        shape.setBounds (260, 220, 140, 28);
        uiScale.setBounds (getLocalBounds().removeFromBottom (44).removeFromRight (44).reduced (6));
        presetBar.browserPanel().setBounds (getLocalBounds());
    }

private:
    static constexpr int headerHeight = 52;

    staple::LookAndFeel lookAndFeel;
    plugkit::ui::EditorTick tick;
    plugkit::ui::Session& session;
    juce::AudioProcessorValueTreeState& state = dynamic_cast<TestProcessor&> (processor).parameterState();
    juce::RangedAudioParameter& frequencyParameter = *state.getParameter (TestProcessor::frequencyId);
    juce::RangedAudioParameter& gainParameter = *state.getParameter (TestProcessor::gainId);
    plugkit::ui::PresetBar presetBar;
    plugkit::ui::CompareGroup compareGroup;
    staple::Knob frequency { staple::tokens::knob::frequency, "Frequency" }, gain { staple::tokens::knob::gain, "Gain" };
    juce::SliderParameterAttachment frequencyAttachment { frequencyParameter, frequency }, gainAttachment { gainParameter, gain };
    juce::ToggleButton invert { "Phase Invert" };
    juce::TextButton resetGain { "Reset Gain" };
    juce::ComboBox shape { "Shape" };
    juce::AudioProcessorValueTreeState::ButtonAttachment invertAttachment { state, TestProcessor::invertId, invert };
    juce::AudioProcessorValueTreeState::ComboBoxAttachment shapeAttachment { state, TestProcessor::shapeId, shape };
    plugkit::ui::UiScaleMenu uiScale;
    plugkit::ui::KeyboardControl keyboard;
};

inline juce::AudioProcessorEditor* TestProcessor::createEditor() { return new TestEditor (*this); }

} // namespace plugkit::test
