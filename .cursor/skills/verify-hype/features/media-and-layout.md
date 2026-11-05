# Media and layout

Users paste or drop an image/video onto a slide, choose its name, and adjust fit, background or corner placement while keeping slide text.

## Sub-features

- `media-paste`: paste from preview and both text editors, name, replace or cancel.
- `media-drop`: drop one or several files and preserve original assets.
- `media-layout`: Fit/Span, background color/blur, Place and Size.
- `media-play`: video/animation preview and presentation playback.

## How to get to it (user POV)

- Ctrl+V with image/file clipboard data, drop files on the preview, or choose Media/+ Image / video.
- In the naming dialog, enter a name and press Enter, or Escape to cancel.
- Open Layout above the editor; choose Fit/Span, a background, or Place → Bottom right and Size → Small.
- Enter presentation mode; Space pauses/replays media, arrows leave the slide.
- CLI users write `![fit background=blur](photo.png)` or `![position=bottom-right size=10%](logo.png)` with files in the deck's images/videos directories.

## Driving it with verify-hype

Preconditions:

- Launch/doctor passed. The GUI cases create their own images, saved decks and clipboard payloads using the offscreen clipboard. FFmpeg and Qt Multimedia are installed.
- `visualOperations` optionally uses `trials/rails-world-2023/presentation.md`; that private trial is not a baseline fixture. If missing, trial-video coverage is unmet and must be reported.

- **Paste and cancel.** `.cursor/skills/verify-hype/verify.py gui "$RUN" media` dispatches `visualOperations` and `placeMenuPlacesImage`. `visualOperations` presses Ctrl+V on `stage`, waits for `pasteDialog`/`pasteName`, accepts a name, rejects a duplicate, and cancels with Escape. It also pastes from per-slide and document editors. Require correct media names, restored focus and preserved text in its assertions; inspect `paste-dialog.png`.
- **Understand fixture boundaries.** This existing case sets clipboard contents and naming text directly, then submits with Enter. It establishes the production paste/submit behavior, not desktop clipboard integration or typing the filename.
- **Layout supporting check.** `placeMenuPlacesImage` locates `placeMenu`/`sizeMenu` and triggers their menu items programmatically. Use its state assertions as supporting evidence; extend it with real clicks on `mediaOptionsButton`, Place and Size rows when proving the entire Layout path. Capture before/after renders and saved Markdown.
- **Prove side effects.** For a changed import journey, copy the actual written asset, resulting Markdown and a render into `$RUN/evidence` before temporary fixtures disappear, compare original asset hashes as appropriate, and reopen through the user path. A parseMedia assertion alone does not prove the written file is correct.
- **CLI layout.** Create a disposable deck and media asset under scratch, then use `.cursor/skills/verify-hype/verify.py cli "$RUN" -- check talk/presentation.md --json` and `.cursor/skills/verify-hype/verify.py cli "$RUN" -- render talk/presentation.md --slide 1 -o "$RUN/evidence/media.png" --json`. Inspect the actual layout and retain the input files.

## Gotchas

- The one-slide baseline has no media. A journey needing a real video, animation, large image or multiple files fails its prerequisite until those assets are supplied or generated locally; text-only slides cannot prove playback or conversion.
- Fit/Span backgrounds apply differently from placed images; videos do not support placement. Do not combine incompatible options to make a screenshot pass.
- The existing helper does not prove file-manager drag/drop, the Media chooser portal, compression cancellation or real desktop clipboard transfer. Record them as unverified when relevant.
- Media paste writes assets beside the deck. Cleanup must target only disposable copies.
