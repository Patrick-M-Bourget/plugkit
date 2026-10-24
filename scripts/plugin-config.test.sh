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

standards=$(dirname "$check")/../CODING_STANDARDS.md

# A scratch plugin repository holding a plugkit.conf with the given lines and a copy of plugkit's
# CODING_STANDARDS.md; prints its path.
plugin() {
    local repo
    repo=$(mktemp -d "$tmp/plugin.XXXXXX")
    mkdir -p "$repo/engine/include/demo" "$repo/tests" "$repo/tools/lab"
    printf '%s\n' "$@" > "$repo/plugkit.conf"
    cp "$standards" "$repo/CODING_STANDARDS.md"
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
echo "// See eq1's docs/missing"'.md, "Nowhere".' > "$repo/engine/a.cpp"
expect pass "" "docs skips a pointer into another repository's docs, prefixed with its name" "$repo" docs

repo=$(plugin "${base[@]}")
echo "// See demo's docs/missing"'.md, "Nowhere".' > "$repo/engine/a.cpp"
expect fail "docs/missing" "a pointer prefixed with the plugin's own name is checked" "$repo" docs

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

repo=$(plugin "${base[@]/PLUGIN_DIR=plugin/}")
echo 'g.setColour (juce::Colours::white);' > "$repo/engine/Editor.cpp"
expect pass "Skipped the colours check: no PLUGIN_DIR" "a library with no Plugin Shell leaves PLUGIN_DIR out, and docs skips colours, saying so" "$repo" docs

# Nothing is built here, so the stage fails; its heading is what's checked.
repo=$(plugin "${base[@]}")
expect fail "== Engine and Plugin Shell tests" "test is headed as a plugin's CI step names it by default" "$repo" test
repo=$(plugin "${base[@]}" 'TEST_STEP="demo-dsp tests"')
expect fail "== demo-dsp tests" "TEST_STEP heads the test stage" "$repo" test

# focus, in a plugin whose CMake project has no build folder yet: a stand-in for each test target, recording that it
# was built, and two tests, one failing, so that running a test the regex doesn't match fails the stage.
focus_project() {
    local repo=$1 target
    shift
    {
        printf 'cmake_minimum_required(VERSION 3.22)\nproject(demo NONE)\nenable_testing()\n'
        for target in "$@"; do
            printf 'add_custom_target(%s COMMAND ${CMAKE_COMMAND} -E touch built-%s)\n' "$target" "$target"
        done
        printf 'add_test(NAME Tooltip.shows COMMAND ${CMAKE_COMMAND} -E echo ok)\n'
        printf 'add_test(NAME Other.fails COMMAND ${CMAKE_COMMAND} -E false)\n'
    } > "$repo/CMakeLists.txt"
}

# expect_built <what> <build folder> <target...>: each target was built in the folder.
expect_built() {
    local what=$1 dir=$2 target
    shift 2
    for target in "$@"; do
        [ -f "$dir/built-$target" ] || { echo "FAIL: $what: $target wasn't built" >&2; failures=$((failures + 1)); }
    done
}

repo=$(plugin "${base[@]}")
focus_project "$repo" demo_engine_tests demo_plugin_tests
FETCHCONTENT_BASE_DIR=$repo/.deps expect pass "100% tests passed" \
    "focus configures a missing build folder, then builds the default test targets and runs only the matching tests" \
    "$repo" focus Tooltip
expect_built "focus builds <name>_engine_tests and <name>_plugin_tests by default" "$repo/build" demo_engine_tests demo_plugin_tests

repo=$(plugin "${base[@]}" 'TEST_TARGETS="demo_dsp_tests demo_plugin_tests"')
focus_project "$repo" demo_dsp_tests demo_plugin_tests
FETCHCONTENT_BASE_DIR=$repo/.deps expect pass "100% tests passed" "focus builds the TEST_TARGETS the config names" \
    "$repo" focus Tooltip
expect_built "focus builds every TEST_TARGETS target" "$repo/build" demo_dsp_tests demo_plugin_tests

repo=$(plugin "${base[@]}")
focus_project "$repo" demo_engine_tests demo_plugin_tests
FETCHCONTENT_BASE_DIR=$repo/.deps expect fail "No tests were found" "focus with a regex matching no test fails" \
    "$repo" focus Nothing

# asan, on a stand-in macOS (uname) whose AddressSanitizer starts (a c++ whose programs exit at once), so it runs
# anywhere and quickly: focus's project without its failing test, plus a target that isn't a test, which must stay
# unbuilt.
fake_mac() {
    local bin=$1/fake-bin
    mkdir -p "$bin"
    printf '#!/usr/bin/env bash\necho Darwin\n' > "$bin/uname"
    printf '#!/usr/bin/env bash\nwhile [ $# -gt 0 ]; do [ "$1" = -o ] && out=$2; shift; done\n%s\nchmod +x "$out"\n' \
        "printf '#!/bin/sh\\nexit 0\\n' > \"\$out\"" > "$bin/c++"
    chmod +x "$bin/uname" "$bin/c++"
    printf '%s\n' "$bin"
}

# expect_unbuilt <what> <build folder> <target...>: no target was built in the folder.
expect_unbuilt() {
    local what=$1 dir=$2 target
    shift 2
    for target in "$@"; do
        [ ! -f "$dir/built-$target" ] || { echo "FAIL: $what: $target was built" >&2; failures=$((failures + 1)); }
    done
}

repo=$(plugin "${base[@]}")
focus_project "$repo" demo_engine_tests demo_plugin_tests demo_tool
sed -i.bak /Other.fails/d "$repo/CMakeLists.txt" && rm "$repo/CMakeLists.txt.bak"
PATH=$(fake_mac "$repo"):$PATH FETCHCONTENT_BASE_DIR=$repo/.deps expect pass "100% tests passed" \
    "asan builds the default test targets and runs the tests" "$repo" asan
expect_built "asan builds <name>_engine_tests and <name>_plugin_tests by default" "$repo/build-asan" demo_engine_tests demo_plugin_tests
expect_unbuilt "asan builds only the test targets" "$repo/build-asan" demo_tool

repo=$(plugin "${base[@]}" 'TEST_TARGETS="demo_dsp_tests demo_plugin_tests"')
focus_project "$repo" demo_dsp_tests demo_plugin_tests demo_tool
sed -i.bak /Other.fails/d "$repo/CMakeLists.txt" && rm "$repo/CMakeLists.txt.bak"
PATH=$(fake_mac "$repo"):$PATH FETCHCONTENT_BASE_DIR=$repo/.deps expect pass "100% tests passed" \
    "asan builds the TEST_TARGETS the config names" "$repo" asan
expect_built "asan builds every TEST_TARGETS target" "$repo/build-asan" demo_dsp_tests demo_plugin_tests
expect_unbuilt "asan builds only the TEST_TARGETS" "$repo/build-asan" demo_tool

# screens and paint, in a configured scratch project: each test target a stand-in executable holding tests with the
# given tags (<target>:<tag,...>), which lists the tests a tag matches as Catch2 does and records each run, with its
# arguments and the screens folder, in ran-<target>.
tagged_project() {
    local repo=$1 spec target
    shift
    mkdir -p "$repo/build/tests"
    for spec in "$@"; do
        target=${spec%%:*}
        printf '#!/usr/bin/env bash\ntags=%q\n' "${spec#*:}" > "$repo/build/tests/$target"
        cat >> "$repo/build/tests/$target" << 'EOF'
tag=${@: -1}
if [ "$1" = --list-tests ]; then
    case ",$tags," in *",$tag,"*) echo "A test tagged $tag" ;; esac
else
    echo "$* ${DEMO_SCREENS:-}" > "${0%/*}/ran-${0##*/}"
fi
EOF
        chmod +x "$repo/build/tests/$target"
    done
    focus_project "$repo" "${@%%:*}"
    cmake -S "$repo" -B "$repo/build" > /dev/null
}

# expect_ran <what> <repo> <target> <text>: the stand-in for target ran, its record holding text.
expect_ran() {
    grep -qF -- "$4" "$2/build/tests/ran-$3" 2> /dev/null || {
        echo "FAIL: $1: $3 didn't run with '$4'" >&2
        failures=$((failures + 1))
    }
}

# expect_not_ran <what> <repo> <target>: the stand-in for target never ran.
expect_not_ran() {
    [ ! -f "$2/build/tests/ran-$3" ] || { echo "FAIL: $1: $3 ran" >&2; failures=$((failures + 1)); }
}

repo=$(plugin "${base[@]}")
tagged_project "$repo" demo_engine_tests:'[response]' demo_plugin_tests:'[.screens],[paint]'
PATH=$(fake_mac "$repo"):$PATH expect pass "" "screens runs the default test targets' screens tests" "$repo" screens "$repo/shots"
expect_ran "screens renders from the test target holding [.screens]" "$repo" demo_plugin_tests "[.screens] $repo/shots"
expect_not_ran "screens leaves a test target without [.screens] tests" "$repo" demo_engine_tests

repo=$(plugin "${base[@]}" 'TEST_TARGETS="demo_dsp_tests demo_ui_tests"')
tagged_project "$repo" demo_dsp_tests:'[response]' demo_ui_tests:'[.screens],[paint]'
PATH=$(fake_mac "$repo"):$PATH expect pass "" "screens runs the TEST_TARGETS' screens tests" "$repo" screens "$repo/shots"
expect_built "screens builds every TEST_TARGETS target" "$repo/build" demo_dsp_tests demo_ui_tests
expect_ran "screens renders from the TEST_TARGETS target holding [.screens]" "$repo" demo_ui_tests "[.screens] $repo/shots"
expect_not_ran "screens leaves a TEST_TARGETS target without [.screens] tests" "$repo" demo_dsp_tests
GITHUB_ACTIONS=1 PATH=$(fake_mac "$repo"):$PATH expect pass "" "paint runs the TEST_TARGETS' paint test" "$repo" paint
expect_ran "paint times the TEST_TARGETS target holding [paint]" "$repo" demo_ui_tests "[paint]"

repo=$(plugin "${base[@]}" 'TEST_TARGETS="demo_dsp_tests demo_ui_tests"')
tagged_project "$repo" demo_dsp_tests:'[response]' demo_ui_tests:'[paint]'
PATH=$(fake_mac "$repo"):$PATH expect fail "No test in demo_dsp_tests demo_ui_tests matches [.screens]" \
    "screens with no [.screens] test in any test target fails, saying so" "$repo" screens "$repo/shots"

repo=$(plugin "${base[@]}")
expect fail "SAMPLE_RATES" "a stage needing a setting the config leaves out says which" "$repo" validate

repo=$(plugin "${base[@]}")
expect fail "check.sh docs" "an unknown stage prints the usage" "$repo" nonsense

repo=$(plugin "${base[@]}")
rm "$repo/CODING_STANDARDS.md"
expect fail "has no CODING_STANDARDS.md" "a plugin without CODING_STANDARDS.md fails docs" "$repo" docs

repo=$(plugin "${base[@]}")
sed -i.bak 's/Judgement calls/Rules/' "$repo/CODING_STANDARDS.md" && rm "$repo/CODING_STANDARDS.md.bak"
expect fail "cp $(cd "$(dirname "$standards")" && pwd)/CODING_STANDARDS.md" \
    "a copy that drifts from plugkit's fails docs, giving the command that copies plugkit's" "$repo" docs

repo=$(plugin "${base[@]}")
printf "\n## demo's own rules\n\nDemo only.\n" >> "$repo/CODING_STANDARDS.md"
expect pass "" "the plugin's own rules under its marker section are its own" "$repo" docs

repo=$(plugin "${base[@]}")
printf "\n## demo's own rules\n\nDemo only.\n" >> "$repo/CODING_STANDARDS.md"
sed -i.bak 's/Judgement calls/Rules/' "$repo/CODING_STANDARDS.md" && rm "$repo/CODING_STANDARDS.md.bak"
expect fail "the part above '## demo's own rules' isn't plugkit's" "a shared part that drifts above the marker fails" "$repo" docs
# The command the message gives restores the shared part and keeps the plugin's own rules.
fix=$(run "$repo" docs | sed -n 's/.*keeping the plugin.s own rules: //p')
(cd "$repo" && eval "$fix")
expect pass "" "the command docs gives fixes the copy" "$repo" docs
grep -qx 'Demo only.' "$repo/CODING_STANDARDS.md" || { echo "FAIL: the fix dropped the plugin's own rules" >&2; failures=$((failures + 1)); }

repo=$(plugin "${base[@]}")
printf "## demo's own rules\n\nDemo only.\n" >> "$repo/CODING_STANDARDS.md"
expect fail "then a blank line" "the marker section follows a blank line" "$repo" docs

repo=$(plugin "${base[@]}")
printf "\n## other's own rules\n\nOther only.\n" >> "$repo/CODING_STANDARDS.md"
expect fail "has no '## demo's own rules' section" "only the plugin's own name marks its section" "$repo" docs

# A scratch plugkit checkout, run on itself: its CODING_STANDARDS.md is the shared one, with no plugin's section.
kit=$(mktemp -d "$tmp/plugkit.XXXXXX")
mkdir "$kit/scripts"
cp "$(dirname "$check")"/{check,config,cited-tickets}.sh "$kit/scripts/"
cp "$standards" "$kit/CODING_STANDARDS.md"
printf '%s\n' "${base[@]/PLUGIN_NAME=demo/PLUGIN_NAME=plugkit}" > "$kit/plugkit.conf"
git -C "$kit" init -q
check=$kit/scripts/check.sh expect pass "" "plugkit's own CODING_STANDARDS.md passes on plugkit" "$kit" docs
printf "\n## plugkit's own rules\n\nplugkit only.\n" >> "$kit/CODING_STANDARDS.md"
check=$kit/scripts/check.sh expect fail "plugkit's CODING_STANDARDS.md is the shared part only" \
    "plugkit's own CODING_STANDARDS.md has no own-rules section" "$kit" docs
rm "$kit/CODING_STANDARDS.md"
check=$kit/scripts/check.sh expect fail "has no CODING_STANDARDS.md" "plugkit without CODING_STANDARDS.md fails" "$kit" docs

if [ "$failures" = 0 ]; then
    echo "A plugin runs the harness from its own repository, with its own plugkit.conf"
else
    exit 1
fi
