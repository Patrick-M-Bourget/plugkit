# plugkit

The shared library for the [eq1](https://github.com/Patrick-M-Bourget/eq1) and [comp1](https://github.com/Patrick-M-Bourget/comp1) audio plugins. It has two modules, mirroring eq1's own split:

- **`plugkit-dsp`:** plain C++ with no JUCE. It holds the Smoother and the 16-sample run grid, the matched filter designs, and Placement.
- **`plugkit-ui`:** the JUCE layer. It holds the UI kit and the Plugin Shell's Presets, A/B Compare and edit history.

The CI scripts and the check harness live here too.

## Rules

- **Copied from eq1 until eq1 ships.** eq1 doesn't change before then, so everything here is a copy of its code. Each copied piece, and every eq1 fix ported by hand, is listed in [eq1#231](https://github.com/Patrick-M-Bourget/eq1/issues/231), the ticket that later migrates eq1 onto plugkit. See comp1's ADR 0006.
- **Pinned per plugin.** Each plugin pins an exact plugkit tag or commit through FetchContent, never a branch. Bumping the pin is that plugin's own PR, through its full checks. See comp1's ADR 0007.
- **Grown in slices**, each just before comp1 needs it:
  1. CI and the check harness ([comp1#2](https://github.com/Patrick-M-Bourget/comp1/issues/2)).
  2. `plugkit-dsp` ([comp1#5](https://github.com/Patrick-M-Bourget/comp1/issues/5)).
  3. `plugkit-ui` ([comp1#22](https://github.com/Patrick-M-Bourget/comp1/issues/22)).
