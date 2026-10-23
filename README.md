# plugkit

The shared library for the [eq1](https://github.com/Patrick-M-Bourget/eq1) and [comp1](https://github.com/Patrick-M-Bourget/comp1) audio plugins. It has two modules, mirroring eq1's own split, and two test-only targets:

- **`plugkit-dsp`:** plain C++ with no JUCE. It holds the Smoother and the 16-sample run grid, the matched filter designs, Placement, LatestValue, the lock-free hand-off of the newest settings from the UI thread to the audio thread, and OutputLevel, a channel's peak and RMS in dBFS for the Output Meter, with OutputLevelTap, which measures it on the audio thread for one reader thread.
- **`plugkit::test_support`:** for a plugin's test executables only, so `plugkit-dsp` stays free of it. It holds AllocationGuard (`plugkit/test/AllocationGuard.h`), which counts the heap allocations its thread makes while it is alive, and the bytes they hold, for realtime-safety and footprint tests. Linking it, `target_link_libraries(<name>_engine_tests PRIVATE plugkit::test_support)`, replaces operator new and delete in that executable.
- **`plugkit-ui`** (`plugkit::ui`): the JUCE layer. It holds Staple, eq1's UI kit (namespace `staple`: tokens, the Manrope font, icons, the LookAndFeel and the controls), and in namespace `plugkit::ui`:
  - KeyboardSlider, KeyboardControl (the editor-wide keys), `accessibility::spokenValue`, ValueText, `showMenu` and EditorTick, the editor's one tick.
  - **The Analyzer:** AnalyzerSpectrum (FFT, smoothing, Peak Hold, Tilt), and in `plugkit::ui::display` the display geometry, the Frequency grid and its labels, any grid's lines (`paintGrid (g, lines, size)`), the edge fades and the Analyzer layer. Each display passes its own Frequency range, narrow width and fade distances.
  - **The Output Meter:** OutputMeter, ClipLights and the meter ballistics. The meter reads an `OutputLevelSource`, which the plugin's processor implements: `readOutputLevel` returns its Engine's OutputLevel through a `ClipLights` member's `note`, which lights a channel's Clip Light on a peak above 0 dBFS, and `isClipLit` and `clearClipLights` ask that member. The processor clears the Clip Lights when it restores a session.
  - **The session state:** `Session`, the Plugin Shell's undo history (EditHistory), A/B Compare, Preset Library and window size and UI Scale, with the rules for saving and restoring them, and the widgets that drive it (PresetBar and its PresetBrowser, CompareGroup's Undo, Redo, A/B Compare and Copy, and UiScaleMenu). See "Session state".
  - **The editor shell:** `EditorShell`, the base of a plugin's editor, and `HeaderBar`, the header it holds: the plugin's wordmark, PresetBar and CompareGroup. See "The editor shell".
  - **The footer's Output readout:** `OutputReadout`, the footer's Output Gain value (plus Auto Gain's estimate while Auto Gain is on), with `outputReadoutText` for its text; `OutputPopoverFrame`, the base of the output popover it opens; `OutputToggle`, that popover's toggle; and `PopoverSlider`, a popover's horizontal slider with its readout line. See "The footer's Output readout".

  See "Linking plugkit-ui".
- **`plugkit::editor_harness`:** eq1's editor test harness (`plugkit/test/EditorHarness.h`, namespace `harness`), for a plugin's editor tests. See "Linking plugkit-ui".

The CI scripts and the check harness live here too.

## Rules

- **Copied from eq1 until eq1 ships.** eq1 doesn't change before then, so everything here is a copy of its code. Each copied file names its eq1 source in a "Copied from eq1's <path> (eq1 at dbb1127)" header, and every eq1 fix ported by hand says so in its commit. [eq1#231](https://github.com/Patrick-M-Bourget/eq1/issues/231), the ticket that later migrates eq1 onto plugkit, builds its table from those. See comp1's ADR 0006.
- **Pinned per plugin.** Each plugin pins an exact plugkit tag or commit through FetchContent, never a branch. Bumping the pin is that plugin's own PR, through its full checks. See comp1's ADR 0007.
- **Grown in slices**, each just before comp1 needs it:
  1. CI and the check harness ([comp1#2](https://github.com/Patrick-M-Bourget/comp1/issues/2)).
  2. `plugkit-dsp` ([comp1#5](https://github.com/Patrick-M-Bourget/comp1/issues/5)).
  3. `plugkit-ui`: its foundation ([comp1#22](https://github.com/Patrick-M-Bourget/comp1/issues/22)), the meters and display layers ([comp1#36](https://github.com/Patrick-M-Bourget/comp1/issues/36)), session state ([comp1#37](https://github.com/Patrick-M-Bourget/comp1/issues/37)), the editor shell and header ([comp1#75](https://github.com/Patrick-M-Bourget/comp1/issues/75)) the footer's Output readout ([comp1#103](https://github.com/Patrick-M-Bourget/comp1/issues/103)) and the popover slider ([comp1#113](https://github.com/Patrick-M-Bourget/comp1/issues/113)).

## Linking plugkit-ui

- **JUCE is the plugin's.** Each plugin pins JUCE, and fetches it before plugkit: `plugkit-ui` links the `juce::` targets that already exist, and never fetches JUCE itself when they do. It links `juce_gui_basics`, `juce_audio_processors` and `juce_dsp` (the Analyzer's FFT), which a plugin that fetched JUCE has, and `plugkit::dsp`. Fetched by a project without them (an Engine-only build), plugkit defines no `plugkit::ui` and says so. plugkit's own build fetches JUCE 9.0.3, the version eq1 and comp1 pin, to build and test it.
- **One target compiles it.** `plugkit::ui` is an interface library, as JUCE's modules are: its sources compile into the target that links it, with that target's JUCE settings. The plugin links it from its plugin target (`target_link_libraries(<name> PRIVATE plugkit::ui)`), and only there, or the executable that links that target gets plugkit-ui twice.
- **The editor harness:** a test executable links `plugkit::editor_harness`, which brings the harness and plugkit-ui's headers, not its sources, and defines `PLUGKIT_HARNESS_VAR_PREFIX` as the plugin's `VAR_PREFIX`, so `check.sh screens` reaches `harness::writeSnapshot` through `<prefix>SCREENS`:

  ```cmake
  target_link_libraries(<name>_plugin_tests PRIVATE <name> plugkit::editor_harness Catch2::Catch2WithMain)
  target_compile_definitions(<name>_plugin_tests PRIVATE PLUGKIT_HARNESS_VAR_PREFIX="COMP1_")
  ```

  `harness::OpenEditor<Processor>` opens the plugin's editor in a window of its own; a plugin's subclass of it adds helpers for its own displays. The editor must be an `EditorTick::Host`. The harness lives in plugkit's checkout, not the plugin's tests, so a plugin's `plugkit.conf` leaves `EDITOR_HARNESS` out.
- **Tokens:** Staple's tokens are the kit's (`plugkit/ui/staple/Tokens.h`), documented for every plugin in `ui/plugkit/ui/staple/design-system/` (the brand book, `tokens.json` and the component guidelines). A plugin's own colours and sizes live in its own tokens header, which its `TOKENS_DIR` names.

## Session state

A plugin's processor owns one `plugkit::ui::Session`, built after its parameters from the processor, its `AudioProcessorValueTreeState` and a `Session::Config`:

- **`isPresetSetting`:** the parameters a Preset and each A/B side hold; by default every one but `global_bypass`.
- **`productName`:** names the User Preset folder (`Documents/<name>/Presets`) and the per-user settings' folder and lock.
- **`presetFileExtension`** and **`presetType`:** a Preset file's extension, and its root type, the saved state's type.
- **`factoryPresets`:** name and data pairs, from the plugin's own `juce_add_binary_data`.
- **`stateVersion`** and **`migrate`:** the saved format's version and the plugin's migration to it. A plugin adding A/B Compare appends `ABCompare::initialState()` in that version's step.
- **`window`:** the window's default logical size and its limits.
- **`presetFolder`** and **`userSettingsFile`:** where User Presets and the per-user settings live; a shipped plugin passes `UserSettings::defaultFile (productName)`, and tests leave both empty or point them at a temporary folder.

The processor saves and restores through it:

```cpp
void getStateInformation (juce::MemoryBlock& destData) override
{
    auto state = session.saved(); // the parameters, A/B Compare, the version, window size and UI Scale
    state.setProperty ("zoom", zoom, nullptr); // and the plugin's own view
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void setStateInformation (const void* data, int size) override
{
    clearTransientState(); // the plugin's own, such as its Clip Lights or Solo
    if (auto xml = getXmlFromBinary (data, size))
        session.restore (juce::ValueTree::fromXml (*xml), [this] (juce::ValueTree& state) {
            zoom = state.getProperty ("zoom", 1); // reads the plugin's view, and takes it off
            state.removeProperty ("zoom", nullptr);
        });
}
```

`restore` runs from any thread: it migrates, reads the window's size and UI Scale, calls the view reader, restores A/B Compare, restores every parameter (a missing one at its default, a non-finite value at its default unless it is an infinity the range ends at, an out-of-range one at the nearer end, and unknown content kept and saved again as it was), and empties the undo history. The editor gives its widgets the session, and its editor-wide keys its `editHistory()`.

## The editor shell

A plugin's editor derives from `plugkit::ui::EditorShell`, which lays out the Staple window:

- **The header** (`HeaderBar`), 52 px: the wordmark on the left, PresetBar centred in the window, and CompareGroup on the right. The Preset browser opens over everything.
- **The plugin's body** between the header and the footer, which takes whatever the window grows by, and **its footer**, 44 px, along the bottom. The window has 14 px padding and 12 px gaps (`staple::tokens::layout`).
- **The window:** resizable within the Session's `window` limits, sized and drawn at the UI Scale through one transform on the content (the overlay layer for popovers), with `staple::paintWindowBackground` under everything. It follows a restored session's size and UI Scale on its tick (EditorTick), which it owns.
- **The keys:** Cmd-Z undoes, Shift-Cmd-Z and Cmd-Y redo, Cmd-B toggles Global Bypass as one undo step (Ctrl on Windows), and with nothing focused Tab focuses the first control and Shift+Tab the last. KeyboardControl takes in every control.

The plugin passes the processor, its Session and an `EditorShell::Config`: `wordmark`, its name, and `globalBypassId`, the parameter Cmd-B toggles (`global_bypass` by default). It adds its components to `content()`, numbering their Tab order from `firstFocusOrder`, lays them out in `layOutBody`, and calls `open (footer)` last:

```cpp
class PluginEditor final : public plugkit::ui::EditorShell
{
public:
    explicit PluginEditor (PluginProcessor& p) : EditorShell (p, p.session(), { "comp1" }), footer (p)
    {
        content().addAndMakeVisible (display);
        display.setExplicitFocusOrder (firstFocusOrder);
        footer.uiScaleMenu().onPicked = [this] (int) { applyUiScale(); };
        open (footer); // along the bottom, last in Tab's order
    }

private:
    void layOutBody (juce::Rectangle<int> area) override { display.setBounds (area); }

    Display display;
    Footer footer;
};
```

The footer, the display and the meters are the plugin's own; the footer's Output readout is shared (below).

## The footer's Output readout

The footer's Output readout and the frame of the output popover it opens are shared; what the popover holds, and the footer's layout, are each plugin's own.

- **`OutputReadout`:** a plain `staple::TextChip` titled "Output", whose text a screen reader reads as its value. The plugin sets its text on its tick, `outputReadoutText (db, silentDb)`: Output Gain, plus Auto Gain's estimate while Auto Gain is on, signed to one decimal ("+0.0 dB", "−1.8 dB"), or "−inf dB" at or below Output Gain's silent bottom (eq1's −80 dB; a range without one, comp1's, leaves `silentDb` out). `setNotApplied (true)` draws it in text-4 under Global Bypass; `setOpen` draws fill-2 behind it, and no hover, while its popover is open.
- **`OutputPopoverFrame`:** a `staple::Popover` whose `openFrom (readout)` opens it above the readout, right-aligned to it and kept inside the window. It closes on Escape or a click outside it and the readout. The plugin's popover derives from it, adds its controls, sizes its card and lays them out, and gives `followProcessor`: what it shows that no parameter attachment does (eq1's Output Meter toggle, and whether the track is stereo). The frame calls it as the popover opens and on each editor tick while it is open; the plugin's constructor calls it once too.
- **`OutputToggle`:** one of the popover's toggles: an icon, or a letter (Auto Gain's "A"), on fill-2 in text-1 while on, else in text-3.
- **`PopoverSlider`:** a horizontal slider the popover's width, `PopoverSlider::height` (32 px) tall: a 6 px fill-2 track in a 14 px row, filled in text-2 from its centre past a centre tick (`Fill::fromCentre`, eq1's Output Pan) or from its left end (`Fill::fromLeft`, a level such as comp1's Dry and Wet), with a text-1 thumb, and beneath it a readout line in fs-1 text-3. The `readout` function gives the line's texts for a value, at its left, centred and at its right (eq1's "L", "40 L" and "R"); by default it shows the slider's title at the left and its value's text at the right. The slider travels as its range does, so a parameter's own mapping holds, an Off at −inf at its bottom included. A click on the track or a drag is one gesture (one undo step), landing on whole multiples of `setDragStep` if set (eq1's whole percent); its arrow steps, double-click return value and tooltip are the plugin's to set, as on any `KeyboardSlider`. The owner repaints it when something else its readout shows changes (eq1's Pan Mode).

The footer wires them, as eq1's does:

```cpp
output.onClick = [this] {
    if (outputPopover.isOpen())
        outputPopover.close();
    else
    {
        outputPopover.openFrom (output);
        output.setOpen (outputPopover.isOpen());
    }
};
outputPopover.onClose = [this] {
    output.setOpen (false);
    addChildComponent (outputPopover); // back in the footer, hidden, so it stays on the editor's tick
};
addAndMakeVisible (output);
addChildComponent (outputPopover);
```

## The check harness

`scripts/check.sh` runs a plugin's checks one stage at a time, as eq1's does: the same stages, checks and exit codes. Its header lists every stage. The git hooks in `.githooks/`, the Claude Code worktree hook in `.claude/hooks/`, and the helper scripts in `scripts/` come with it.

### How a plugin runs it

The plugin calls the script from **its own repository**, which stays the working directory. The script finds its helpers through its own path and reads the plugin's settings from `plugkit.conf` at the plugin's root:

```sh
cd <plugin repo>
<plugkit checkout>/scripts/check.sh docs        # or build, test, cpu, validate, null <ref>, ...
```

The plugin keeps no copy of the harness. It uses the plugkit checkout at its pinned tag, in one of two places:

- **Locally:** the copy FetchContent has fetched, `.deps/plugkit-src` (the harness's default `FETCHCONTENT_BASE_DIR`).
- **In CI, before anything is configured:** a clone of the same tag. CI runs `docs` before `build`, so the harness has to be there before CMake fetches anything.

Keep the tag in one place so the two copies can't drift. One way to do that is a `PLUGKIT_TAG=v0.1.0` line in `plugkit.conf`, read by both the CMake and the CI:

```cmake
file(STRINGS plugkit.conf plugkit_tag REGEX "^PLUGKIT_TAG=")
string(REPLACE "PLUGKIT_TAG=" "" plugkit_tag "${plugkit_tag}")
FetchContent_Declare(plugkit GIT_REPOSITORY https://github.com/Patrick-M-Bourget/plugkit.git GIT_TAG "${plugkit_tag}")
FetchContent_MakeAvailable(plugkit)
```

**The pin check:** every stage that configures or builds (`build`, `focus`, `screens`, `cpu`, `paint`, `null`, `size`, `tsan`, `asan`) first checks that the dependencies folder's `plugkit-src` is at the commit `PLUGKIT_TAG` names. A linked worktree shares the main checkout's `.deps`, which CMake doesn't fetch again when the worktree's pin moves on, so without the check the build fails later on whatever plugkit changed since.

- **On a mismatch:** the stage stops before configuring. It names the folder, the version it is at and the pinned one, and the two fixes: refresh that checkout (it prints the `git fetch` and `git checkout`), or set `FETCHCONTENT_BASE_DIR` to the worktree's own `.deps`.
- **In CI:** `.deps` is a cache, which a pin change can restore from an older pin. The stage deletes the stale `plugkit-src`, `plugkit-subbuild` and `plugkit-build` instead, and CMake fetches the pinned tag.
- **Not checked:** without `PLUGKIT_TAG` (plugkit's own repository), or when `plugkit-src` is missing (a first build) or isn't a git checkout.

### Per-plugin config

`plugkit.conf` is a bash file of `KEY=value` lines. The harness sources it, so quote values that hold spaces. `PLUGKIT_CONFIG=<path>` names a different file, relative to the plugin's root. A stage that needs a key the file leaves empty stops and names that key. "Needed by" names exactly those stages, and in parentheses the optional uses; `hooks` checks it against `check.sh` (`scripts/needed-by.py`). The values in the table are eq1's.

| Key | What it is | Example (eq1) | Needed by |
|---|---|---|---|
| `PLUGKIT_TAG` | The plugkit tag (or full commit hash) the plugin pins, which its CMake fetches and its CI clones. Optional: plugkit itself leaves it out | `v0.11.0` | (optional: the stages that configure or build check `.deps/plugkit-src` is at it) |
| `PLUGIN_NAME` | CMake target prefix: `<name>_engine_tests`, `<name>_plugin_tests`, `<name>_cpu_budget`, `<name>_render`, `<name>_sidechain_routing_check`, the plugin target `<name>` (artefacts in `<PLUGIN_DIR>/<name>_artefacts`), the Engine target `<name>_engine`; also the C++ namespace `size` attributes code to | `eq1` | every stage |
| `PRODUCT_NAME` | Bundle and binary name: `<product>.vst3`, `.component`, `.clap`, `.aaxplugin`, `.app`, `.exe` | `eq1` | `validate`, `size` |
| `VAR_PREFIX` | Prefix of the CMake options the harness sets (`<prefix>LTO`, `<prefix>STRIP`, `<prefix>BUILD_PLUGIN`, `<prefix>BUILD_TESTS`) and the environment variables it reads (`<prefix>REQUIRE_ASAN`, `<prefix>FIXES_RED_CI`) or sets (`<prefix>SCREENS`); `docs` refuses a test reading a `<prefix>` variable outside the editor harness | `EQ1_` | `docs`, `build`, `screens`, `cpu`, `null`, `size`, `tsan`, `asan`, the pre-push hook |
| `SOURCE_DIRS` | Directories, space-separated, where code may not cite a closed ticket (`scripts/cited-tickets.sh`, also used by `merge-on-green.sh`) | `"engine plugin tools tests"` | `docs` |
| `TESTS_DIR` | The tests' source directory, and their build folder under `BUILD_DIR`; one directory | `tests` | `docs`, `screens`, `cpu`, `null`, `paint`, `size`, `validate` |
| `TEST_STEP` | The `test` stage's heading: the name of the CI step that runs it. Optional: it defaults to eq1's | `"Engine and Plugin Shell tests"` | (optional: heads `test`) |
| `PLUGIN_DIR` | The Plugin Shell's source directory and its build folder; `colours` reads it. A library with no Plugin Shell (plugkit itself) leaves it out, and `docs` skips `colours`, saying so | `plugin` | `validate`, `size` (and `docs`/`colours` when set) |
| `ENGINE_DIR` | The Engine's build folder, for `size`'s code by library | `engine` | `size` |
| `EDITOR_HARNESS` | The one test file allowed to run timers, read `<prefix>` variables and open a Graphics on an image. Optional: leave it out when the harness comes from plugkit-ui | `tests/plugin/EditorHarness.h` | (optional: read by `docs`) |
| `TOKENS_DIR` | The one place in `PLUGIN_DIR` allowed literal colours. Optional: leave it out when the tokens come from plugkit-ui | `plugin/staple/` | (optional: read by `colours`) |
| `LAB_RANGES` | Parameter ranges, space-separated, that must match between the lab and the Engine. Optional: leave it empty to skip the comparison | `"frequency gain q slope"` | (optional: read by `docs`) |
| `LAB_SCRIPT` | The lab file holding `<NAME>_RANGE = (lo, hi)` lines | `tools/filter-lab/filterlab.py` | `docs`, when `LAB_RANGES` is set |
| `LAB_SETTINGS` | The Engine header holding `inline constexpr ParameterRange <name> { lo, hi };` lines | `engine/include/eq1/Settings.h` | `docs`, when `LAB_RANGES` is set |
| `SAMPLE_RATES` | Every sample rate the plugin supports, comma-separated, for pluginval | `44100,48000,88200,96000,176400,192000` | `validate` |
| `AU_ID` | The AU's type, subtype and manufacturer codes, for auval | `"aufx Eq01 Pmbg"` | `validate` (macOS) |
| `BUILD_INPUTS` | A bash array of what the build reads, so the hooks can check the tree they build is the one being committed or pushed | `(engine plugin tests cmake presets assets '*CMakeLists.txt')` | the git hooks |

eq1's `plugkit.conf` would read:

```sh
PLUGIN_NAME=eq1
PRODUCT_NAME=eq1
VAR_PREFIX=EQ1_
SOURCE_DIRS="engine plugin tools tests"
TESTS_DIR=tests
PLUGIN_DIR=plugin
ENGINE_DIR=engine
EDITOR_HARNESS=tests/plugin/EditorHarness.h
TOKENS_DIR=plugin/staple/
LAB_RANGES="frequency gain q slope"
LAB_SCRIPT=tools/filter-lab/filterlab.py
LAB_SETTINGS=engine/include/eq1/Settings.h
SAMPLE_RATES=44100,48000,88200,96000,176400,192000
AU_ID="aufx Eq01 Pmbg"
BUILD_INPUTS=(engine plugin tests cmake presets assets '*CMakeLists.txt')
```

### What the plugin stages expect

The stages that build or run a plugin expect eq1's layout, named by the keys above:

- **CMake options:** `<prefix>LTO`, `<prefix>STRIP`, `<prefix>BUILD_PLUGIN` and `<prefix>BUILD_TESTS`.
- **Tests:** the tests' executables are built in `BUILD_DIR/<TESTS_DIR>`, with Catch2 tags `[.screens]`, `[paint]` and `[footprint]`, and ctest labels `response` and `sweep`.
- **CPU budget:** `<name>_cpu_budget` takes `scenes` and `run <scene> <rate> <seconds>`.
- **Null check:** the render tool, `<name>_render`, is built from `<TESTS_DIR>/render/RenderTool.cpp` and takes `render <dir>` and `compare <a> <b>`.
- **CPU A/B:** copies `<TESTS_DIR>/cpu/` and `<TESTS_DIR>/engine/Measure.h` into the ref's tree.
- **Sidechain routing:** the check is a JUCE console app, `<name>_sidechain_routing_check`.

### Coding standards

plugkit's `CODING_STANDARDS.md` holds the coding standards plugkit and its plugins share. Each plugin keeps a copy at its root, where its agents and code review find it without looking anything up, at the version it pins:

- **The file:** the plugin's `CODING_STANDARDS.md` is plugkit's at the tag the plugin pins, byte for byte. A plugin with rules of its own adds a blank line, the marker, and its rules.
- **The marker:** the line `## <PLUGIN_NAME>'s own rules`, exactly, with the plugin's `PLUGIN_NAME`: `## comp1's own rules`. Everything below it is the plugin's own; everything above it is plugkit's. plugkit's own file has no such section.
- **The check:** `scripts/check.sh docs` compares the plugin's file with the `CODING_STANDARDS.md` of the plugkit checkout it runs from, which in CI is the pinned tag. A missing file, a difference above the marker, or no blank line before it fails, with the command that fixes it. Run on plugkit itself, it checks that the file exists and has no own-rules section.
- **Bumping `PLUGKIT_TAG`:** the same PR copies the new tag's file, from the plugin's root, with `$PLUGKIT` the new tag's checkout (`.deps/plugkit-src` once CMake has fetched it). Without own rules: `cp "$PLUGKIT/CODING_STANDARDS.md" .`. With them, keeping them:

  ```sh
  { cat "$PLUGKIT/CODING_STANDARDS.md"; echo; sed -n "/^## comp1's own rules\$/,\$p" CODING_STANDARDS.md; } > CODING_STANDARDS.md.new && mv CODING_STANDARDS.md.new CODING_STANDARDS.md
  ```

- **From v0.3.0:** the file and its check come with plugkit v0.3.0. A plugin pinning an earlier tag has neither; the PR that bumps it to v0.3.0 adds its copy.

### A plugin's CI

The plugin's workflow mirrors plugkit's own: the macOS Universal and Windows x64 matrix, with sccache and a cached `.deps`. Its jobs keep the names `macOS Universal` and `Windows x64`, because `merge-on-green.sh` waits for both by name. Each step calls one stage, from the plugin's checkout:

```yaml
    env:
      FETCHCONTENT_BASE_DIR: ${{ github.workspace }}/.deps
      GH_TOKEN: ${{ github.token }}
    steps:
      - uses: actions/checkout@v5
      # ... Ninja, the MSVC developer environment, sccache and the .deps cache, as in plugkit's ci.yml
      # A job's env can't read the runner context, so the later steps find the checkout through GITHUB_ENV, as a
      # path Git Bash runs scripts from on Windows too.
      - name: plugkit at the pinned tag
        run: |
          PLUGKIT=$(cygpath -u "$RUNNER_TEMP" 2> /dev/null || echo "$RUNNER_TEMP")/plugkit
          echo "PLUGKIT=$PLUGKIT" >> "$GITHUB_ENV"
          git clone --quiet --depth 1 --branch "$(sed -n 's/^PLUGKIT_TAG=//p' plugkit.conf)" https://github.com/Patrick-M-Bourget/plugkit.git "$PLUGKIT"
      - name: Doc references
        run: '"$PLUGKIT/scripts/check.sh" docs'
      - name: Build
        run: '"$PLUGKIT/scripts/check.sh" build'
      - name: Engine and Plugin Shell tests
        timeout-minutes: 8
        run: '"$PLUGKIT/scripts/check.sh" test'
      - name: CPU budget
        timeout-minutes: 3
        run: '"$PLUGKIT/scripts/check.sh" cpu'
      - name: Engine tests under ThreadSanitizer
        if: runner.os == 'macOS' && github.event_name == 'push'
        timeout-minutes: 15
        run: '"$PLUGKIT/scripts/check.sh" tsan'
      - name: Validate plugins (pluginval, auval, clap-validator, AAX and Standalone built)
        if: runner.os == 'Windows' || github.event_name == 'push'
        timeout-minutes: 8
        run: '"$PLUGKIT/scripts/check.sh" validate'
```

The other stages run by hand from the plugin's root: `null <ref>`, `cpu <ref>`, `paint`, `size`, `asan`, `focus <regex>`, `screens <dir>`. `hooks` tests plugkit's own scripts. A plugin can run it, but plugkit's CI already does.

### Hooks

- **Git hooks:** run `<plugkit>/scripts/install-hooks.sh` from the plugin's root. It points `core.hooksPath` at plugkit's `.githooks`. pre-commit runs `docs`, then builds and tests the tree (`gate.sh`). pre-push gates the pushed branch the same way, and holds a push onto a branch whose CI failed until `<prefix>FIXES_RED_CI=1`.
- **Worktree hook:** add `.claude/hooks/one-branch-per-worktree.sh`, from plugkit's checkout, as a `PreToolUse` hook on `Bash` in the plugin's `.claude/settings.json`. It keeps the main checkout on `main`.

### plugkit's own CI

plugkit is a library, so `.github/workflows/ci.yml` runs only `docs`, `hooks` (on macOS, where the hooks run), and `build` and `test` of `plugkit-dsp`, `plugkit-ui` and their tests, using its own `plugkit.conf`. It runs on pushes to `main` and on pull requests to any branch.
