#!/usr/bin/env bash
# Tests the plugkit pin check: in a linked worktree sharing the main checkout's dependencies, a stage that runs CMake
# stops when that folder's plugkit checkout (.deps/plugkit-src) isn't at the PLUGKIT_TAG the worktree's plugkit.conf
# pins. Runs scripts/check.sh from scratch plugin repositories, with fakes of cmake and ctest, so nothing is built.
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
unset $(git rev-parse --local-env-vars)
unset GITHUB_ACTIONS FETCHCONTENT_BASE_DIR
check=$(cd "$(dirname "$0")" && pwd)/check.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com
export GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com
# cmake and ctest only say they ran.
mkdir "$tmp/bin"
for tool in cmake ctest; do
    printf '#!/usr/bin/env bash\necho "%s ran"\n' "$tool" > "$tmp/bin/$tool"
    chmod +x "$tmp/bin/$tool"
done
export PATH=$tmp/bin:$PATH
failures=0

# A plugkit repository with two releases, v0.1.0 and v0.2.0, to clone as the dependencies' checkout.
upstream=$tmp/plugkit
git init -q "$upstream"
git -C "$upstream" commit -q --allow-empty -m one && git -C "$upstream" tag v0.1.0
git -C "$upstream" commit -q --allow-empty -m two && git -C "$upstream" tag v0.2.0
v2=$(git -C "$upstream" rev-parse v0.2.0)

# plugin <deps tag or ""> [<conf line>...]: a scratch plugin repository, its main checkout's .deps/plugkit-src cloned
# from upstream at <deps tag> (none for ""), its plugkit.conf the base lines and the given ones, committed, and a
# linked worktree of it; prints the worktree's path, the repository's being its parent's.
plugin() {
    local repo tag=$1
    shift
    repo=$(mktemp -d "$tmp/plugin.XXXXXX")
    mkdir "$repo/main"
    printf '%s\n' PLUGIN_NAME=demo PRODUCT_NAME=Demo VAR_PREFIX=DEMO_ TESTS_DIR=tests PLUGIN_DIR=plugin \
        ENGINE_DIR=engine "$@" > "$repo/main/plugkit.conf"
    # What the stages read before CMake runs: null's render tool, the CPU A/B's scenes.
    mkdir -p "$repo/main/tests/render" "$repo/main/tests/cpu" "$repo/main/tests/engine"
    touch "$repo/main/tests/render/RenderTool.cpp" "$repo/main/tests/cpu/Scenes.cpp" "$repo/main/tests/engine/Measure.h"
    git -C "$repo/main" init -q
    git -C "$repo/main" add -A && git -C "$repo/main" commit -q -m conf
    git -C "$repo/main" worktree add -q "$repo/linked"
    # focus writes its log into the build folder.
    mkdir "$repo/linked/build"
    if [ -n "$tag" ]; then
        mkdir "$repo/main/.deps"
        git clone -q "$upstream" "$repo/main/.deps/plugkit-src"
        git -C "$repo/main/.deps/plugkit-src" checkout -q "$tag"
    fi
    printf '%s\n' "$repo/linked"
}

# expect <pass|fail> <output text or ""> <what> <dir> <stage...>
expect() {
    local want=$1 text=$2 what=$3 dir=$4 out got=pass
    shift 4
    out=$(cd "$dir" && "$check" "$@" 2>&1) || got=fail
    if [ "$got" != "$want" ] || { [ -n "$text" ] && ! grep -qF -- "$text" <<< "$out"; }; then
        echo "FAIL: $what: expected $want${text:+ saying '$text'}, got $got:" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

# expect_not <text> <what> <dir> <stage...>: the stage's output doesn't hold <text>.
expect_not() {
    local text=$1 what=$2 dir=$3 out
    shift 3
    out=$(cd "$dir" && "$check" "$@" 2>&1)
    if grep -qF -- "$text" <<< "$out"; then
        echo "FAIL: $what: the output says '$text':" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

# fix <dir> <which>: the fix the message gives, the refresh or the worktree's own dependencies.
fix() {
    (cd "$1" && "$check" build 2>&1) | sed -n "s/^ *$2: //p"
}

linked=$(plugin v0.2.0 PLUGKIT_TAG=v0.2.0)
expect pass "cmake ran" "a shared checkout at the pinned tag builds" "$linked" build

linked=$(plugin v0.1.0 PLUGKIT_TAG=v0.2.0)
main=$(dirname "$linked")/main
expect fail "isn't at v0.2.0" "a shared checkout at another tag stops build" "$linked" build
expect fail "it is at v0.1.0" "the message names the checkout's version" "$linked" build
expect fail "$main/.deps/plugkit-src" "the message names the folder" "$linked" build
expect_not "cmake ran" "a mismatch stops before CMake runs" "$linked" build
# Every stage that runs CMake, each at its first CMake call; focus sends CMake's output to a log, and null and
# cpu <ref> configure the ref's tree into one. cpu and paint first ask whether the machine is busy, and asan whether
# AddressSanitizer starts, so they aren't run here.
for stage in "focus x" "screens shots" "null HEAD" "cpu HEAD" size tsan; do
    # shellcheck disable=SC2086 # a stage and its argument
    expect fail "isn't at v0.2.0" "a mismatch stops $stage, which runs CMake" "$linked" $stage
done
expect pass "ctest ran" "test runs no CMake, and isn't checked" "$linked" test
expect pass "cmake ran" "the main checkout isn't checked: a changed pin reconfigures there, and CMake fetches it" \
    "$main" build
FETCHCONTENT_BASE_DIR=$main/.deps expect pass "cmake ran" \
    "an explicit FETCHCONTENT_BASE_DIR isn't checked: CMake fetches the pin into it" "$linked" build
GITHUB_ACTIONS=true expect pass "cmake ran" "CI isn't checked" "$linked" build

own=$(fix "$linked" "or build with the worktree's own dependencies")
if ! out=$(cd "$linked" && eval "$own" 2>&1) || ! grep -q "^cmake ran" <<< "$out"; then
    echo "FAIL: the own-dependencies fix doesn't build:" >&2
    sed 's/^/    /' <<< "$out" >&2
    failures=$((failures + 1))
fi
# mktemp's folder may be reached through /private on macOS.
[[ $own == "FETCHCONTENT_BASE_DIR=$linked/.deps "* || $own == "FETCHCONTENT_BASE_DIR=/private$linked/.deps "* ]] ||
    { echo "FAIL: the own-dependencies fix names '$own', not the worktree's .deps" >&2; failures=$((failures + 1)); }

refresh=$(fix "$linked" "refresh it")
(cd "$linked" && eval "$refresh") > /dev/null 2>&1
expect pass "cmake ran" "the refresh the message gives fixes the checkout" "$linked" build

linked=$(plugin v0.1.0 "PLUGKIT_TAG=$v2")
expect fail "isn't at $v2" "a commit-hash pin is checked too" "$linked" build
refresh=$(fix "$linked" "refresh it")
(cd "$linked" && eval "$refresh") > /dev/null 2>&1
expect pass "cmake ran" "the refresh the message gives fetches a commit-hash pin" "$linked" build

linked=$(plugin v0.2.0 PLUGKIT_TAG=v0.3.0)
expect fail "has no v0.3.0" "a shared checkout that lacks the pinned tag stops build" "$linked" build

linked=$(plugin "" PLUGKIT_TAG=v0.2.0)
mkdir -p "$(dirname "$linked")/main/.deps"
expect pass "cmake ran" "no checkout yet (a first build): build runs, and CMake fetches it" "$linked" build

linked=$(plugin "" PLUGKIT_TAG=v0.2.0)
mkdir -p "$(dirname "$linked")/main/.deps/plugkit-src"
expect pass "cmake ran" "a plugkit-src that isn't a git checkout isn't checked" "$linked" build

linked=$(plugin v0.1.0)
expect pass "cmake ran" "no PLUGKIT_TAG (plugkit's own repository): nothing is checked" "$linked" build

if [ "$failures" = 0 ]; then
    echo "A linked worktree refuses a shared plugkit checkout that isn't at the pinned tag"
else
    exit 1
fi
