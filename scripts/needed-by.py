#!/usr/bin/env python3
"""Checks that the README's "Needed by" column agrees with the keys scripts/check.sh's stages need.

Usage: needed-by.py <check.sh> <README.md>

A stage needs a key when it, or a function it calls, runs `need <key>` (scripts/config.sh); a `need` outside every
function is every stage's. The README's "Per-plugin config" table names, in a key's last column, the stages that need
it, each in backticks: `every stage` for every stage, and any other use (a stage that reads the key when it is set)
in parentheses, which aren't compared. Prints each disagreement and exits 1; exits 0 when they agree.
"""

import re
import sys

FUNCTION = re.compile(r"^([a-z_][a-z0-9_]*)\(\) *[({]\s*$")
NEED = re.compile(r"(?:^|[;&|(!]|\bthen |\bdo |\belse )\s*need((?: +[A-Z_][A-Z0-9_]*)+)")
STAGE = re.compile(r"^ {4}([a-z]+)\) (.*);;\s*$")


def calls(line, names):
    """The functions of names that line calls, at the start of a command."""
    found = set()
    for name in names:
        if re.search(r"(?:^|[;&|(!]|\bthen |\bdo |\belse )\s*" + name + r"(?=[\s;)]|$)", line):
            found.add(name)
    return found


def stage_needs(check):
    lines = open(check, encoding="utf-8").read().splitlines()
    bodies, everywhere, current = {}, set(), None
    for line in lines:
        match = FUNCTION.match(line)
        if match:
            current = match.group(1)
            bodies[current] = []
            continue
        if current is not None:
            if line in ("}", ")"):
                current = None
            else:
                bodies[current].append(line)
            continue
        for keys in NEED.findall(line.split("#")[0]):
            everywhere.update(keys.split())

    direct = {name: set() for name in bodies}
    called = {name: set() for name in bodies}
    for name, body in bodies.items():
        for line in body:
            code = line.split(" #")[0]
            for keys in NEED.findall(code):
                direct[name].update(keys.split())
            called[name] |= calls(code, bodies) - {name}

    def closure(name, seen):
        if name in seen:
            return set()
        seen.add(name)
        keys = set(direct[name])
        for other in called[name]:
            keys |= closure(other, seen)
        return keys

    stages, dispatch = {}, False
    for line in lines:
        if line.startswith('case "${1:-all}" in'):
            dispatch = True
            continue
        if dispatch:
            if line == "esac":
                break
            match = STAGE.match(line)
            if match and match.group(1) != "all":
                stages[match.group(1)] = set()
                for name in calls(match.group(2), bodies):
                    stages[match.group(1)] |= closure(name, set())
    return stages, everywhere


def readme_needs(readme, stage_names):
    rows = {}
    for line in open(readme, encoding="utf-8"):
        match = re.match(r"^\| `([A-Z_][A-Z0-9_]*)` \|.*\| ([^|]*) \|\s*$", line)
        if match:
            column = re.sub(r"\([^)]*\)", "", match.group(2))
            if column.strip() == "every stage":
                rows[match.group(1)] = "every stage"
            else:
                rows[match.group(1)] = set(re.findall(r"`([a-z]+)`", column)) & stage_names
    return rows


def main():
    check, readme = sys.argv[1:3]
    stages, everywhere = stage_needs(check)
    rows = readme_needs(readme, set(stages))
    wrong = []
    keys = everywhere | set().union(*stages.values())
    for key in sorted(keys | set(rows)):
        if key not in rows:
            wrong.append(f"{key}: needed by check.sh, but has no row in the README's table")
            continue
        if key in everywhere:
            if rows[key] != "every stage":
                wrong.append(f"{key}: check.sh needs it for every stage; the README says otherwise")
            continue
        needed = {stage for stage, needs in stages.items() if key in needs}
        listed = set() if rows[key] == "every stage" else rows[key]
        if rows[key] == "every stage" or needed != listed:
            missing, extra = sorted(needed - listed), sorted(listed - needed)
            detail = "; ".join(filter(None, [
                missing and "needed by " + ", ".join(missing) + " but not listed",
                extra and "listed for " + ", ".join(extra) + ", which don't need it",
                rows[key] == "every stage" and "listed for every stage",
            ]))
            wrong.append(f"{key}: the README's \"Needed by\" disagrees with check.sh: {detail}")
    for line in wrong:
        print(line, file=sys.stderr)
    if wrong:
        print("Make the README's \"Needed by\" column name exactly the stages whose need lines (and those of the "
              "functions they call) name the key; uses that aren't needs go in parentheses", file=sys.stderr)
        return 1
    print("The README's \"Needed by\" column names the stages that need each key")
    return 0


if __name__ == "__main__":
    sys.exit(main())
