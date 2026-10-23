#!/usr/bin/env bash
# Prints, one per line, the tickets a PR or the current branch will close, as gh reads them in the repository at the
# working directory. scripts/merge-on-green.sh (the PR it merges) and scripts/check.sh docs (the current branch)
# share it.
#
#   scripts/closing-tickets.sh <PR number>   the PR's closingIssuesReferences, plus each Closes/Fixes/Resolves #N
#                                            (any case, any tense, an optional colon) in its body
#   scripts/closing-tickets.sh --branch      the current branch's open PR's, as above; with no open PR, the number
#                                            the branch name starts with (<N>-...), when it names an open issue in
#                                            this repository; nothing on a detached HEAD or another branch name
#
# A branch's ticket can live in another repository (a plugkit branch for a comp1 ticket): the open-issue check is
# what keeps such a number from standing for this repository's ticket of that number. Exits non-zero only when gh
# can't read the PR or the branch's PRs.
set -uo pipefail
usage='usage: scripts/closing-tickets.sh <PR number> | --branch'

# pr_closes <PR number>
pr_closes() {
    gh pr view "$1" --json closingIssuesReferences,body -q '[.closingIssuesReferences[].number,
        (.body | scan("(?:^|[^A-Za-z])(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?):?\\s+#([0-9]+)"; "i") | .[0] | tonumber)]
        | unique | .[]'
}

case "${1:?$usage}" in
    --branch)
        branch=$(git symbolic-ref -q --short HEAD) || exit 0
        pr=$(gh pr list --head "$branch" --state open --json number -q '.[0].number // empty') || exit 1
        if [ -n "$pr" ]; then
            pr_closes "$pr"
            exit
        fi
        [[ $branch =~ ^([0-9]+)- ]] || exit 0
        ticket=$((10#${BASH_REMATCH[1]}))
        [ "$(gh issue view "$ticket" --json state -q .state 2> /dev/null)" = OPEN ] && echo "$ticket"
        exit 0
        ;;
    *[!0-9]*) echo "$usage" >&2; exit 2 ;;
    *) pr_closes "$1" ;;
esac
