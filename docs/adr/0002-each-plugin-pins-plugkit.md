# Each plugin pins plugkit to an exact version

Each plugin pins an exact plugkit tag or commit through FetchContent, never a branch: its `PLUGKIT_TAG` in `plugkit.conf` (README.md, "How a plugin runs it"). Bumping the pin is that plugin's own PR, through its full checks, including its null check. A change to plugkit therefore never changes a plugin's sound or behaviour until that plugin chooses to take it, and a regression shows up in one plugin's PR rather than in every plugin's CI at once.

## Consequences

- **What a plugin follows is plugkit at its pin.** These ADRs, `CODING_STANDARDS.md` and the check harness all bind a plugin as they read at the tag it pins, not as they read on plugkit's main. The PR that bumps the pin takes in whatever changed, and copies the new `CODING_STANDARDS.md` (README.md, "Coding standards").
- **A linked worktree checks its pin.** A worktree shares the main checkout's `.deps`, which CMake doesn't fetch again when the pin moves, so the harness checks `.deps/plugkit-src` is at `PLUGKIT_TAG` first (README.md, "How a plugin runs it", the pin check).

## Considered Options

- **Tracking plugkit's main branch.** Rejected: a library change would land in every plugin unreviewed, and the plugins' null checks would go red without anyone changing them.
- **A monorepo holding the plugins and the library.** Rejected: the plugins ship on their own schedules, and eq1 is not to change until it ships (ADR 0001).
