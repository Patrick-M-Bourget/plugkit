# Knob

The rotary control for every continuous parameter in the family, drawn the same way in eq1 and comp1.

**When to use.** Any continuous parameter: Frequency, Gain and Q in eq1; Threshold, Ratio, Attack, Release, Colour, Dry, Wet and the Gate's controls in comp1. Use a toggle chip for on/off, and a segmented control for a short list of choices.

**Anatomy.**
- **Face:** a radial gradient (`knob-face-top` → `knob-face-mid` → `knob-face-edge`, centred at 50% 12%) inside a `knob-rim` rim, with a soft drop shadow (0 6px 14px black at 40%). There is no outer black ring.
- **Value arc:** inside the face, at radius r − max(4.5, d × 0.1). It is 3 px wide, or 2 px below 40 px, sweeps 270° (−135° to +135°) and has round caps. It is drawn in `curve-main`, or the Band colour, with a 1.5 px glow at 30 %. **No background track:** only the active arc is drawn. A bipolar knob (Gain, Output Gain) starts at 12 o'clock.
- **Ring** (optional): a 12 px lane in `knob-ring-lane`, `space-ring-offset` (10 px) outside the knob. It is filled with a red range gradient in eq1 (Dynamic Range), with the teal Detection Level on comp1's Threshold, or with Gain Reduction on comp1's Ceiling. A ▲▼ grab hint in `ring-hint` sits at its end.
- **Label:** below the knob, in the `control` style and `text-2`. Labels sit on one baseline even when knob sizes differ. There's no value read-out.

**Sizes.** `knob-large` (66 px) for the primary control, `knob-medium` (50 px) for secondary controls, `knob-small` (30 px) for tertiary ones. A knob of 30 px or less takes a 1.5 px rim. `knob-hero` (120 px) is for a view's one-knob control, centred, with smaller controls beside it, comp1's Compact View Comp. Above 66 px, the rim, arc and glow widths grow in proportion to the diameter (× d / 66), so the hero knob's arc is about 5.5 px.

**Behaviour.**
- **Drag:** vertical, 200 px for the full range, 800 px with Shift.
- **Keys:** arrow keys step 1 %, or 0.2 % with Shift.
- **Reset:** double-click resets to the default.
- **Value:** hovering or dragging shows the Tooltip; double-click the tooltip to type a value.

**The consumer supplies** the label, the normalised value 0–1, whether it's bipolar, the arc colour, and an optional ring value and colour.
