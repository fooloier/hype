# Editor modes and slide organization

Users write individual slides in Visual mode, edit the document in Markdown mode, and organize slides in the Overview grid or sidebar.

## Sub-features

- `mode-toggle`: toolbar and keyboard switches preserve the previous editing mode.
- `slide-navigate`: grid/sidebar selection, arrows, row jumps and Enter.
- `slide-organize`: move, range-select, drag, duplicate, add and delete.
- `discover-controls`: slide context menu, file menu and shortcut overlay.

## How to get to it (user POV)

- Open a presentation with `hype open <file>`; `--overview` and `--markdown` choose startup modes.
- Click the grid toolbar button or press Ctrl+M; click the adjacent mode button or press Ctrl+period.
- Click a sidebar/grid slide; use arrows, Page Up/Down, Home/End and Shift selection. Enter or double-click a grid slide opens Visual mode.
- Drag slides, use Ctrl+arrows to move, Ctrl+D to duplicate, Ctrl+Enter to add, or Delete to remove. Right-click a slide for its menu.
- Click the Hype logo, press `?` or F1 for shortcuts. Click the file icon for file actions.

## Driving it with verify-hype

Preconditions:

- Launch/doctor passed. The Qt journey seeds twelve slides S1–S12 independently of the baseline CLI deck.

- **Run the journey.** `.cursor/skills/verify-hype/verify.py gui "$RUN" overview` runs `overviewMode` with verbose assertions. Require the test to pass without skips.
- **Switch modes.** The journey presses Ctrl+M/Ctrl+period and clicks `overviewButton`/`modeButton`, checking `mode`, `overviewGrid`, `thumbnails` and `editorPane` after transitions. Overview returns to the prior Visual or Markdown mode.
- **Navigate and reorder.** The journey clicks grid cells using current cell geometry, sends arrow/page/home/end keys and Ctrl+Down, and drags S1 after S2. It asserts selected indices and slide text. Some range preconditions and undo steps use model methods; these are not proof of UI range selection or UI undo.
- **Discover controls.** It right-clicks the selected grid slide, opens shortcuts through the logo and `?`, opens/closes `fileMenu` through `fileButton`, and presses Enter to return to Visual. Expect `overview-menu.png`, `shortcuts.png`, and `file-menu.png` under evidence, plus passing state assertions.
- **Check production startup.** `.cursor/skills/verify-hype/verify.py snapshot "$RUN" --overview` captures `editor.png` from the real executable on the CLI seed. Inspect it; repeat with `--markdown` for startup-mode changes, copying screenshots to distinct evidence filenames between runs.
- **Complete other entries when in scope.** `.cursor/skills/verify-hype/verify.py gui "$RUN" media` includes sidebar drag/duplicate/add in `visualOperations`, but also requires its media preconditions. Extend a QTest journey for double-click, F1, Delete, Shift selection and context-menu row activation when verifying those entry points. Capture input events and resulting state; do not infer coverage from a menu screenshot.

## Gotchas

- Arrow, Delete and duplicate behavior depends on whether `slideEditor` or `sourceEditor` has focus. Editing text changes the meaning of these keys.
- `overviewMode` invokes internal model operations for part of its setup and tests. Scope its proof to actual QTest events and observations.
- The overview fixture is an in-memory deck; it proves organization without proving disk persistence. Use the writing journey for saved-file evidence.
- Context-menu and file-menu screenshots prove opening only. Native file dialogs and individual menu actions are separate journeys.
