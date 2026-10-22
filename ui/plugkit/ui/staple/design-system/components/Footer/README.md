# Footer

The bottom bar shared by every plugin, holding the global settings.

It is 36–40 px tall.
- **Left:** **Global Bypass** (the power button), then **Processing** (the Processing Mode label in `text-2` beside its value chip), then the plugin's own chips: eq1's **Analyzer ▾**; comp1's **Oversampling** and, in the Expanded View, **Detection** (with a Stereo Placement dot).
- **Right:** the plugin's output controls. eq1 has the Gain Scale (100 %) and the **Output readout**, which opens the Output popover. comp1 has a Limiter chip showing its Ceiling, and the Output readout.
- **Far right:** the window-size or view switch: `Icon::expand` to grow the window or open the larger view, `Icon::collapse` to go back.

The chips are text buttons on `fill-1`, 28 px tall. A chip that opens a popover shows ▾ and opens the popover above, `space-popover-above` away.

## Output readout

The **Output readout** is shared by every plugin (plugkit-ui's `OutputReadout`). It shows Output Gain, plus Auto Gain's estimate while Auto Gain is on, as a signed level to one decimal (`+0.0 dB`), or `−inf dB` at Output Gain's silent bottom. It is a plain text button in `text-1`, with `fill-1` behind it on hover. It is drawn in `text-4` while Global Bypass leaves it unapplied, and on `fill-2` while its popover is open.

A click opens the **Output popover** above it, `space-popover-above` away, its right edge on the readout's. A second click, Escape or a click outside closes it. The popover's frame is shared (`OutputPopoverFrame`); its contents are each plugin's own. Its toggles (`OutputToggle`) show an icon, or a letter (Auto Gain's "A"), on `fill-2` in `text-1` while on, else in `text-3`.
