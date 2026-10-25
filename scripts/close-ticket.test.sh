#!/usr/bin/env bash
# Tests scripts/close-ticket.sh against a fake gh on PATH (no ticket is closed), in a scratch plugin repo whose
# origin is a local bare repo. origin/main's engine/a.cpp cites #12. The clone's working tree cites #34 (uncommitted),
# which origin/main doesn't; a commit pushed to origin after the clone cites #56, which only a fetch sees.
# The fake gh logs each call to gh.log, one argument per line, and exits with gh.exit's code (0 when it's missing).
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook).
set -uo pipefail
script=$(cd "$(dirname "$0")" && pwd)/close-ticket.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" << 'EOF'
#!/usr/bin/env bash
printf '%s\n' "$@" "--" >> "$FAKE/gh.log"
exit "$(cat "$FAKE/gh.exit" 2> /dev/null || echo 0)"
EOF
chmod +x "$tmp/bin/gh"
export FAKE=$tmp

git init -q --bare "$tmp/origin.git"
git -C "$tmp/origin.git" symbolic-ref HEAD refs/heads/main
commit() { git -c user.name=t -c user.email=t@t "$@"; }
git init -q -b main "$tmp/seed"
mkdir "$tmp/seed/engine"
echo 'SOURCE_DIRS="engine plugin tools tests"' > "$tmp/seed/plugkit.conf"
printf '// Holds until #12 lands.\n// So does this (#12).\n' > "$tmp/seed/engine/a.cpp"
git -C "$tmp/seed" add .
commit -C "$tmp/seed" commit -qm cites
git -C "$tmp/seed" push -q "$tmp/origin.git" main
git clone -q "$tmp/origin.git" "$tmp/repo"
printf '// Waits on #34.\n' > "$tmp/repo/engine/b.cpp"
git -C "$tmp/repo" add engine/b.cpp
printf '// Waits on #56.\n' > "$tmp/seed/engine/c.cpp"
git -C "$tmp/seed" add .
commit -C "$tmp/seed" commit -qm later
git -C "$tmp/seed" push -q "$tmp/origin.git" main
cd "$tmp/repo" || exit 1

failures=0
fail() {
    echo "FAIL: $1" >&2
    failures=$((failures + 1))
}
# expect <exit code> <expected output> <expected gh.log> <description> <script arguments...>
expect() {
    local code=$1 want=$2 calls=$3 what=$4 out got
    shift 4
    rm -f "$tmp/gh.log"
    out=$(PATH="$tmp/bin:$PATH" "$script" "$@" 2>&1)
    got=$?
    if [ "$got" != "$code" ] || [ "$out" != "$want" ]; then
        fail "$what: expected exit $code and:"
        sed 's/^/    /' <<< "$want" >&2
        echo "  got exit $got and:" >&2
        sed 's/^/    /' <<< "$out" >&2
    fi
    if [ "$(cat "$tmp/gh.log" 2> /dev/null)" != "$calls" ]; then
        fail "$what: expected gh calls:"
        sed 's/^/    /' <<< "$calls" >&2
        echo "  got:" >&2
        sed 's/^/    /' "$tmp/gh.log" >&2 2> /dev/null
    fi
}

refusal="NOT CLOSED #%s: code cites it; say what the code does now, or point at an open ticket"
# shellcheck disable=SC2059 # the format is the refusal
expect 1 $'engine/a.cpp:1\nengine/a.cpp:2\n'"$(printf "$refusal" 12)" "" \
    "a ticket origin/main's code cites isn't closed, and each citation is listed" 12
expect 0 "" $'issue\nclose\n77\n--' "a ticket nothing cites is closed" 77
expect 0 "" $'issue\nclose\n77\n--comment\nDone in #80.\n--' "the comment is passed through" 77 --comment "Done in #80."
expect 0 "" $'issue\nclose\n34\n--' "the working tree's citations don't count, only origin/main's" 34
# shellcheck disable=SC2059
expect 1 $'engine/c.cpp:1\n'"$(printf "$refusal" 56)" "" "origin/main is fetched first" 56

echo 4 > "$tmp/gh.exit"
expect 4 "" $'issue\nclose\n77\n--' "gh's failure is the script's" 77
rm "$tmp/gh.exit" "$tmp/gh.log"

for args in "" "12x" "12 --comment" "12 --bogus x"; do
    # shellcheck disable=SC2086 # one argument per word
    PATH="$tmp/bin:$PATH" "$script" $args > /dev/null 2>&1 && fail "bad arguments ($args) should fail"
done
out=$(PATH="$tmp/bin:$PATH" "$script" 77 --comment "" 2>&1) && fail "an empty comment should fail"
grep -q "^usage: " <<< "$out" || fail "an empty comment doesn't print the usage: $out"
[ -e "$tmp/gh.log" ] && fail "bad arguments called gh"

# shellcheck disable=SC2059
want=$'engine/a.cpp:1\nengine/a.cpp:2\n'"$(printf "$refusal" 12)"
out=$(cd engine && PATH="$tmp/bin:$PATH" "$script" 12 2>&1) && fail "run from a subdirectory, a cited ticket was closed"
[ "$out" = "$want" ] || fail "run from a subdirectory, it doesn't read the whole repository: $out"
rm -f "$tmp/gh.log"

git remote set-url origin "$tmp/missing.git"
out=$(PATH="$tmp/bin:$PATH" "$script" 77 2>&1) && fail "an unreachable origin should fail, not close the ticket"
grep -q "NOT CLOSED #77: could not fetch origin/main" <<< "$out" || fail "an unreachable origin doesn't say so: $out"
[ -e "$tmp/gh.log" ] && fail "an unreachable origin called gh"

if [ "$failures" = 0 ]; then
    echo "close-ticket.sh closes a ticket only when origin/main's code doesn't cite it"
else
    exit 1
fi
