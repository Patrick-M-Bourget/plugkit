#pragma once

// Copied from eq1's plugin/AssumedSampleRate.h (eq1 at dbb1127), in namespace plugkit::ui.

#include <juce_audio_processors/juce_audio_processors.h>

namespace plugkit::ui
{

// The sample rate the editor assumes before the host has given one (the processor's is 0 until then).
inline constexpr double assumedSampleRate = 48000.0;

// The processor's sample rate, or assumedSampleRate before it has one.
inline double sampleRateOrAssumed (const juce::AudioProcessor& processor)
{
    return processor.getSampleRate() > 0.0 ? processor.getSampleRate() : assumedSampleRate;
}

} // namespace plugkit::ui
