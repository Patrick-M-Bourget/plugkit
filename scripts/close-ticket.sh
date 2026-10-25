#!/usr/bin/env bash
# Closes a ticket with gh issue close, unless origin/main's code still cites it: closed, that citation would point at
# a closed ticket, which scripts/check.sh docs refuses on main. merge-on-green.sh guards the tickets a PR closes; this
# guards one closed by hand. It fetches origin first, then lists each file:line citing the ticket and refuses with
# "NOT CLOSED #N: code cites it; ...". The rule for a citation is scripts/cited-tickets.sh's.
#
#   scripts/close-ticket.sh <ticket number> [--comment "<text>"]
#
# The comment goes to gh issue close as it is. Run it at the plugin repository's root; it closes the ticket in the
# repository gh reads from there. It exits with gh's code, or 1 when it refuses or can't read origin/main.
set -uo pipefail
usage='usage: scripts/close-ticket.sh <ticket number> [--comment "<text>"]'
ticket=${1:-}
[[ $ticket =~ ^[0-9]+$ ]] || { echo "$usage" >&2; exit 2; }
close=(gh issue close "$ticket")
if [ $# -gt 1 ]; then
    [ $# = 3 ] && [ "$2" = --comment ] || { echo "$usage" >&2; exit 2; }
    close+=(--comment "$3")
fi

if ! git fetch -q origin main; then
    echo "NOT CLOSED #$ticket: could not fetch origin/main to read its files"
    exit 1
fi
if ! cited=$("$(dirname "$0")/cited-tickets.sh" --at origin/main "$ticket"); then
    echo "NOT CLOSED #$ticket: could not read origin/main's files"
    exit 1
fi
if [ -n "$cited" ]; then
    sed -E 's/:#[0-9]+$//' <<< "$cited"
    echo "NOT CLOSED #$ticket: code cites it; say what the code does now, or point at an open ticket"
    exit 1
fi
"${close[@]}"
