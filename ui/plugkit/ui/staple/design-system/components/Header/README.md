# Header

The top bar shared by every plugin in the family. It holds the plugin's name, the Preset, and edit history.

It is 44–48 px tall, with three zones:
- **Left:** the wordmark (`wordmark` style), optionally followed by a short tag in `caption`/`text-3` ("COMP").
- **Centre:** the Loaded Preset name in `body`, centred in the window, with ‹ › to step presets and a Modified dot when it applies. Click it to open the preset browser.
- **Right:** undo and redo icon buttons, then **A/B** (one text button, the active letter lit; click to switch sides) and **Copy** (copies the active side to the other; its tooltip says which way).

The logic behind it is plugkit-ui's Session (Presets, A/B Compare, EditHistory), shared by eq1 and comp1.
