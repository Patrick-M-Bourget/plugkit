#!/usr/bin/env bash
# Tests the plugkit pin check: a stage that configures or builds stops when the dependencies' plugkit checkout
# (<deps>/plugkit-src) isn't at the PLUGKIT_TAG the plugin's plugkit.conf pins, and in CI fetches it again.
# Runs scripts/check.sh from scratch plugin repositories, with fakes of cmake and ctest, so nothing is built.
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
unset $(git rev-parse --local-env-vars)
unset GITHUB_ACTIONS FETCHCONTENT_BASE_DIR
check=$(cd "$(dirname "$0")" && pwd)/check.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com
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

# plugin [<conf line>...]: a scratch plugin repository with a plugkit.conf of the base lines and the given ones;
# prints its path.
plugin() {
    local repo
    repo=$(mktemp -d "$tmp/plugin.XXXXXX")
    printf '%s\n' PLUGIN_NAME=demo VAR_PREFIX=DEMO_ TESTS_DIR=tests "$@" > "$repo/plugkit.conf"
    git -C "$repo" init -q
    printf '%s\n' "$repo"
}

# checkout <deps> <tag>: the dependencies' plugkit checkout, cloned from upstream and at <tag>.
checkout() {
    mkdir -p "$1"
    git clone -q "$upstream" "$1/plugkit-src"
    git -C "$1/plugkit-src" checkout -q "$2"
}

# expect <pass|fail> <output text or ""> <what> <repo> <stage...>
expect() {
    local want=$1 text=$2 what=$3 repo=$4 out got=pass
    shift 4
    out=$(cd "$repo" && "$check" "$@" 2>&1) || got=fail
    if [ "$got" != "$want" ] || { [ -n "$text" ] && ! grep -qF -- "$text" <<< "$out"; }; then
        echo "FAIL: $what: expected $want${text:+ saying '$text'}, got $got:" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

# expect_not <text> <what> <repo> <stage...>: the stage's output doesn't hold <text>.
expect_not() {
    local text=$1 what=$2 repo=$3 out
    shift 3
    out=$(cd "$repo" && "$check" "$@" 2>&1)
    if grep -qF -- "$text" <<< "$out"; then
        echo "FAIL: $what: the output says '$text':" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$repo/.deps" v0.2.0
expect pass "cmake ran" "a checkout at the pinned tag builds" "$repo" build

repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$repo/.deps" v0.1.0
expect fail "isn't at v0.2.0" "a checkout at another tag stops build" "$repo" build
expect fail "it is at v0.1.0" "the message names the checkout's version" "$repo" build
expect fail "$repo/.deps/plugkit-src" "the message names the folder" "$repo" build
expect fail "FETCHCONTENT_BASE_DIR" "the message offers the worktree's own dependencies folder" "$repo" build
expect_not "cmake ran" "a mismatch stops before configuring" "$repo" build
for stage in "focus x" "screens shots" cpu paint "null HEAD" "cpu HEAD" size tsan asan all; do
    # shellcheck disable=SC2086 # a stage and its argument
    expect fail "isn't at v0.2.0" "a mismatch stops $stage, which builds" "$repo" $stage
done
expect pass "ctest ran" "test builds nothing, and runs on" "$repo" test
expect_not "isn't at v0.2.0" "test doesn't check the pin" "$repo" test

repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$repo/.deps" v0.1.0
# The fix the message gives.
fix=$(cd "$repo" && "$check" build 2>&1 | sed -n 's/^ *refresh it: //p')
(cd "$repo" && eval "$fix") > /dev/null 2>&1
expect pass "cmake ran" "the refresh the message gives fixes the checkout" "$repo" build

repo=$(plugin PLUGKIT_TAG=v0.3.0)
checkout "$repo/.deps" v0.2.0
expect fail "has no v0.3.0" "a checkout that lacks the pinned tag stops build" "$repo" build

repo=$(plugin PLUGKIT_TAG=v0.2.0)
expect pass "cmake ran" "no checkout yet (a first build): build runs, and CMake fetches it" "$repo" build

repo=$(plugin PLUGKIT_TAG=v0.2.0)
mkdir -p "$repo/.deps/plugkit-src"
expect pass "cmake ran" "a plugkit-src that isn't a git checkout isn't checked" "$repo" build

repo=$(plugin)
checkout "$repo/.deps" v0.1.0
expect pass "cmake ran" "no PLUGKIT_TAG (plugkit's own repository): nothing is checked" "$repo" build

repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$tmp/elsewhere" v0.1.0
FETCHCONTENT_BASE_DIR=$tmp/elsewhere expect fail "$tmp/elsewhere/plugkit-src" \
    "FETCHCONTENT_BASE_DIR's checkout is the one checked" "$repo" build
FETCHCONTENT_BASE_DIR=$tmp/fresh expect pass "cmake ran" "a FETCHCONTENT_BASE_DIR with no checkout yet builds" "$repo" build

# A linked worktree shares the main checkout's dependencies, which may be at an older pin.
repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$repo/.deps" v0.1.0
git -C "$repo" add plugkit.conf && git -C "$repo" commit -q -m conf
git -C "$repo" worktree add -q "$tmp/linked"
expect fail "$repo/.deps/plugkit-src" "a linked worktree checks the main checkout's shared checkout" "$tmp/linked" build
FETCHCONTENT_BASE_DIR=$tmp/linked/.deps expect pass "cmake ran" \
    "the worktree's own dependencies folder, as the message offers, builds" "$tmp/linked" build

# In CI the dependencies folder is a cache, which a pin change can restore from an older pin: the checkout is
# deleted, with its build folders, for CMake to fetch the pinned tag again.
repo=$(plugin PLUGKIT_TAG=v0.2.0)
checkout "$repo/.deps" v0.1.0
mkdir -p "$repo/.deps/plugkit-subbuild" "$repo/.deps/plugkit-build" "$repo/.deps/juce-src"
GITHUB_ACTIONS=true expect pass "cmake ran" "in CI a stale checkout is fetched again, not refused" "$repo" build
for gone in plugkit-src plugkit-subbuild plugkit-build; do
    [ ! -e "$repo/.deps/$gone" ] || { echo "FAIL: in CI the stale $gone is kept" >&2; failures=$((failures + 1)); }
done
[ -d "$repo/.deps/juce-src" ] || { echo "FAIL: in CI the other dependencies are deleted too" >&2; failures=$((failures + 1)); }

if [ "$failures" = 0 ]; then
    echo "A stage that builds refuses a plugkit checkout that isn't at the pinned tag"
else
    exit 1
fi
