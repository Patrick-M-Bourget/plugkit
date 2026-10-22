# Popover

Menus, popovers and option cards that open from a control. Examples: the Analyzer and Output popovers in eq1's footer, and comp1's Character popover.

The fill is opaque `menu`, with a 1 px `line-2` edge, `r-3` corners and `shadow-1`. It pops in over 180 ms (a 3 px translate and 0.985 → 1 scale). It sits `space-popover-below` (6 px) below its opener, or `space-popover-above` (10 px) above it for footer popovers. Menu items are `menu-item` (30 px) tall with `space-menu-pad` padding, and get a `fill-1` fill on hover. A popover with a title uses the `section` style over a 1 px `line-1` divider. It closes on an outside click or Esc. Uses `role=menu`, `listbox` or `dialog`.

## Popover slider

A popover's horizontal slider (plugkit-ui's `PopoverSlider`), the popover's full width: a 6 px `fill-2` track with 3 px rounded ends and a 6 x 12 px `text-1` thumb with a soft shadow, filled in `text-2` either from the centre, past a 1 px `line-3` centre tick (eq1's Output Pan), or from the left end (a level, as comp1's Dry and Wet). Beneath it, 4 px down, is its readout line in `fs-1` `text-3`: a text at the left, one centred and one at the right (eq1's `L`, `40 L` and `R`; a level's name and its value, `Off` at the bottom). Disabled, it dims to 35 % with its readout. A click on the track or a drag sets it, a double-click resets it, and the arrow keys step it.
