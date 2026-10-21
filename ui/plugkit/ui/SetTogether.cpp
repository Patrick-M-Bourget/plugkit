// Copied from eq1's plugin/Parameters.cpp (eq1 at dbb1127): setTogether alone, in namespace plugkit::ui rather than
// eq1::parameters.

#include "SetTogether.h"

namespace plugkit::ui
{

void setTogether (const std::vector<NewValue>& values)
{
    for (const auto& [parameter, value] : values)
        parameter->beginChangeGesture();
    for (const auto& [parameter, value] : values)
        parameter->setValueNotifyingHost (value);
    for (const auto& [parameter, value] : values)
        parameter->endChangeGesture();
}

} // namespace plugkit::ui
