#!/usr/bin/env bash
# Tests scripts/check.sh docs' ticket-citation rules, the closed tickets and the ones the current branch will close
# (scripts/closing-tickets.sh --branch), in a scratch plugin repo against a fake gh on PATH: no GitHub is read.
# Run: scripts/check.sh hooks (by hand, or in a full check; never from a git hook). Needs jq.
#
# The fake reads the repository's tickets from files in $FAKE: closed and open (one issue number per line), pr (the
# number of the current branch's open PR, empty for none), and that PR's closes (JSON array of numbers, its
# closingIssuesReferences) and body. With $FAKE/signed-out, every call fails, as gh does signed out.
set -uo pipefail
plugkit=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" << 'EOF'
#!/usr/bin/env bash
set -uo pipefail
[ ! -e "$FAKE/signed-out" ] || { echo "To get started with GitHub CLI, please run: gh auth login" >&2; exit 4; }
query=. state=
args=("$@")
for ((i = 0; i < ${#args[@]}; i++)); do
    case ${args[i]} in
        -q | --jq) query=${args[i + 1]} ;;
        --state) state=${args[i + 1]} ;;
    esac
done
case "$1 $2" in
    "issue list") jq -R 'tonumber' "$FAKE/$state" | jq -s 'map({number: .})' | jq -r "$query" ;;
    "issue view")
        if grep -qx "$3" "$FAKE/open"; then s=OPEN; elif grep -qx "$3" "$FAKE/closed"; then s=CLOSED; else
            echo "GraphQL: Could not resolve to an issue or pull request with the number of $3." >&2
            exit 1
        fi
        jq -n --arg s "$s" '{state: $s}' | jq -r "$query"
        ;;
    "pr list") jq -R 'tonumber' "$FAKE/pr" | jq -s 'map({number: .})' | jq -r "$query" ;;
    "pr view")
        [ "$3" = "$(cat "$FAKE/pr")" ] || { echo "no pull requests found for $3" >&2; exit 1; }
        jq -n --argjson c "$(cat "$FAKE/closes")" --arg b "$(cat "$FAKE/body")" \
            '{closingIssuesReferences: $c | map({number: .}), body: $b}' | jq -r "$query"
        ;;
    *) echo "fake gh: unexpected call: $*" >&2; exit 99 ;;
esac
EOF
chmod +x "$tmp/bin/gh"

repo=$tmp/repo
mkdir -p "$repo/engine"
cd "$repo" || exit 1
git init -q -b main .
git config user.email test@example.com
git config user.name test
printf 'PLUGIN_NAME=demo\nVAR_PREFIX=DEMO_\nSOURCE_DIRS=engine\nTESTS_DIR=engine\n' > plugkit.conf
cp "$plugkit/CODING_STANDARDS.md" .
printf '// Holds.\n' > engine/a.cpp
git add . && git commit -qm one

failures=0
run=0
# on <branch> <code in engine/a.cpp>: checks out a fresh fake repository's branch, with tickets 3 closed and 12
# and 13 open, no PR, and that code
on() {
    run=$((run + 1))
    export FAKE=$tmp/gh$run
    mkdir "$FAKE"
    printf '3\n' > "$FAKE/closed"
    printf '12\n13\n' > "$FAKE/open"
    : > "$FAKE/pr"
    echo "[]" > "$FAKE/closes"
    echo "Does a thing." > "$FAKE/body"
    git checkout -q -B "$1"
    printf '%s\n' "$2" > engine/a.cpp
    git add engine/a.cpp
}
# expect <exit code> <output pattern> <description>
expect() {
    local code=$1 pattern=$2 what=$3 got
    out=$(PATH="$tmp/bin:$PATH" "$plugkit/scripts/check.sh" docs 2>&1)
    got=$?
    if [ "$got" != "$code" ] || ! grep -qE -- "$pattern" <<< "$out"; then
        echo "FAIL: $what: expected exit $code and /$pattern/, got exit $got:" >&2
        sed 's/^/    /' <<< "$out" >&2
        failures=$((failures + 1))
    fi
}
closes_msg='^engine/a.cpp:1: points at #12, which this branch closes; say what the code does now$'

on 12-a-thing '// Holds until #12 lands.'
expect 1 "$closes_msg" "a branch named for an open ticket, with no PR, can't cite it"

on 012-a-thing '// Holds until #12 lands.'
expect 1 "$closes_msg" "a branch name's leading zeros name the same ticket"

on 12-a-thing '// Holds.'
expect 0 "^Code points at no closed ticket, nor at one this branch closes \(#12\)$" \
    "a branch named for an open ticket passes when nothing cites it"

on 14-a-thing '// Holds until #14 lands.'
expect 0 "^Code points at no closed ticket$" \
    "a branch whose leading number is no issue here (its ticket in another repository) doesn't count it"

on 3-a-thing '// Holds.'
expect 0 "^Code points at no closed ticket$" "a branch whose leading number is a closed issue doesn't count it"

on topic '// Holds until #12 lands.'
echo 7 > "$FAKE/pr"
echo "[12]" > "$FAKE/closes"
expect 1 "$closes_msg" "a branch whose PR closes a ticket can't cite it"

on topic '// Holds until #12 lands.'
echo 7 > "$FAKE/pr"
printf 'Does a thing.\n\nFixes: #12\n' > "$FAKE/body"
expect 1 "$closes_msg" "a branch whose PR body's Fixes line closes a ticket can't cite it"

on topic '// Holds until #13 lands.'
echo 7 > "$FAKE/pr"
echo "[12]" > "$FAKE/closes"
expect 0 "^Code points at no closed ticket, nor at one this branch closes \(#12\)$" \
    "a branch whose PR closes a ticket passes when nothing cites it"

on 13-a-thing '// Holds until #13 lands.'
echo 7 > "$FAKE/pr"
echo "[12]" > "$FAKE/closes"
expect 0 "^Code points at no closed ticket, nor at one this branch closes \(#12\)$" \
    "an open PR's tickets stand for the branch's, not the number its name starts with"

on main '// Holds until #12 lands.'
expect 0 "^Code points at no closed ticket$" "main counts no ticket as one it closes"

on main '// Held until #3 landed.'
expect 1 "^engine/a.cpp:1: points at closed ticket #3; say what the code does now, or point at an open ticket$" \
    "main still refuses a closed ticket"

on 12-a-thing '// Holds until #12 lands.'
git checkout -q --detach
expect 0 "^Code points at no closed ticket$" "a detached HEAD counts no ticket as one it closes"

on 12-a-thing '// Holds until #12 lands.'
touch "$FAKE/signed-out"
expect 0 "^Skipped the closed-ticket check: gh isn't signed in$" "signed out, the ticket checks are skipped"

if [ "$failures" = 0 ]; then
    echo "check.sh docs refuses code citing a closed ticket or one the branch closes, and only those"
else
    exit 1
fi
