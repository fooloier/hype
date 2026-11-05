# Hype verification map

Read this index before a drive. Each feature records user entry points, existing harness coverage, expected results, and gaps. An alternate entry point is not proof of an unexercised one.

## Baseline preconditions

- Follow `../SKILL.md` Launch and set `RUN` to its printed absolute directory.
- Require doctor success, a usable editor screenshot, the `paper` theme and `scratch/talk/presentation.md` titled `Verification talk`.
- Use one run per concurrent drive. Qt fixtures are disposable; never open a personal deck for these recipes.
- Call the executable helper as `.cursor/skills/verify-hype/verify.py` from the repository root.

## Driving conventions

- `gui` dispatches the existing Qt Test cases with real `Main.qml`, QTest events and object-name lookups. `cli` dispatches the built production executable.
- Relative CLI paths resolve under `$RUN/scratch`. Put permanent output under `$RUN/evidence`.
- Qt assertions are logged with `-v2`; a skip fails the helper. Setup and internal setter calls are fixture work, not user-path proof.
- Capture the action, resulting UI state, and file/package side effects. Inspect screenshots visually.
- Run cleanup on every attempt; retain all evidence, including failures. Missing fixtures produce failed or unverified journeys with reasons.

## Features

- [Editor modes and slide organization](editor-modes.md): Visual/Markdown/Overview, navigation, reorder, menus and shortcuts.
- [Writing, saving and recovery](writing-and-saving.md): text input, formatting, autosave, version history and external changes.
- [Media and layout](media-and-layout.md): paste, replacement, placement, backgrounds and playback.
- [Presenting](presenting.md): fullscreen, navigation, speaker notes and two-screen behavior.
- [CLI authoring, validation and export](cli-and-export.md): create, theme, check, outline, PNG, PDF and PowerPoint.

Each feature uses exactly four H2s: Sub-features; How to get to it (user POV); Driving it with verify-hype; Gotchas. Keep entry points and unmet prerequisites current with `/maintain-verification-skill`.
