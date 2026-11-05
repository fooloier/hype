# CLI authoring, validation and export

Users and agents create a Markdown deck, inspect problems and slide outlines, render previews and share PDF or PowerPoint files without a display.

## Sub-features

- `cli-new`: create a themed presentation and refuse overwrite.
- `cli-inspect`: check errors/warnings, outline slides, list themes and read format help.
- `cli-render`: render one or all slides, including error banners.
- `cli-export`: PDF/PPTX, public legacy flags, and preserve existing output on failure.
- `gui-export`: export through file-menu entries or keyboard shortcuts.

## How to get to it (user POV)

- Run `hype new`, `check`, `slides`, `themes`, `help format`, `render` or `export` in a terminal; use `--json` for structured output.
- `hype <deck> --pdf <output>` and `--pptx <output>` are public export aliases.
- In the editor choose File → Export as PDF/PowerPoint, Ctrl+E or Ctrl+Shift+E. Choose palette/font toolbar controls to save appearance in front matter.

## Driving it with verify-hype

Preconditions:

- Launch/doctor passed. The deck `talk/presentation.md` has one slide titled `Verification talk`, with local paper theme colors. No media is required for this text-only journey.

- **Create.** Launch already ran the real `new` command and preserved `evidence/seed.md`. Create an additional deck with `.cursor/skills/verify-hype/verify.py cli "$RUN" -- new talk/second.md --title "Second talk" --theme paper --json`. Verify title, Markdown, and sibling images/videos directories. Repeat with `cli "$RUN" --expect-failure -- new talk/second.md --json`; expect exit 1 and unchanged file bytes.
- **Inspect.** Run `.cursor/skills/verify-hype/verify.py cli "$RUN" -- check talk/presentation.md --json`, then `cli "$RUN" -- slides talk/presentation.md --json`, `cli "$RUN" -- themes --json`, and `cli "$RUN" -- help format`. Require no problems, a slide named Verification talk, paper in the theme list, and format guidance. These abbreviated commands use the same executable helper prefix as the first command.
- **Render one.** `.cursor/skills/verify-hype/verify.py cli "$RUN" -- render talk/presentation.md --slide 1 --width 640 -o "$RUN/evidence/slide.png" --json`. Require a 640×360 PNG and visually readable title with expected colors.
- **Render all.** `.cursor/skills/verify-hype/verify.py cli "$RUN" -- render talk/presentation.md -o "$RUN/evidence/slides" --json`. Inspect `slides/slides.json`, ensure every listed image exists, and inspect the rendered content. The seeded deck has one slide.
- **Export.** Run `.cursor/skills/verify-hype/verify.py cli "$RUN" -- export talk/presentation.md "$RUN/evidence/talk.pdf" --json` and the same with `talk.pptx`. PDF must parse with one page (e.g. `pdfinfo` when available) and have visible title text when rendered. Use Python's `zipfile` and `xml.etree.ElementTree` to check the PPTX ZIP CRCs, XML validity and `ppt/presentation.xml` slide count; inspect its embedded slide image. Preserve the actual exports and inspection output. File signatures alone are insufficient.
- **Error path.** Copy the deck to a separate scratch fixture, add a missing-media reference, and retain it in evidence. `cli "$RUN" --expect-failure -- check <fixture> --json` must identify the missing media with slide/line. A broken `render --slide` returns 1 but should still emit a banner PNG. Do not break the baseline deck doctor depends on.
- **Supporting regressions.** `python -m unittest discover -s tests -p test_cli.py` and `python -m unittest discover -s tests -p test_export.py` use production CLI paths and inspect artifacts. Read their fixture prerequisites first; they do not capture retained proof automatically. Run public legacy export flags through an isolated subprocess using `environment(run)` from the helper if those aliases changed.
- **GUI export entry.** Offscreen CLI export cannot establish File → Export or Ctrl+E/Ctrl+Shift+E. Drive the native portal on the real desktop with a disposable deck, observe progress/cancellation, and inspect resulting files when those entries are in scope.

## Gotchas

- The checkout currently lacks media fixtures referenced by some Python cases (for example `tests/fixtures/animated.gif`). Report missing fixture failures precisely; do not remove expectations or describe the suite as passing.
- Text-only exports do not prove video conversion, embedding, animation flags or PowerPoint playback. Those journeys require explicit media fixtures and real side-effect inspection. Microsoft PowerPoint playback needs a receiving-app check.
- Qt file portals need desktop integration; the helper intentionally runs a windowless export journey. Native export dialogs remain unverified.
- `check` warnings may still exit 0. JSON `problems`, not exit status alone, defines success. Malformed code fences must fail export without replacing an existing file.
- Theme/font toolbar selection is a separate UI entry point. The seeded paper theme and saved colors do not establish those combo boxes work.
