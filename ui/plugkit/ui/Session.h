#pragma once

// New in plugkit, lifted from eq1's plugin/PluginProcessor.h and plugin/PluginProcessor.cpp (eq1 at dbb1127): the
// edit history, A/B Compare, the Preset Library and the window's state its processor holds, with presetState,
// loadPreset and the save and restore order of its getStateInformation and setStateInformation. The restore rule is
// comp1's: an infinity at a parameter's own range end is kept, where eq1's restores the default, and a parameter the
// state lacks is added at its default explicitly, which eq1 leaves to replaceState (PresetSettings.h).

#include "ABCompare.h"
#include "EditHistory.h"
#include "PresetLibrary.h"
#include "PresetSettings.h"
#include "UserSettings.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <functional>
#include <vector>

namespace plugkit::ui
{

// The Plugin Shell's session state, which a plugin's processor owns, built after its parameters: the undo
// history of the editor's edits (EditHistory), A/B Compare (ABCompare), the Preset Library (PresetLibrary), and the
// editor window's logical size and UI Scale. The editor's widgets (PresetBar, CompareGroup, UiScaleMenu) take it.
//
// Saved with the session: every host parameter, A/B Compare's side, the other side's settings and each side's
// Loaded Preset, the window's size, the UI Scale once the instance has one, the version, and what the plugin adds
// itself (its view). Not saved: the undo history, which empties on restore, the Loaded Preset's folder, and the
// plugin's transient state, which the plugin clears itself before restoring.
//
// A Preset holds every Preset setting (Config::isPresetSetting) as plain values with the version, and nothing of the
// view, UI Scale or window size. Loading one is one undo step: the settings it lacks go to their defaults, the rest
// (Global Bypass, the view) is untouched, an older format migrates first, and anything that isn't a Preset changes
// nothing. Modified compares only Preset settings. Neither the window's size nor its UI Scale is an undo step or
// makes a side Modified.
//
// Message thread only, except compareSide(), the window's size, saved() and restore().
class Session
{
public:
    // The window's logical size (as at 100% UI Scale): a new instance's, and the limits a restored one is held to.
    struct Window
    {
        juce::Point<int> defaultSize, minimum, maximum;
    };

    struct Config
    {
        // The settings a Preset and each A/B side hold: by default every parameter but Global Bypass.
        IsPresetSetting isPresetSetting = everyParameterButGlobalBypass;
        // Names the User Preset folder (Documents/<productName>/Presets), the per-user settings' folder and their
        // lock (UserSettings).
        juce::String productName;
        // A Preset file's extension, with its dot, and a Preset's root type, the saved state's type (empty for the
        // parameters' state's type).
        juce::String presetFileExtension;
        juce::Identifier presetType;
        // The plugin's Factory Presets, from its own juce_add_binary_data.
        std::vector<PresetLibrary::FactoryPresetData> factoryPresets;
        // The version of the saved state's format, which saved() and presetState() write, and the plugin's
        // migration, which brings an older state up to it one version at a time (ABCompare::initialState() is the
        // A/B Compare child of a state from before it). Each plugin owns its version history.
        int stateVersion = 1;
        std::function<void (juce::ValueTree&)> migrate;
        Window window;
        // Where User Presets are kept (empty for Documents/<productName>/Presets) and the per-user settings file
        // (empty for none, so nothing is kept: UserSettings::defaultFile (productName) is the plugin's).
        juce::File presetFolder, userSettingsFile;
    };

    Session (juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& parameters, Config config);

    // The undo history of the editor's edits. It outlives the editor, and restoring a session empties it.
    EditHistory& editHistory() { return history; }

    // A/B Compare: which side the host parameters hold, from any thread; selecting a side and copying the side
    // you're on to the other side, each one undo step.
    CompareSide compareSide() const { return compare.side(); }
    void selectCompareSide (CompareSide side) { compare.select (side); }
    void copyToOther() { compare.copyToOther(); }

    PresetLibrary& presetLibrary() { return library; }
    // The settings a Preset holds, from the side you're on, in the saved state's format.
    juce::ValueTree presetState();
    // Puts a Preset's settings on the side you're on and makes it the side's Loaded Preset, named name, as one undo
    // step, bringing an older version of the format up to date first. Anything that isn't a Preset changes nothing
    // and returns false. folder is where the browser lists it, empty for a Preset from elsewhere
    // (ABCompare::loadedPresetFolder).
    bool loadPreset (const juce::ValueTree& preset, const juce::String& name, const juce::String& folder = {});
    // The side you're on was saved as a Preset (presetState()) named name, into folder: it becomes the side's Loaded
    // Preset, as one undo step.
    void presetSaved (const juce::ValueTree& preset, const juce::String& name, const juce::String& folder = {})
    {
        compare.presetSaved (preset, name, folder);
    }
    // The Loaded Preset of the side you're on, or an empty name for none; the folder it was last loaded or saved
    // in, in memory only (ABCompare); and whether the side is Modified.
    juce::String loadedPresetName() const { return compare.loadedPresetName(); }
    juce::String loadedPresetFolder() const { return compare.loadedPresetFolder(); }
    bool isLoadedPresetModified() const { return compare.isModified(); }

    // The editor's window: its size in logical pixels (as at 100%), from any thread, and its UI Scale in percent
    // (one of uiScale::percents). Saved with the session, not in Presets, not undoable.
    const Window& window() const { return config.window; }
    juce::Point<int> editorSize() const { return { editorWidth.load(), editorHeight.load() }; }
    void setEditorSize (juce::Point<int> logical);
    // An instance that has never had a UI Scale, new or from a session saved before it, takes the one last picked
    // in any instance here (UserSettings), and keeps it from then on.
    int uiScalePercent();
    // The UI Scale picked by hand: this instance's, and the default for new ones.
    void pickUiScale (int percent);

    // The session to save, from any thread: A/B Compare's race-safe copy of the parameters' state
    // (ABCompare::savedState), with the version, the window's size and the UI Scale once the instance has one. The
    // plugin adds its view's properties and children to it, then writes it out.
    juce::ValueTree saved() const;

    // Reads and removes the plugin's own view properties and children from a state being restored.
    using ViewReader = std::function<void (juce::ValueTree& state)>;
    // Restores a saved session, from any thread, as hosts restore from theirs: one of another type (not the saved
    // state's) changes nothing and returns false. In order, it runs the plugin's migration, takes off the version,
    // reads and takes off the window's size and UI Scale, calls readView, takes off the A/B Compare child and
    // restores it (none puts the settings on A, with B a copy), restores the parameters, and empties the undo
    // history. A parameter's saved value restores as restoredPlainValue() gives it, one the state lacks at its
    // default, and what the state holds that this version doesn't know is kept and saved again as it was
    // (sanitiseSavedSettings). Clearing the plugin's transient state is the plugin's, before it calls this.
    bool restore (juce::ValueTree state, const ViewReader& readView = {});

private:
    juce::AudioProcessorValueTreeState& parameters;
    const Config config;
    const juce::Identifier presetType; // config's, or the saved state's type
    EditHistory history;
    ABCompare compare;
    PresetLibrary library;
    UserSettings userSettings;
    // Kept out of the parameter state, which a host may save from another thread, and written into a copy of it
    // when saving.
    std::atomic<int> editorWidth, editorHeight;
    std::atomic<int> uiScale { 0 }; // 0 until the instance has one

    JUCE_DECLARE_NON_COPYABLE (Session)
};

} // namespace plugkit::ui
