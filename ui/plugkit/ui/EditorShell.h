#pragma once

// Copied from eq1's plugin/PluginEditor.h (eq1 at dbb1127), in namespace plugkit::ui: its parts that aren't eq1's, as
// a base for a plugin's editor, taking the Session and the plugin's wordmark where eq1's takes eq1's PluginProcessor.
// The window's limits are the Session's (Session::Window), where eq1's are its tokens. eq1's EQ display, Band panel,
// Detection Range bar, Display Range, Output Meter and footer are a plugin's, which it lays out below the header.

#include "EditorTick.h"
#include "HeaderBar.h"
#include "KeyboardControl.h"
#include "PresetSettings.h"
#include "staple/CachedLayer.h"
#include "staple/LookAndFeel.h"
#include "staple/TooltipWindow.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace plugkit::ui
{

class Session;

// The native editor's shell, laid out as the Staple window (HANDOFF.md §4, §9B): the header (HeaderBar), the plugin's
// body and its footer, over bg0 and three soft highlights (staple::paintWindowBackground). The header and footer keep
// their height; the body takes the rest. The Preset browser opens as a modal over everything. Resizable within the
// Session's window limits, and drawn at the instance's UI Scale through one transform on the content component,
// marked as the overlay layer, so popovers and knob tooltips float in it scaled; everything is drawn as vectors, so it
// stays sharp at any UI Scale and display scale. It owns the one tick its components follow the processor on
// (EditorTick), and follows the window's size and UI Scale on it itself, so a restored session shows its own.
//
// A plugin's editor derives from it: it adds its components to content() (and its footer's), numbers their Tab order
// from firstFocusOrder, lays them out in layOutBody, and calls open (footer) last in its constructor.
class EditorShell : public juce::AudioProcessorEditor, public EditorTick::Host, private EditorTick::Client
{
public:
    struct Config
    {
        // The plugin's name, which the header shows as its wordmark.
        juce::String wordmark;
        // The parameter Cmd-B toggles: Global Bypass. None when the processor has no parameter with this ID.
        juce::String globalBypassId = plugkit::ui::globalBypassId;
    };

    EditorShell (juce::AudioProcessor& processor, Session& session, Config config);
    ~EditorShell() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    // Cmd-Z undoes, Shift-Cmd-Z (or Cmd-Y) redoes and Cmd-B toggles Global Bypass; Ctrl on Windows.
    // With nothing focused, Tab focuses the first control and Shift+Tab the last.
    bool keyPressed (const juce::KeyPress& key) override;
    // Any click in the editor hides the focus ring until a key brings it back. A plugin's override calls this.
    void mouseDown (const juce::MouseEvent& e) override;

    EditorTick& editorTick() override { return ticker; }

    HeaderBar& header() { return headerBar; }
    // Sizes the window to the Session's logical size and UI Scale, kept within its limits: what the plugin's UI Scale
    // menu calls after a pick (UiScaleMenu::onPicked), so the window doesn't wait for the tick.
    void applyUiScale();
    // Global Bypass on or off, as one undo step: what Cmd-B does.
    void toggleGlobalBypass();

    // The Tab order of the plugin's first component in content(): the header and the Preset browser come before it.
    static constexpr int firstFocusOrder = 3;

protected:
    // Everything the editor shows, laid out in logical pixels (as at 100%) and drawn at the UI Scale through one
    // transform on it. The plugin adds its components here.
    juce::Component& content() { return contentComponent; }
    staple::LookAndFeel& stapleLookAndFeel() { return lookAndFeel; }

    // The last call in the plugin editor's constructor, once its components are in content(): adds footer along the
    // bottom, after them in Tab's order, then takes the LookAndFeel, takes in every control for the editor-wide keys
    // (KeyboardControl) and sizes the window (applyUiScale).
    void open (juce::Component& footer);

    // Lays out the plugin's components in area: content()'s logical pixels between the header and the footer, inside
    // the window's padding, with a gap above and below. A component may reach outside it (to the window's edge).
    virtual void layOutBody (juce::Rectangle<int> area) = 0;

private:
    // Follows the window's size and UI Scale, restored with the plugin's state.
    void tick (double seconds) override;

    Session& session;
    const Config config;
    // First, so it outlives every Client below.
    EditorTick ticker;
    // Every component below, and the plugin's, draws with it, so it outlives them all.
    staple::LookAndFeel lookAndFeel;
    // It holds every component below, and the plugin's.
    juce::Component contentComponent;
    // The window's background, drawn again only when the window's size or the scale it is drawn at
    // changes: whatever repaints over it then costs a copy of it.
    staple::CachedLayer<int> background;
    HeaderBar headerBar;
    staple::TooltipWindow tooltips { &contentComponent };
    juce::Component* footer = nullptr; // the plugin's, once open
    float scale = 1.0f;
    // What the window last took from the Session, to follow a restored session.
    int shownScalePercent = 0;
    juce::Point<int> shownSize;
    bool applyingScale = true; // until the window first takes its size

    // Last, so it lets go of the shell's controls before they go; the plugin's go first, and it holds them safely.
    KeyboardControl keyboard;
};

} // namespace plugkit::ui
