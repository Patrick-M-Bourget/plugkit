# Reads the plugin's settings for the check harness: plugkit.conf at the root of the plugin repository (the working
# directory), or the file PLUGKIT_CONFIG names (relative to that root). The keys, and the stages that need each, are
# in plugkit's README.md, "Per-plugin config". Sourced by scripts/check.sh, scripts/cited-tickets.sh and the git
# hooks, from the plugin repository's root.
# shellcheck shell=bash
plugkit_config=${PLUGKIT_CONFIG:-plugkit.conf}
if [ ! -f "$plugkit_config" ]; then
    echo "$PWD has no $plugkit_config: the check harness runs from a plugin repository holding its plugkit.conf (or set PLUGKIT_CONFIG); see plugkit's README.md, \"Per-plugin config\"" >&2
    exit 1
fi
# A bare name would be looked up on PATH first.
case "$plugkit_config" in
    /* | [A-Za-z]:*) source_path=$plugkit_config ;;
    *) source_path=./${plugkit_config#./} ;;
esac
# shellcheck source=/dev/null
source "$source_path"
unset source_path

# need <key>...: stops, naming every key the config leaves empty, when a stage needs them.
need() {
    local key missing=
    for key in "$@"; do
        [ -n "${!key:-}" ] || missing="$missing $key"
    done
    if [ -n "$missing" ]; then
        echo "$plugkit_config sets no$missing, which this needs; see plugkit's README.md, \"Per-plugin config\"" >&2
        exit 1
    fi
}
