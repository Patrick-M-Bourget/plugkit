#pragma once

// New in plugkit, lifted from eq1's plugin/HeaderBar.h (eq1 at dbb1127): CompareButton as it is there, and
// CompareGroup, HeaderBar's Undo, Redo, A/B Compare and Copy with their layout, taking the Session, where HeaderBar
// takes eq1's PluginProcessor. The wordmark, the Preset group (PresetBar) and the header's layout stay eq1's.

#include "ABCompare.h"
#include "EditorTick.h"
#include "staple/controls/IconButton.h"
#include "staple/controls/TextChip.h"
#include "staple/controls/Tween.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace plugkit::ui
{

class Session;

// The A/B Compare button (HANDOFF.md §5.6): one plain text button reading "A/B", the side you're on
// in text1 and the other in text4, the slash in text4 at 400. The letters' colours change over dur2.
// A screen reader reads the side you're on as its value.
class CompareButton final : public staple::TextChip
{
public:
    CompareButton();

    void showSide (CompareSide side);
    // A letter's colour now, part way through a change over dur2.
    juce::Colour letterInk (CompareSide letter) const;
    int getIdealWidth() const;

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    CompareSide side = CompareSide::A, previousSide = CompareSide::A;
    staple::Tween change { staple::tokens::motion::dur2Ms, 1.0f }; // from previousSide to side
};

// The header's right group (HANDOFF.md §4, §5.6): Undo, Redo, A/B Compare and Copy, left to right, ending at its
// right edge. Copy copies the side you're on to the other side, its tooltip naming the direction, and reads "Copied"
// for 1 s. A click on any of them leaves keyboard focus where it was. It follows the side and what can be undone on
// the editor's tick (EditorTick).
class CompareGroup final : public juce::Component, private EditorTick::Client
{
public:
    explicit CompareGroup (Session& session);

    void resized() override;
    // Its buttons' widths and gaps, Copy as wide as "Copied".
    int getIdealWidth() const;

    void undo();
    void redo();
    // Enables Undo and Redo by what can be undone and redone: what an edit elsewhere in the editor (a Preset load)
    // calls, so they don't wait for the tick.
    void showUndoState();

private:
    void tick (double seconds) override;
    // While Copy reads "Copied", so the second it reads it for is kept to the frame.
    bool isActive() const override { return copiedUntil != 0; }
    // Follows the side, which edits, undo and restoring a session change too.
    void showSide();
    void copyToOther();
    void edited();

    Session& session;
    staple::IconButton undoButton { "Undo", staple::Icon::undo }, redoButton { "Redo", staple::Icon::redo };
    CompareButton compareButton;
    staple::TextChip copyButton { "Copy", staple::TextChip::Look::plain, staple::tokens::size::fs4 };
    int copyWidth = 0;
    juce::uint32 copiedUntil = 0; // while Copy reads "Copied": when it reads "Copy" again
};

} // namespace plugkit::ui
