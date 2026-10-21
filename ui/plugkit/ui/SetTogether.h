#pragma once

// Copied from eq1's plugin/Parameters.h (eq1 at dbb1127): NewValue and setTogether alone, in namespace plugkit::ui
// rather than eq1::parameters.

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace plugkit::ui
{

// A parameter's new normalised value.
struct NewValue
{
    juce::AudioProcessorParameter* parameter;
    float value;
};

// Sets several parameters as one edit the host sees: every gesture begins before the first value
// changes, and ends after the last.
void setTogether (const std::vector<NewValue>& values);

} // namespace plugkit::ui
