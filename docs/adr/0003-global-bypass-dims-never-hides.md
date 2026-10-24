# Global Bypass dims, never hides

Under Global Bypass, a display keeps drawing what the processing would do, dimmed so it reads as not applied. What shows the plugin's work (a curve, an output's line and fill, a Gain Reduction shading) keeps `staple::tokens::motion::globalBypassScale` (0.45) of its usual alphas, and glides there and back over `motion::globalBypassFadeMs` (150 ms) as Global Bypass switches. What shows the signal itself, such as an input's line, stays as it is. An editor opened while Global Bypass is on starts dimmed, with no fade. The tokens live in `ui/plugkit/ui/staple/Tokens.h`, and `tests/ui/StapleTest.cpp` pins the scale.

Global Bypass changes what the plugin outputs, and nothing upstream of that. The Engine keeps processing underneath and crossfades its output to the input, the taps report what the processing does underneath, and the analyzers read them as without Global Bypass. So the display can show what turning Global Bypass off would do, and turning it off is a crossfade from audio that was already running.

The plugins implement it on their own displays: comp1's `plugin/GlobalBypassFade.h` (which also starts an editor opened under Global Bypass at 1) and its Spectrum display, and eq1's `plugin/display/CurvesLayer.cpp`.

## Consequences

- **A new display follows it.** A display a plugin adds dims its work by `globalBypassScale` over `globalBypassFadeMs`, and doesn't hide it or pick a scale of its own.
- **The Output readout dims by colour.** The footer's Output readout doesn't fade: it draws in text-4 while Global Bypass leaves it unapplied (`plugkit::ui::OutputReadout::setNotApplied`).
- **Repaints while the fade moves.** A display that otherwise holds still with no audio (comp1's Spectrum display) still repaints while its fade is on its way.

## Considered Options

- **Hiding the plugin's work under Global Bypass.** Rejected: the user loses sight of what the settings would do just when comparing with and without them.
- **Stopping the Engine, or its taps, under Global Bypass.** Rejected: the display would have nothing to show, and switching back would start the processing from cold rather than crossfade.
