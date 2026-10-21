#pragma once

// Copied from eq1's plugin/ShowMenu.h (eq1 at dbb1127), in namespace plugkit::ui; listed in eq1#231.

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace plugkit::ui
{

// Shows popup for owner (the EQ display's and the Hover Card's Band menu, the Display Range chip's, the
// UI Scale menu and the Band panel's Slope list):
// drawn with owner's look, since a menu is a window of its own and draws with the editor's look only
// when given it, and closed unchosen if owner goes, so its actions never outlive what they use.
// options say where and how; done, when given, is called with the chosen item's id when it closes.
inline void showMenu (juce::PopupMenu popup, juce::Component& owner, const juce::PopupMenu::Options& options,
                      std::function<void (int)> done = {})
{
    popup.setLookAndFeel (&owner.getLookAndFeel());
    if (done)
        popup.showMenuAsync (options.withDeletionCheck (owner), std::move (done));
    else
        popup.showMenuAsync (options.withDeletionCheck (owner));
}

} // namespace plugkit::ui
