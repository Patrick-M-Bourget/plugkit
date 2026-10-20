#include "Placeholder.h"

#include <numeric>

namespace plugkit::placeholder
{
float sum (std::span<const float> samples)
{
    return std::accumulate (samples.begin(), samples.end(), 0.0f);
}
} // namespace plugkit::placeholder
