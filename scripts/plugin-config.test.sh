#!/usr/bin/env bash
# Tests how a plugin runs plugkit's check harness: scripts/check.sh called from a scratch plugin repository, which
# stays the working directory, reading that repository's plugkit.conf (or the file PLUGKIT_CONFIG names).
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
unset $(git rev-parse --local-env-vars)
check=$(cd "$(dirname "$0")" && pwd)/check.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
# No gh: the closed-ticket check is skipped, as where gh isn't signed in.
mkdir "$tmp/bin"
printf '#!/usr/bin/env bash\nexit 1\n' > "$tmp/bin/gh"
chmod +x "$tmp/bin/gh"
export PATH=$tmp/bin:$PATH
failures=0

# A scratch plugin repository holding a plugkit.conf with the given lines; prints its path.
plugin() {
    local repo
    repo=$(mktemp -d "$tmp/plugin.XXXXXX")
    mkdir -p "$repo/engine/include/demo" "$repo/tests" "$repo/tools/lab"
    printf '%s\n' "$@" > "$repo/plugkit.conf"
    echo 'int main() { return 0; }' > "$repo/tests/Test.cpp"
    git -C "$repo" init -q
    printf '%s\n' "$repo"
}

# run <repo> <stage...>: runs the harness from inside the repository; prints its output, returns its status.
run() {
    local repo=$1
    shift
    (cd "$repo" && "$check" "$@" 2>&1)
}

# expect <pass|fail> <output text or ""> <what> <repo> <stage...>
expect() {
    local want=$1 text=$2 what=$3 repo=$4 out got=pass
    shift 4
    git -C "$repo" add -A
    out=$(run "$repo" "$@") || got=fail
    if [ "$got" != "$want" ] || { [ -n "$text" ] && ! grep -qF -- "$text" <<< "$out"; }; then
        echo "FAIL: $what: expected $want${text:+ saying '$text'}, got $got:" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

base=(PLUGIN_NAME=demo PRODUCT_NAME=Demo VAR_PREFIX=DEMO_ 'SOURCE_DIRS="engine tests tools"' TESTS_DIR=tests PLUGIN_DIR=plugin)

repo=$(plugin "${base[@]}")
rm "$repo/plugkit.conf"
expect fail "plugkit.conf" "a plugin repository without plugkit.conf is refused, naming the file" "$repo" docs

repo=$(plugin "${base[@]}")
mv "$repo/plugkit.conf" "$repo/elsewhere.conf"
PLUGKIT_CONFIG=elsewhere.conf expect pass "" "PLUGKIT_CONFIG names another config file" "$repo" docs
unset PLUGKIT_CONFIG

repo=$(plugin "${base[@]}")
expect pass "" "a clean plugin passes docs" "$repo" docs

repo=$(plugin "${base[@]}")
# Split, so plugkit's own docs check doesn't read it as a citation.
echo '// See docs/missing''.md, "Nowhere".' > "$repo/engine/a.cpp"
expect fail "docs/missing" "docs reads the plugin's files, not plugkit's" "$repo" docs

repo=$(plugin "${base[@]}")
mkdir -p "$repo/engine/sub"
echo '// See docs/missing''.md.' > "$repo/engine/a.cpp"
expect fail "docs/missing" "called from a subdirectory, it checks the whole plugin repository" "$repo/engine/sub" docs

repo=$(plugin "${base[@]}")
echo 'auto v = std::getenv ("DEMO_RENDERS");' > "$repo/tests/Env.cpp"
expect fail "reads a DEMO_ environment variable" "the env-variable rule uses the plugin's VAR_PREFIX" "$repo" docs

repo=$(plugin "${base[@]}")
echo 'auto v = std::getenv ("OTHER_RENDERS");' > "$repo/tests/Env.cpp"
expect pass "" "another prefix's variable isn't the plugin's" "$repo" docs

repo=$(plugin "${base[@]}" EDITOR_HARNESS=tests/Harness.h)
echo 'auto v = std::getenv ("DEMO_RENDERS");' > "$repo/tests/Harness.h"
expect pass "" "the editor harness may read the plugin's variables" "$repo" docs

repo=$(plugin "${base[@]}" 'TESTS_DIR=checks')
mkdir -p "$repo/checks"
echo 'auto v = std::getenv ("DEMO_RENDERS");' > "$repo/checks/Env.cpp"
expect fail "checks/Env.cpp" "the test rules read TESTS_DIR" "$repo" docs

lab=(LAB_SCRIPT=tools/lab/lab.py LAB_SETTINGS=engine/include/demo/Settings.h 'LAB_RANGES="ratio knee"')
repo=$(plugin "${base[@]}" "${lab[@]}")
printf 'RATIO_RANGE = (1, 100)\nKNEE_RANGE = (0, 72)\n' > "$repo/tools/lab/lab.py"
printf 'inline constexpr ParameterRange ratio { 1.0, 100.0 };\ninline constexpr ParameterRange knee { 0.0, 72.0 };\n' \
    > "$repo/engine/include/demo/Settings.h"
expect pass "" "the lab's ranges matching the plugin's pass" "$repo" docs
printf 'inline constexpr ParameterRange ratio { 1.0, 20.0 };\ninline constexpr ParameterRange knee { 0.0, 72.0 };\n' \
    > "$repo/engine/include/demo/Settings.h"
expect fail "tools/lab/lab.py: ratio's range" "a lab range that drifts from the plugin's fails, naming both files" "$repo" docs

repo=$(plugin "${base[@]}" PLUGIN_DIR=shell TOKENS_DIR=shell/kit/)
mkdir -p "$repo/shell/kit"
echo 'inline const juce::Colour ink { 0xff102030 };' > "$repo/shell/kit/Tokens.h"
expect pass "" "colours allows literals in TOKENS_DIR" "$repo" colours
echo 'g.setColour (juce::Colours::white);' > "$repo/shell/Editor.cpp"
expect fail "shell/Editor.cpp:1:" "colours reads PLUGIN_DIR" "$repo" colours

repo=$(plugin "${base[@]}")
expect fail "SAMPLE_RATES" "a stage needing a setting the config leaves out says which" "$repo" validate

repo=$(plugin "${base[@]}")
expect fail "check.sh docs" "an unknown stage prints the usage" "$repo" nonsense

if [ "$failures" = 0 ]; then
    echo "A plugin runs the harness from its own repository, with its own plugkit.conf"
else
    exit 1
fi
