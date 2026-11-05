---
name: verify-hype
description: Verify Hype's native Qt presentation editor and headless CLI after editing, navigation, media, presentation, or export changes; launch isolated runs and retain screenshots and command evidence.
---

# Verify Hype

Hype's primary surface is the native Qt Quick editor. Its secondary surface is the display-free `new/check/slides/render/export/themes` CLI. Reuse `tests/tests.cpp` for Qt input and object-name discovery; this app has no browser or CDP endpoint. Read [the feature index](features/README.md) and the relevant journeys before driving it.

## Launch

Run from the repository root. Dependencies are listed in `pkgbuild/PKGBUILD`: a C++17 compiler, make, git, patch, Qt 6.9+ with Quick/Multimedia/SVG, tinyxml2, zlib, libwebp, FFmpeg and source-highlight. Python 3 runs the helper using only its standard library. No credentials or database are needed.

```sh
.cursor/skills/verify-hype/verify.py launch
```

This runs the documented `bin/build`, builds the existing Qt Test target with the same qmake, creates a unique run under `build/verification/`, seeds the local `paper` theme, and uses the real `hype new` command to create `scratch/talk/presentation.md`. It captures the actual editor via `hype open ... --screenshot` and exits. Readiness is the final `READY: /absolute/run/path` message, a successful doctor, and `evidence/launch.png` showing a usable single-slide editor. A failed launch cleans its scratch directory and leaves its logs.

Set `RUN` to the exact `RUN=` path printed by launch. Subsequent examples use that shell variable.

The first build can clone pinned MicroTeX from GitHub. `HYPE_QMAKE` and `HYPE_MICROTEX_SOURCE` are honored by the build. It must use the pinned clean source accepted by `bin/build-math`. Build errors belong in the evidence; repair or report them before claiming verification.

Each command launches and waits for its own child process; there is no server left running. GUI drives use the real `Main.qml` and production components inside Qt Test, following the repository's existing harness. The initial screenshot runs the actual `build/hype` executable. For a manual desktop session the production command is `./build/hype open <disposable-deck.md>`; apply the helper's isolated environment before using it.

Run state uses private XDG config/state/data/cache/runtime directories, a seeded `OMARCHY_PATH`, and offscreen software rendering. The helper preserves HOME and the user's fonts/toolchain; Hype also reads themes from `~/omarchy/themes` and `~/.config/omarchy/themes`. Extra themes may appear, so choose `paper` by name. Commands that install skills or desktop launchers are outside this workflow.

Build products and math resources remain shared in `build/`. Launches serialize their builds using `build/verification/build.lock`; avoid concurrent source edits or external builds during a drive. Two drives can use distinct run directories; serialize commands within one run. The helper never attaches to the user's desktop window or clipboard. No network service ports are required.

## Doctor

Whenever anything looks wrong, first run this read-only application check:

```sh
.cursor/skills/verify-hype/verify.py doctor "$RUN"
```

It compares SHA-256 hashes of both executables with `run.json`, runs `--version`, checks the seeded presentation with `check --json`, and confirms `paper` is discoverable. It writes command evidence, but does not mutate presentation state. `Healthy isolated run:` is success. A cleaned run, changed executable, missing dependency, or broken deck is failure; launch a fresh run after fixing it. It does not certify native portals, desktop graphics, or actual monitors.

## Drive

```sh
.cursor/skills/verify-hype/verify.py gui "$RUN" overview
.cursor/skills/verify-hype/verify.py cli "$RUN" -- slides talk/presentation.md --json
.cursor/skills/verify-hype/verify.py snapshot "$RUN" --overview
```

`gui` supports `overview`, `editing`, `media`, and `presenter`. Each runs explicitly named existing tests with `HYPE_GUI_TESTS=1` and verbose assertions, rejects skips, and records the result. `presenter` uses `tests/fixtures/presenter-screens.json` for two virtual displays. Read the feature recipes for what these tests actually exercise and what still requires a new user journey. A test that calls internal setters proves only the input-driven portions and read-only observations around them.

`cli` forwards actual public commands, logs argv/stdout/stderr/exit status, and expects exit 0. Prefix arguments with `--expect-failure --` for an expected exit 1. Paths are relative to `$RUN/scratch`; use absolute `$RUN/evidence/...` destinations to preserve outputs. `snapshot` runs the actual editor with `--screenshot`, optionally `--slide N`, `--markdown`, or `--overview`, and exits after about 1.8 seconds. Inspect screenshots: a PNG existing does not prove correct rendering.

Stable GUI handles include `overviewButton`, `modeButton`, `overviewGrid`, `thumbnails`, `slideEditor`, `sourceEditor`, `fileButton`, `fileMenu`, `presentButton`, and `deckSummary`. The tests resolve Qt `objectName`s, map item centers into window coordinates, and use `QTest` keyboard/mouse events. Prefer this to hardcoded screen coordinates. Read `src/Main.qml` and the matching test before extending a drive; never substitute `Deck::editSlide`, `setProperty("text", ...)`, or an internal export snapshot for the user action under investigation.

## Evidence

Artifacts live at `$RUN/evidence/`; `run.json` records checkout revision and executable hashes. Each `command-NNN.json` records the command, working directory, stdout, stderr, expected and actual exit code. Qt `-v2` logs preserve actions' assertions and resulting state. Overview also captures the slide context menu, shortcuts overlay, and file menu. Keep input Markdown, screenshots, rendered slides, exports, and read-only checks of side effects in this directory before cleanup. Qt Test temporary fixtures disappear when the tests finish; retain any additional fixtures needed to prove a mutation.

Proof requires exercising the real user entry point, recording the action and resulting state, and checking side effects alongside the visible result. Reopen saved files or inspect exported packages; a toast, screenshot, signature, or exit code alone is insufficient. Cover each entry point listed in the selected map; report unexercised paths as unverified. Seed fixtures establish preconditions, never replace the action being proved. Mocks are appropriate only at an existing production boundary to an external system, with the mocked boundary stated.

There is no dry-run guarantee in this recipe: `new`, rendering, autosave, and export really write files, and builds can fetch dependencies. If introducing a dry-run journey, observe files, network activity and git refs (and browser launches if relevant) to establish what is actually skipped.

Offscreen verification proves Qt behavior with software rendering. Desktop GPU behavior, real fullscreen placement, native file portals, the installed launcher, and playback in Microsoft PowerPoint require their own target runs. The stand-in creates a private XDG runtime, a synthetic paper theme, a CLI-seeded deck, Qt Test fixtures and optionally virtual screens; these do not verify desktop installation, actual monitor discovery, a user's theme inventory, or real clipboard/portal integration. Missing media or private trial decks are failed preconditions for those journeys, not permission to drop assertions.

## Cleanup

After success or any failed drive:

```sh
.cursor/skills/verify-hype/verify.py cleanup "$RUN"
```

This removes only the run's `scratch/` state and writes `cleanup.json`; evidence and build products survive. Child processes normally exit before each command returns. Timeouts/interruption terminate only the subprocess group created for that command. Never kill Hype by process name. If the helper itself was force-killed, inspect its command log and exact child PID before stopping any leftover process.

After cleanup, use the file tools to confirm `$RUN/evidence/launch.png`, command logs, and the feature's proof still exist, and that `scratch/` is absent. Read `cleanup.json` for the retained list. A cleanup that loses evidence invalidates the run.

## Helpers

The executable helper is [verify.py](verify.py). Invocation is `.cursor/skills/verify-hype/verify.py <action> ...`; `--help` lists actions. The recipes above show launch, doctor, GUI, CLI, snapshot, and cleanup. Update the feature map as behavior changes; `/maintain-verification-skill` provides the maintenance workflow.
