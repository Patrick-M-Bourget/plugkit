// New in plugkit, lifted from eq1's plugin/FooterBar.cpp (eq1 at dbb1127): its UiScaleMenu, taking the Session.

#include "UiScaleMenu.h"

#include "Accessibility.h"
#include "Session.h"
#include "ShowMenu.h"
#include "UiScale.h"
#include "ValueText.h"

namespace plugkit::ui
{

UiScaleMenu::UiScaleMenu (Session& s)
    : staple::IconButton ("UI Scale", staple::Icon::uiScale), EditorTick::Client (static_cast<juce::Component&> (*this)), session (s)
{
    setTitle ("UI Scale");
    setTooltip ("UI Scale");
    setIconSize (14.0f);
    shown = session.uiScalePercent();
    onClick = [this] {
        showMenu (menu(), *this, juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<UiScaleMenu> (this)] (int picked) {
            if (safe != nullptr && picked > 0)
                safe->pick (picked);
        });
    };
}

juce::PopupMenu UiScaleMenu::menu() const
{
    juce::PopupMenu items;
    items.addSectionHeader ("UI Scale");
    for (int percent : uiScale::percents)
        items.addItem (percent, valueText::percent (percent), true, percent == shown);
    return items;
}

void UiScaleMenu::pick (int percent)
{
    session.pickUiScale (percent);
    shown = session.uiScalePercent();
    if (onPicked != nullptr)
        onPicked (percent);
}

void UiScaleMenu::tick (double) { shown = session.uiScalePercent(); }

std::unique_ptr<juce::AccessibilityHandler> UiScaleMenu::createAccessibilityHandler()
{
    return accessibility::handler (*this, juce::AccessibilityRole::button, [this] { return valueText::percent (shown); }, [this] { triggerClick(); });
}

} // namespace plugkit::ui
