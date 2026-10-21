# Meters

The Output Meter with its Clip Light, and the Gain Reduction meter. They're shared so levels read the same in every plugin.

**Output Meter.** Two vertical bars (L, R), each about 4–5 px wide, on `meter-track`. Each bar fills from the bottom with a gradient: `meter-1` at the bottom, `meter-2` at 70 %, `meter-3` at the top. Above them sits the **Clip Light**, `meter-clip-off` unlit and `meter-clip` lit (click to clear). A peak read-out in `caption` sits under the bars. It lives at the right edge of the display in both plugins.

**Gain Reduction meter** (comp1). One bar hanging from the top, in a gradient from `dyn-range` to `dyn-range-inner`, with its read-out in `caption`. It sits beside the Output Meter. comp1's Gate and Limiter report their own Gain Reduction to the same meter style.

**Scale.** dB marks in `caption`, `text-3`, right-aligned to the left of the bars.

**The consumer supplies** levels in dBFS from the level taps and the clip state. Ballistics come from plugkit-ui's meter code.
