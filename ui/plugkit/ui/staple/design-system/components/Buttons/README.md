# Buttons

The family's small controls: icon buttons, text buttons, toggle chips, the power button and segmented choices.

- **Icon button.** 24 px in panels and 28–30 px in the header and footer. Transparent, with the icon in `text-3`; it brightens to `text-1` on hover. 16 px icons with 1.5 px strokes. Always carries an `aria-label`.
- **Text button.** 28 px tall, `r-2`, `body` text. **Footer chips** use a `fill-1` fill (Processing: "Zero Latency"). **Header text buttons** (A/B, Copy) are transparent and show a `fill-2` box only on hover. In **A/B**, the active letter is `text-1` and the other is `text-4`.
- **Toggle chip.** 20–24 px tall, `r-1`, `label` or 11 px at 600. Off: a `fill-1` fill and `text-2`. On: `curve-main` text on a 14 % `curve-main` fill. Uses `aria-pressed`. Examples: comp1's Comp Follow, Saturation, Limiter.
- **Power button.** The family's on/off and bypass control. On: neutral, icon in `text-3`. Off: icon in `state-off` on a `state-off-bg` fill. Used for Global Bypass, Band bypass, Dynamics Bypass and comp1's sections.
- **Segmented control.** Two to eight choices in a `meter-track` well with 2 px padding; the chosen one gets a `fill-2` fill and `text-1`. Uses `role=radiogroup`. Examples: Before/After, Stereo/Mid/Side, comp1's Style list in the Compact View.

Hover lifts filled controls with `brightness(1.18)`; press goes to 1.3. Focus shows the 2 px `focus` ring.
