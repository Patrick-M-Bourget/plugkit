#pragma once

// Copied from eq1's plugin/staple/Accessibility.h (eq1 at dbb1127); listed in eq1#231.

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace staple::accessibility
{

// An element with a role, its title (Component::getTitle), a read-only value and, when given, an
// action for a press.
std::unique_ptr<juce::AccessibilityHandler> handler (juce::Component& component,
                                                     juce::AccessibilityRole role,
                                                     std::function<juce::String()> value,
                                                     std::function<void()> press = nullptr);

} // namespace staple::accessibility
