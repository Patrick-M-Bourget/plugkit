#pragma once

// What the Output Meter reads: the four calls eq1's OutputMeter (plugin/OutputMeter.cpp, eq1 at dbb1127) makes on
// eq1's processor, as an interface a plugin's processor implements.

#include "plugkit/OutputLevel.h"

namespace plugkit::ui
{

// The Output Level of each output channel, and its Clip Light. A plugin's processor implements it from its Engine's
// Output Level and a ClipLights (ClipLights.h), and the Output Meter reads it on the message thread.
class OutputLevelSource
{
public:
    virtual ~OutputLevelSource() = default;

    // How many channels have an Output Level: 1 on mono, 2 on stereo.
    virtual int outputLevelChannels() const = 0;
    // The Output Level of channel (0 to outputLevelChannels() - 1): the peak since the last read, which this read
    // resets, and the RMS now. A peak above 0 dBFS lights the channel's Clip Light.
    virtual OutputLevel readOutputLevel (int channel) = 0;
    // Whether channel's Clip Light is lit.
    virtual bool isClipLit (int channel) const = 0;
    // Puts out every Clip Light.
    virtual void clearClipLights() = 0;
};

} // namespace plugkit::ui
