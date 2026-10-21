#pragma once

// New in plugkit, lifted from eq1's plugin/FooterBar.h (eq1 at dbb1127): its UiScaleMenu, taking the Session, which a
// pick sets and whose UI Scale it shows on the editor's tick, where eq1's FooterBar tells it what to show.

#include "EditorTick.h"
#include "staple/controls/IconButton.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace plugkit::ui
{

class Session;

// The footer's UI Scale menu: an icon that opens a menu titled "UI Scale" of the five UI Scales, the
// current one ticked. A pick is the session's UI Scale (Session::pickUiScale), and the default for new instances.
class UiScaleMenu final : public staple::IconButton, private EditorTick::Client
{
public:
    explicit UiScaleMenu (Session& session);

    juce::PopupMenu menu() const;
    // As a pick in the menu: sets and shows it, and reports it.
    void pick (int percent);
    int shownPercent() const { return shown; }

    // After a pick, with its percent, for the editor to scale itself.
    std::function<void (int)> onPicked;

private:
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    // Follows the session's UI Scale, which a restored session changes too.
    void tick (double seconds) override;

    Session& session;
    int shown = 100;
};

} // namespace plugkit::ui
