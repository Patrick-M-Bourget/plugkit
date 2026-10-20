#!/usr/bin/env bash
# Points git at plugkit's hooks (.githooks in plugkit's checkout): pre-commit, pre-merge-commit and pre-push build and
# run the tests (.githooks/gate.sh). Run it from the plugin repository: <plugkit>/scripts/install-hooks.sh.
# Copied from eq1's scripts/install-hooks.sh (eq1 at dbb1127), pointing at plugkit's checkout; listed in eq1#231.
set -euo pipefail
hooks=$(cd "$(dirname "$0")/../.githooks" && pwd)
cd "$(git rev-parse --show-toplevel)"
git config core.hooksPath "$hooks"
echo "git hooks: $hooks"
