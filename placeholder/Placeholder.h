#pragma once

#include <span>

namespace plugkit::placeholder
{
// The sum of the samples: something to compile, link and test until plugkit-dsp replaces this target.
float sum (std::span<const float> samples);
} // namespace plugkit::placeholder
