# Tooltip

The value read-out of a knob or handle, shown on hover or drag. It's the only place a knob's value appears.

The fill is `menu`, with `r-2` corners, `shadow-1` and a 1 px `line-2` edge. It is at least 86 px wide, with 5/10/6 px padding, and is centred under the knob, 6 px below it (18 px when the knob has a ring). It has two lines: the **name with its owner** ("Band 4 Gain", "Gate Threshold") in the `label` style and `text-3`, then the **value** in `body` and `text-1`, with tabular figures. While the pointer is on a ring, it names the ring ("Dynamic Range", "Detection Level"). Double-click the tooltip to type a value: Enter commits, Esc cancels. It fades in over 120 ms.
