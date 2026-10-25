#!/usr/bin/env bash
# Tests the install stage: it builds the VST3 and the AU Universal in a build folder of its own, copies them into
# ~/Library/Audio/Plug-Ins over any older copy, runs auval, and prints the installed paths, the commit and the line
# that uninstalls them. Runs scripts/check.sh from a scratch plugin repository, with fakes of cmake, auval and
# killall, and HOME in the scratch folder, so nothing is built and the real ~/Library is untouched.
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
unset $(git rev-parse --local-env-vars)
unset GITHUB_ACTIONS FETCHCONTENT_BASE_DIR BUILD_DIR
check=$(cd "$(dirname "$0")" && pwd)/check.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
if [ "$(uname -s)" != Darwin ]; then
    echo "The install stage runs on macOS only: its test is skipped"
    exit 0
fi
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com
export GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com
export HOME=$tmp/home
mkdir "$HOME"
# Each fake logs its arguments, a line a call, to $tmp/calls. cmake --build makes the plugin's VST3 and AU bundles
# in the build folder it is given; auval exits with $AUVAL_EXIT.
mkdir "$tmp/bin"
cat > "$tmp/bin/cmake" << 'EOF'
#!/usr/bin/env bash
echo "cmake $*" >> "$CALLS"
if [ "$1" = --build ]; then
    artefacts=$2/plugin/demo_artefacts/Release
    mkdir -p "$artefacts/VST3/Demo.vst3/Contents" "$artefacts/AU/Demo.component/Contents"
    echo new > "$artefacts/VST3/Demo.vst3/Contents/built"
    echo new > "$artefacts/AU/Demo.component/Contents/built"
fi
EOF
printf '#!/usr/bin/env bash\necho "auval $*" >> "$CALLS"\nexit "${AUVAL_EXIT:-0}"\n' > "$tmp/bin/auval"
printf '#!/usr/bin/env bash\necho "killall $*" >> "$CALLS"\n' > "$tmp/bin/killall"
chmod +x "$tmp/bin/"*
export PATH=$tmp/bin:$PATH CALLS=$tmp/calls
failures=0

repo=$tmp/plugin
mkdir "$repo"
printf '%s\n' PLUGIN_NAME=demo PRODUCT_NAME=Demo VAR_PREFIX=DEMO_ TESTS_DIR=tests PLUGIN_DIR=plugin \
    'AU_ID="aufx Dm01 Demo"' > "$repo/plugkit.conf"
git -C "$repo" init -q
git -C "$repo" add -A && git -C "$repo" commit -q -m conf
commit=$(git -C "$repo" rev-parse --short HEAD)
plugins=$HOME/Library/Audio/Plug-Ins

fail() {
    echo "FAIL: $1:" >&2
    sed 's/^/    /' <<< "$out" >&2
    failures=$((failures + 1))
}

# run [<VAR=value>...]: the install stage from the scratch plugin, its output in $out, its exit status in $status.
run() {
    : > "$CALLS"
    out=$(cd "$repo" && env "$@" "$check" install 2>&1)
    status=$?
    out=$out$'\n'"-- calls:"$'\n'$(cat "$CALLS")
}

# An older install, which the stage replaces.
mkdir -p "$plugins/VST3/Demo.vst3/Contents" "$plugins/Components/Demo.component/Contents"
touch "$plugins/VST3/Demo.vst3/Contents/stale" "$plugins/Components/Demo.component/Contents/stale"

run
[ "$status" = 0 ] || fail "install fails"
grep -q "^cmake -S \. -B build-install .*-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" "$CALLS" ||
    fail "install doesn't configure a Universal build in a folder of its own, BUILD_DIR-install"
grep -qx "cmake --build build-install --config Release --parallel --target demo_VST3 demo_AU" "$CALLS" ||
    fail "install doesn't build exactly the VST3 and AU targets"
for bundle in VST3/Demo.vst3 Components/Demo.component; do
    [ -f "$plugins/$bundle/Contents/built" ] || fail "install doesn't copy $bundle into $plugins"
    [ ! -e "$plugins/$bundle/Contents/stale" ] || fail "install leaves the older $bundle's files"
done
grep -qx "auval -v aufx Dm01 Demo" "$CALLS" || fail "install doesn't run auval on AU_ID"
[ "$(grep -n -m1 '^killall .*AudioComponentRegistrar' "$CALLS" | cut -d: -f1)" = "$(($(grep -n -m1 '^auval' "$CALLS" | cut -d: -f1) - 1))" ] ||
    fail "install doesn't restart AudioComponentRegistrar just before auval"
grep -qF "$plugins/VST3/Demo.vst3" <<< "$out" && grep -qF "$plugins/Components/Demo.component" <<< "$out" ||
    fail "install doesn't print the installed paths"
grep -qF "$commit" <<< "$out" || fail "install doesn't print the commit it built ($commit)"
uninstall=$(sed -n 's/^ *\(rm -rf .*\)$/\1/p' <<< "$out")
if [ -z "$uninstall" ]; then
    fail "install prints no rm -rf line"
else
    (eval "$uninstall")
    [ ! -e "$plugins/VST3/Demo.vst3" ] && [ ! -e "$plugins/Components/Demo.component" ] ||
        fail "the printed line ($uninstall) doesn't uninstall both bundles"
fi

run BUILD_DIR="$tmp/elsewhere"
grep -q "^cmake -S \. -B $tmp/elsewhere-install " "$CALLS" || fail "install doesn't build in BUILD_DIR-install"

run AUVAL_EXIT=1
[ "$status" != 0 ] || fail "install passes when auval fails"
grep -qF "auval" <<< "${out%%-- calls:*}" || fail "an auval failure isn't named"

printf '%s\n' PLUGIN_NAME=demo PRODUCT_NAME=Demo VAR_PREFIX=DEMO_ PLUGIN_DIR=plugin > "$repo/plugkit.conf"
run
[ "$status" != 0 ] && grep -qF "AU_ID" <<< "$out" || fail "install without AU_ID doesn't stop naming it"
! grep -q '^cmake' "$CALLS" || fail "install without AU_ID runs CMake first"

if [ "$failures" = 0 ]; then
    echo "install builds, installs over an older copy, runs auval and prints the line that uninstalls"
else
    exit 1
fi
