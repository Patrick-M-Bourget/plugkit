# plugkit

The shared library for the [eq1](https://github.com/Patrick-M-Bourget/eq1) and [comp1](https://github.com/Patrick-M-Bourget/comp1) audio plugins. It has two modules, mirroring eq1's own split:

- **`plugkit-dsp`:** plain C++ with no JUCE. It holds the Smoother and the 16-sample run grid, the matched filter designs, Placement, and LatestValue, the lock-free hand-off of the newest settings from the UI thread to the audio thread.
- **`plugkit-ui`:** the JUCE layer. It holds the UI kit and the Plugin Shell's Presets, A/B Compare and edit history.

The CI scripts and the check harness live here too.

## Rules

- **Copied from eq1 until eq1 ships.** eq1 doesn't change before then, so everything here is a copy of its code. Each copied piece, and every eq1 fix ported by hand, is listed in [eq1#231](https://github.com/Patrick-M-Bourget/eq1/issues/231), the ticket that later migrates eq1 onto plugkit. See comp1's ADR 0006.
- **Pinned per plugin.** Each plugin pins an exact plugkit tag or commit through FetchContent, never a branch. Bumping the pin is that plugin's own PR, through its full checks. See comp1's ADR 0007.
- **Grown in slices**, each just before comp1 needs it:
  1. CI and the check harness ([comp1#2](https://github.com/Patrick-M-Bourget/comp1/issues/2)).
  2. `plugkit-dsp` ([comp1#5](https://github.com/Patrick-M-Bourget/comp1/issues/5)).
  3. `plugkit-ui` ([comp1#22](https://github.com/Patrick-M-Bourget/comp1/issues/22)).

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

### Per-plugin config

`plugkit.conf` is a bash file of `KEY=value` lines. The harness sources it, so quote values that hold spaces. `PLUGKIT_CONFIG=<path>` names a different file, relative to the plugin's root. A stage that needs a key the file leaves empty stops and names that key. "Needed by" names exactly those stages, and in parentheses the optional uses; `hooks` checks it against `check.sh` (`scripts/needed-by.py`). The values in the table are eq1's.

| Key | What it is | Example (eq1) | Needed by |
|---|---|---|---|
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

plugkit is a library, so `.github/workflows/ci.yml` runs only `docs`, `hooks` (on macOS, where the hooks run), and `build` and `test` of `plugkit-dsp` and its tests, using its own `plugkit.conf`. It runs on pushes to `main` and on pull requests to any branch.
