# Popover

Menus, popovers and option cards that open from a control. Examples: the Analyzer and Output popovers in eq1's footer, and comp1's Character popover.

The fill is opaque `menu`, with a 1 px `line-2` edge, `r-3` corners and `shadow-1`. It pops in over 180 ms (a 3 px translate and 0.985 → 1 scale). It sits `space-popover-below` (6 px) below its opener, or `space-popover-above` (10 px) above it for footer popovers. Menu items are `menu-item` (30 px) tall with `space-menu-pad` padding, and get a `fill-1` fill on hover. A popover with a title uses the `section` style over a 1 px `line-1` divider. It closes on an outside click or Esc. Uses `role=menu`, `listbox` or `dialog`.
