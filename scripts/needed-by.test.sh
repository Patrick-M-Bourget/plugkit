#!/usr/bin/env bash
# Tests scripts/needed-by.py, which checks that the README's "Needed by" column names the stages whose need lines name
# each key, against a scratch check.sh and README, then runs it on plugkit's own.
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
failures=0

cat > "$tmp/check.sh" << 'EOF'
need NAME
helper() {
    need DIR
}
first() {
    need PREFIX
    cmake --build x # a word in a command, not a call
}
second() {
    helper
}
case "${1:-all}" in
    first) first ;;
    second) second ;;
    both) first; second ;;
    all) first; second ;;
esac
EOF

# expect <pass|fail> <output text or ""> <what> <README table rows...>
expect() {
    local want=$1 text=$2 what=$3 out got=pass
    shift 3
    printf '| Key | What | Example | Needed by |\n|---|---|---|---|\n' > "$tmp/README.md"
    printf '%s\n' "$@" >> "$tmp/README.md"
    out=$(python3 -I "$here/needed-by.py" "$tmp/check.sh" "$tmp/README.md" 2>&1) || got=fail
    if [ "$got" != "$want" ] || { [ -n "$text" ] && ! grep -qF -- "$text" <<< "$out"; }; then
        echo "FAIL: $what: expected $want${text:+ saying '$text'}, got $got:" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}

name='| `NAME` | x | x | every stage |'
dir='| `DIR` | x | x | `second`, `both` |'
prefix='| `PREFIX` | x | x | `first`, `both` |'

expect pass "" "rows naming exactly the stages that need each key pass" "$name" "$dir" "$prefix"
expect pass "" "a key needed through a called function is the caller's stage's" "$name" "$dir" "$prefix"
expect fail "DIR: " "a stage that needs a key and isn't listed fails, naming the key" \
    "$name" '| `DIR` | x | x | `second` |' "$prefix"
expect fail "listed for first" "a listed stage that doesn't need the key fails" \
    "$name" '| `DIR` | x | x | `first`, `second`, `both` |' "$prefix"
expect fail "PREFIX: needed by check.sh, but has no row" "a needed key with no row fails" "$name" "$dir"
expect fail "NAME: " "a key every stage needs, listed for fewer, fails" '| `NAME` | x | x | `first` |' "$dir" "$prefix"
expect pass "" "uses in parentheses aren't compared" \
    "$name" "$dir" "$prefix" '| `OPTIONAL` | x | x | (read by `first` when set) |'
expect fail "OPTIONAL: " "a key no stage needs, listed outside parentheses, fails" \
    "$name" "$dir" "$prefix" '| `OPTIONAL` | x | x | `first` |'

# plugkit's own.
if ! out=$(python3 -I "$here/needed-by.py" "$here/check.sh" "$here/../README.md" 2>&1); then
    echo "FAIL: plugkit's README disagrees with its check.sh:" >&2
    sed 's/^/    /' <<< "$out" >&2
    failures=$((failures + 1))
fi

if [ "$failures" = 0 ]; then
    echo "The README's \"Needed by\" column agrees with check.sh's need lines"
else
    exit 1
fi
