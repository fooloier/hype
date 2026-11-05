# Writing, saving and recovery

Users type Markdown, apply formatting, save automatically or immediately, and recover earlier presentation versions without losing following slides.

## Sub-features

- `write-slide`: per-slide text and complete-document editing.
- `format-text`: toolbar and Ctrl+B/I/U/H/K/slash formatting.
- `save`: autosave after a pause, periodic save during typing, Ctrl+S and Save as.
- `recover`: reopen drafts, restore version history, and follow external file edits.

## How to get to it (user POV)

- Open a deck and type in the editor below the preview. Ctrl+period opens whole-document Markdown mode.
- Select text and choose Bold, Italic, Underline, Headline, Code or Note in the formatting bar, or use the corresponding shortcut.
- Pause typing for autosave, press Ctrl+S, or choose Save/Save as from the file menu. Ctrl+N starts a new deck; Ctrl+O opens another.
- Choose Version history from the file menu to restore a snapshot. Reopen after an interrupted session for draft recovery. Edit the same file externally to see it update.

## Driving it with verify-hype

Preconditions:

- Launch/doctor passed. Existing editing cases create named temporary decks and private recovery storage.

- **Type and autosave.** `.cursor/skills/verify-hype/verify.py gui "$RUN" editing` runs `autosavePreservesSlideEditorCursor` and `typingCodeInVisualEditorKeepsTheDeck`. The former focuses `slideEditor`, types `x` and trailing whitespace, waits for autosave and verifies text/cursor and saved content; all data rows must pass. The latter types a Rust fence through key events while preserving following slides, then presses Ctrl+S.
- **Format through the UI.** The code-typing case clicks `boldButton` for selected text and `source.boldButton` in full-document mode, checking resulting Markdown and selected text. Text injected with `setProperty` and selection through an internal method are fixture work; they do not prove typing or mouse selection.
- **Preserve proof.** Review the verbose command record for each typed action and saved-file assertion. If verifying new persistence behavior, extend the journey to copy its resulting Markdown and recovery snapshots into `$RUN/evidence` before Qt's temporary directory exits; reopen through the UI or use the public CLI to inspect the saved result.
- **History and external-change follow-up.** Existing `autosaveAndVersionHistory`, `recoversIncompleteDraftAndSlideBoundaries`, `externalEditsKeepTheCurrentSlide` and `historyPopupProtectsSlides` in `tests/tests.cpp` provide supporting checks. They use model methods for mutations/restoration and cannot prove the menu-to-restore user path. Add a QTest journey that clicks `fileButton`, selects `Version history…`, selects a real version in `historyDialog`, restores, and reopens the deck when that feature changes.

## Gotchas

- Unfinished fences are recoverable drafts; they must not overwrite the last valid file. Finish them before export.
- Backups live beside the named presentation in `.hype-backups/`; recovery snapshots contain Markdown and slide boundaries, not media copies.
- Save as/Open/first save require a native file portal. Offscreen tests do not establish desktop portal behavior; that entry point remains unverified until driven on the desktop with a disposable deck.
- The helper does not currently automate menu-to-history restore, crash/reopen, New/Open dialogs, every formatting shortcut, or continuous-typing autosave. Report these coverage gaps when relevant; a lower-level pass is supporting evidence only.
