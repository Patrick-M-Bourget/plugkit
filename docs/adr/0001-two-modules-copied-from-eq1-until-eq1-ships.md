# Two modules, copied from eq1 until eq1 ships

The plugins share eq1's infrastructure through plugkit, a library in its own repo, so no plugin holds a private copy of another's code. plugkit mirrors eq1's own split. `plugkit-dsp` is plain C++ with no JUCE: the Smoother and the 16-sample run grid, the matched filter designs, Placement. `plugkit-ui` is the JUCE layer the plugins share: the UI kit (Staple), the Analyzer and the display's generic layers, the meter ballistics and the Output Meter, and the Plugin Shell's session state (Presets, A/B Compare, edit history and UI Scale). The editor test harness, the CI scripts and the check harness come with it. README.md lists what each module holds.

eq1 does not change until it ships. Until then, everything in plugkit is either a copy of eq1's code or new code eq1 takes when it migrates. A copied file names its eq1 source in a "Copied from eq1's <path> (eq1 at dbb1127)" header, a new one says "New in plugkit, for eq1 to take", and each one, with every eq1 fix ported here by hand, is listed in [eq1#231](https://github.com/Patrick-M-Bourget/eq1/issues/231). After eq1 ships, it migrates onto plugkit in that one gated ticket, and its null check must stay `exact`.

## Consequences

- **Fixes go both ways by hand.** Until eq1 migrates, a bug fixed in eq1 must also be fixed in plugkit, and a fix made here that eq1's copy also needs is listed for the migration. eq1#231's list is how that drift is tracked.
- **EqDisplay stays in eq1.** It and its layers (curves, handles, ghost) carry eq1's Band model and its response maths, which no other plugin uses. `plugkit-ui` holds the generic layers instead (the display geometry, the grid, the Analyzer and the edge fades), and each plugin builds its own graphs on them.
- **JUCE is the plugin's.** Each plugin pins JUCE, and `plugkit-ui` links the JUCE its consumer has already fetched (README.md, "Linking plugkit-ui").

## Considered Options

- **Wrapping eq1's components behind an interface instead of copying them.** Rejected: impossible without changing eq1 before it ships.
- **Each plugin vendoring its own copy until eq1 ships.** Rejected: two private copies drift with nothing tracking them, and the later move to a library is two migrations instead of one.
