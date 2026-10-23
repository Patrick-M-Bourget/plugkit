#!/usr/bin/env bash
# Prints, one per line, the tickets a PR or the current branch will close, as gh reads them in the repository at the
# working directory. scripts/merge-on-green.sh (the PR it merges) and scripts/check.sh docs (the current branch)
# share it.
#
#   scripts/closing-tickets.sh <PR>        the PR's (a number, URL or branch, as gh pr view takes it)
#                                          closingIssuesReferences, plus each Closes/Fixes/Resolves #N (any case, any
#                                          tense, an optional colon) in its body
#   scripts/closing-tickets.sh --branch    the current branch's open PR's, as above but only the references to issues
#                                          in this repository; with no open PR, the number the branch name starts
#                                          with (<N>-...), when it names an open issue in this repository; nothing on
#                                          a detached HEAD or another branch name
#
# A branch's ticket can live in another repository (a plugkit branch for a comp1 ticket), so --branch counts only
# this repository's: a number names one of its issues only when gh says it does. Exits non-zero only when gh can't
# read the PR, the branch's PRs or the repository's name.
set -uo pipefail
usage='usage: scripts/closing-tickets.sh <PR> | --branch'

# pr_closes <PR> [<owner>/<name>]: with a repository, only the references to its issues
pr_closes() {
    local keep=true
    [ -z "${2:-}" ] || keep='(.repository.owner.login + "/" + .repository.name | ascii_downcase) == "'"$2"'"'
    gh pr view "$1" --json closingIssuesReferences,body -q '[(.closingIssuesReferences[] | select('"$keep"') | .number),
        (.body | scan("(?:^|[^A-Za-z])(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?):?\\s+#([0-9]+)"; "i") | .[0] | tonumber)]
        | unique | .[]'
}

case "${1:?$usage}" in
    --branch)
        branch=$(git symbolic-ref -q --short HEAD) || exit 0
        pr=$(gh pr list --head "$branch" --state open --json number -q '.[0].number // empty') || exit 1
        if [ -n "$pr" ]; then
            repo=$(gh repo view --json owner,name -q '.owner.login + "/" + .name | ascii_downcase') || exit 1
            [ -n "$repo" ] || exit 1
            pr_closes "$pr" "$repo"
            exit
        fi
        [[ $branch =~ ^([0-9]+)- ]] || exit 0
        ticket=$((10#${BASH_REMATCH[1]}))
        [ "$(gh issue view "$ticket" --json state -q .state 2> /dev/null)" = OPEN ] && echo "$ticket"
        exit 0
        ;;
    *) pr_closes "$1" ;;
esac
