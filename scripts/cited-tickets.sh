#!/usr/bin/env bash
# Prints file:line:#N for each place code cites one of the given tickets: a whole #N in a tracked file under the
# plugin's SOURCE_DIRS (plugkit.conf), Markdown aside, not part of an entity (&#N), a path (a/#N), a word or a longer
# number. scripts/check.sh docs (closed tickets) and scripts/merge-on-green.sh (the tickets a PR closes) share it.
#
#   scripts/cited-tickets.sh [--at <commit>] <ticket number>...
#
# Reads the working tree's tracked files, or with --at a commit's. Run it at the plugin repository's root. Exits 0
# whether or not anything cites them, and non-zero only when git can't read the files (an unknown commit, say).
#
# Copied from eq1's scripts/cited-tickets.sh (eq1 at dbb1127), its directories made the plugin's; listed in eq1#231.
set -uo pipefail
# shellcheck source=config.sh
source "$(dirname "$0")/config.sh"
need SOURCE_DIRS
at=
if [ "${1:-}" = --at ]; then
    at=${2:?usage: scripts/cited-tickets.sh [--at <commit>] <ticket number>...}
    shift 2
fi
[ $# -gt 0 ] || exit 0
# shellcheck disable=SC2086 # one directory per word
git grep -noP '(?<![A-Za-z0-9&/_])#\d{1,4}(?![0-9A-Za-z])' ${at:+"$at"} -- $SOURCE_DIRS ':!*.md' |
    { if [ -n "$at" ]; then cut -d: -f2-; else cat; fi; } |
    awk -F: -v list="$*" 'BEGIN { n = split (list, c, " "); for (i = 1; i <= n; i++) cited["#" c[i]] = 1 }
                          ($NF in cited) { print }'
codes=("${PIPESTATUS[@]}")
# git grep exits 1 when nothing matches at all.
[ "${codes[0]}" -le 1 ] && [ "${codes[1]}" = 0 ] && [ "${codes[2]}" = 0 ]
