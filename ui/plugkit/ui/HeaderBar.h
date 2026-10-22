#pragma once

// Copied from eq1's plugin/HeaderBar.h (eq1 at dbb1127), in namespace plugkit::ui: the header's layout, taking the
// Session and the wordmark the plugin passes, where eq1's takes eq1's PluginProcessor and draws staple::wordmark ("eq1").
// Its right group is CompareGroup (CompareGroup.h), which holds what eq1's HeaderBar.h has as CompareButton and its
// Undo, Redo, A/B Compare and Copy.

#include "CompareGroup.h"
#include "PresetBar.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugkit::ui
{

class Session;

// The window's header (HANDOFF.md §4, §5.6, §9B): the wordmark, the plugin's own name, on the left; ‹, the Presets
// button and › in the centre (PresetBar); Undo, Redo, A/B Compare and Copy on the right (CompareGroup). The side
// groups share the leftover width equally, so the Preset group stays centred in the window. Tab reaches the Preset
// group, then the right group.
class HeaderBar final : public juce::Component
{
public:
    HeaderBar (Session& session, juce::String wordmark);

    void paint (juce::Graphics& g) override;
    void resized() override;

    // The Preset browser, for the editor to place over its whole content.
    juce::Component& presetBrowser() { return presetBar.browserPanel(); }
    const juce::String& wordmark() const { return name; }
    juce::Rectangle<int> wordmarkArea() const { return wordmarkBounds; }

    void undo() { compareGroup.undo(); }
    void redo() { compareGroup.redo(); }

private:
    const juce::String name;
    PresetBar presetBar;
    CompareGroup compareGroup;
    juce::Rectangle<int> wordmarkBounds;
};

} // namespace plugkit::ui
