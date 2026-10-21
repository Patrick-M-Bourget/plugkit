#pragma once

// Copied from eq1's plugin/Accessibility.h (eq1 at dbb1127), in namespace plugkit::ui::accessibility; listed in
// eq1#231.

#include "staple/Accessibility.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace plugkit::ui::accessibility
{

// A parameter's value as a screen reader says it, as the editor writes it (ValueText.h): "1.00 kHz",
// "+3.50 dB" and "−inf dB" (dB to the parameter's own decimals), "12 dB/oct", "100%", or the parameter's
// text when it has no such unit ("0.707"). Words such as "Auto" stay words. The parameter's own text,
// which hosts show, is unchanged.
juce::String spokenValue (const juce::RangedAudioParameter& parameter, float normalisedValue);

// The kit's handler (staple/Accessibility.h), for a plugin's own components.
using staple::accessibility::handler;

} // namespace plugkit::ui::accessibility
