# What the build reads, for the hooks' check that the tree they build and test is the one committed or
# pushed: the plugin's BUILD_INPUTS (plugkit.conf). Sourced by pre-commit and pre-push, at the plugin repository's
# root, with hooks_dir set to this folder; keep BUILD_INPUTS to paths that exist.
# Copied from eq1's .githooks/build-inputs.sh (eq1 at dbb1127), its list made the plugin's; listed in eq1#231.
# shellcheck shell=bash
# shellcheck source=../scripts/config.sh
source "$hooks_dir/../scripts/config.sh"
need BUILD_INPUTS
# shellcheck disable=SC2034 # used by the hooks that source it
build_inputs=("${BUILD_INPUTS[@]}")
