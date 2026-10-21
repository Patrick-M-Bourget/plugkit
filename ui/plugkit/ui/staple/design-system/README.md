# Staple

Staple is the shared interface system for the plugin family: **eq1** (the EQ) and **comp1** (the compressor). It exists so a producer who knows one plugin already knows the other. Same ground, same knobs, same header and footer, same words for the same things.

This folder is the system's written form, beside `Tokens.h`, which is its shipping source. A browsable copy with live previews is the "Staple design system" artifact (https://claude.ai/artifact/AApLC9jminUtzVHNofpF3y); when they differ, this folder and `Tokens.h` win.

Staple was designed for eq1 (`eq1/docs/staple-handoff`: `tokens.json`, `tokens.css`, `HANDOFF.md`, the prototype) and is built into the shared library as **plugkit-ui** (`ui/plugkit/ui/staple/Tokens.h`). Every value here is copied from those files. **The C++ in plugkit-ui is the shipping source of truth.** A change to Staple is made there first, and both plugins pick it up from their pinned plugkit tag.

## Principles

1. **One family, one feel.** A control that exists in both plugins looks and behaves the same in both: Global Bypass, Processing Mode, Presets, A/B, undo, UI Scale, Output Gain, meters, knobs. A plugin may add controls; it does not restyle shared ones.
2. **The signal is the picture.** The display is the largest thing in the window, edge to edge, its grid and fills dissolving into the ground. Controls float over it on `raised` slabs instead of sitting in a separate strip.
3. **Quiet chrome, loud signal.** Chrome is neutral (`bg-0`, `fill-*`, `text-*`, `line-*`). Colour is spent on what the audio is doing: `curve-main` amber for the main trace and every value arc, `dyn-range` red for reduction, `an-sc` teal for the sidechain and detection.
4. **Values on demand.** Knobs carry a label, not a read-out. The value appears in a tooltip on hover or drag, and a double-click on the tooltip types one in.
5. **Numbers never jitter.** Every figure uses tabular figures.

## Content fundamentals

- **Vocabulary comes from each plugin's GLOSSARY.md** and stays the same across the family: *Output Gain*, *Global Bypass*, *Processing Mode* (*Zero Latency*, *High Quality*), *Preset*, *A/B Compare*, *Stereo Placement* (*Stereo*, *Mid*, *Side*, *Left*, *Right*), *Gain Reduction*, *Sidechain*. Never "SC", "key", "makeup" or "listen".
- **Labels are nouns in Title Case** ("Threshold", "Auto Release"); section titles are uppercase with letter-spacing ("COMPRESSOR"). Toggle prefixes such as AUTO light up in `curve-main` when on and sit in `text-4` when off.
- **Units follow the value with a thin space:** `−18.0 dB`, `120 Hz`, `4:1`, `100 %`. Use the true minus sign (−), and ∞ for infinity ("∞:1").
- **Tooltips name the thing and its owner** on line one ("Band 4 Gain", "Gate Threshold"), and the value on line two.

## Visual foundations

**Theme.** Staple is dark only. The window is `bg-0` with three soft neutral radial highlights (`window-highlight-1…3`), no hue, and `space-window` (14 px) padding.

**Surfaces.** From flat to floating: `surface-1` → `fill-1` → `fill-2` → `fill-3` for controls; `raised` for slabs over the display; `menu` (opaque) for menus, popovers and tooltips; `dialog` over a `scrim` for modals.

**Text.** `text-1` for values and primary text, `text-2` for labels, `text-3` for quiet text and idle icons, and `text-4` for disabled states and decoration only. `text-4` fails contrast by design: never use it for text a user must read.

**Lines.** `line-1` hairlines inside groups, `line-2` for menu and popover edges, `line-3` for a stronger edge. Section titles sit over a 1 px divider in `line-1`.

**Radii.** `r-1` (4 px) for chips and icon buttons, `r-2` (6 px) for buttons and tooltips, `r-3` (10 px) for panels, menus and the window.

**Elevation.** `shadow-1` for menus, popovers, tooltips and raised panels; `shadow-2` for modals. Knobs carry their own soft drop shadow, with no outer black ring.

**Motion.** 120 ms (hover, press), 180 ms (popover pop-in: 3 px translate and 0.985 → 1 scale), 240 ms (sections sliding in), all on `cubic-bezier(.2,.7,.2,1)`. Animate translate and scale, never `transform`.

**Type.** One family, **Manrope** (Google Fonts, 400–700), tabular figures everywhere. Sizes are fs-1…fs-5 (10, 11, 12, 13, 16 px), plus the 19 px wordmark.

**The display.**
- Grid: minor lines at `grid-minor`, major at `grid-major`, the 0 dB line at `grid-zero`.
- Edges dissolve over 18 px at the top, 84 px at the bottom, 36 px on the left and 56 px on the right. Handles and panels are not faded.
- Analyzer: a fill from `an-fill-top` through `an-fill-mid` to nothing, a 1 px `an-line`, Peak Hold in `an-peak`, and the Sidechain in `an-sc-line`.
- Main curve: 2 px `curve-main` over a 4 px `curve-main-halo`.

**Knobs** (`knob-large` 66 px, `knob-medium` 50 px, `knob-small` 30 px).
- **Face:** a radial gradient from `knob-face-top` through `knob-face-mid` to `knob-face-edge`, lit from the top, with a `knob-rim` rim (2 px, or 1.5 px at 30 px).
- **Value arc:** 3 px (2 px below 40 px), radius r − max(4.5, d × 0.1), sweeping 270°. Bipolar knobs start at 12 o'clock. **No background track.** The arc is `curve-main`, or the Band colour in eq1's Band panel.
- **Ring:** an optional 12 px lane in `knob-ring-lane`, 10 px outside the knob.
  - eq1 fills it with Dynamic Range: a `dyn-range` gradient, with live reduction in `dyn-live`.
  - comp1 fills it with the Detection Level on Threshold, in `an-sc`, and with Gain Reduction on Ceiling, in `dyn-range`.
- **Handling:** drag vertically, 200 px for the full range (800 px with Shift). Arrow keys step 1 %. Double-click resets to the default.

**Meters.**
- **Output Meter:** two bars on `meter-track`, graded from `meter-1` through `meter-2` to `meter-3`, with a Clip Light above them (`meter-clip` lit, `meter-clip-off` unlit).
- **Gain Reduction meter:** one bar hanging from the top in a gradient from `dyn-range` to `dyn-range-inner`.

**State.** An off or bypassed control turns its power icon `state-off` on a `state-off-bg` fill. When on, the icon sits neutral in `text-3`.

## Iconography

16 px grid, 1.5 px strokes, round caps and joins, `currentColor`, no fills. The set is shared across the family: power (bypass), headphones (solo or audition), chevrons (‹ › stepping, ⌄ menus), close ×, undo and redo arrows, and the expand/collapse arrows for view and window size. An icon-only button always carries an `aria-label`.

## The window, in both plugins

- **Header:** the wordmark on the left; the preset name centred, stepped with ‹ ›, with a Modified dot when it applies; undo, redo, **A/B** (the active letter lit) and **Copy** on the right.
- **Display:** the full width, with the meters at the right edge.
- **Panels:** controls float over the lower part of the display on `raised` slabs.
- **Footer:** Global Bypass (the power icon) far left, then **Processing**, then plugin-specific chips; on the right, the Output Gain value (opens its popover) and the window-size or view switch.

## Per-plugin vocabulary on the shared system

| Need | eq1 | comp1 |
| --- | --- | --- |
| Main trace | summed EQ curve, `curve-main` | Output level trace, `curve-main`; Input level, `an-line` |
| Reduction | Dynamic Range ring, `dyn-range`/`dyn-live` | Gain Reduction graph, meter and Ceiling ring, `dyn-range` |
| Sidechain | Sidechain spectrum, `an-sc-line` | Detection Level ring on Threshold, Detection EQ spectrum, `an-sc` |
| Per-item colour | Band colours `band-01…24` | Detection EQ band handles reuse `band-01…03` |
| Primary knob | Gain, `knob-large` | Threshold and Comp, `knob-large` |

comp1 adds **no new colours**. Every comp1 element maps onto an existing Staple token.

## Accessibility

- Every control is a real button or input, keyboard-reachable, with a visible 2 px `focus` ring at a 2 px offset.
- Text meets 4.5:1 on its ground: `text-1`, `text-2` and `text-3` pass on `bg-0` and `raised`, and `text-4` never carries information.
- Colours that must be told apart also differ in lightness. For example, `dyn-range` red against `curve-main` amber.
- Screen-reader names follow the tooltip text ("Band 4 Gain, −11.4 dB").

## Window scaling

The window is resizable, with eq1's UI Scale choices. Everything is drawn from these tokens at 100 % and scaled, so sizes here are 100 % values.

## Not synced

- eq1's 24 `bandBypassed` colours: they're derived per Band in code (`oklch(0.72 0.045 h)`), so they're not listed as tokens.
- Motion durations and easing: described above, not tokens; this format has no motion family.
- The knob tooltip, Band panel bell shape and edge selectors are described in eq1's `HANDOFF.md` §4. Only the tooltip has a card here.
- The component cards are **static renditions, hand-written** from eq1's prototype (`docs/staple-handoff/prototype/Main.dc.html`) and `HANDOFF.md`. The shipping components are C++ in plugkit-ui, so there's no JavaScript bundle.
