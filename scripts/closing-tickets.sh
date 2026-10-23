#!/usr/bin/env bash
# Prints, one per line, the tickets a PR will close, as gh reads them in the repository at the working directory.
# scripts/merge-on-green.sh (the PR it merges) uses it.
#
#   scripts/closing-tickets.sh <PR number>   the PR's closingIssuesReferences, plus each Closes/Fixes/Resolves #N
#                                            (any case, any tense, an optional colon) in its body
#
# Exits non-zero only when gh can't read the PR.
set -uo pipefail
usage='usage: scripts/closing-tickets.sh <PR number>'

# pr_closes <PR number>
pr_closes() {
    gh pr view "$1" --json closingIssuesReferences,body -q '[.closingIssuesReferences[].number,
        (.body | scan("(?:^|[^A-Za-z])(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?):?\\s+#([0-9]+)"; "i") | .[0] | tonumber)]
        | unique | .[]'
}

case "${1:?$usage}" in
    *[!0-9]*) echo "$usage" >&2; exit 2 ;;
    *) pr_closes "$1" ;;
esac
