# Presenting

Users start fullscreen slides from the selection or beginning, navigate, control playback, and see notes on a separate presenter display.

## Sub-features

- `present-toggle`: selected-slide fullscreen and return to editing.
- `present-start`: start/restart at slide one.
- `present-navigate`: keyboard and click navigation, media pause/replay.
- `presenter-view`: audience screen, current/next previews and speaker notes.

## How to get to it (user POV)

- Click Present, press Ctrl+Space or F5 to toggle from the current slide.
- Press Shift+F5 to start from the beginning; arrows and slide clicks advance, Home/End jump.
- Space controls videos/animations; Escape returns to editing.
- With an external display attached, Presenter View appears on the laptop and audience slides on the external display. Navigate from either window. Screen changes end the presentation.

## Driving it with verify-hype

Preconditions:

- Launch/doctor passed. The helper selects the repository's two-screen offscreen fixture for this journey.
- `presenterViewUsesSeparateScreens` requires `tests/fixtures/animated.webp`, which is absent in this checkout. This journey currently fails that prerequisite; restore or generate the required three-frame animation before running it. Keep the expected animation assertions intact.

- **Run two-screen behavior.** `.cursor/skills/verify-hype/verify.py gui "$RUN" presenter` runs `presenterViewUsesSeparateScreens` with verbose assertions. Require a pass and no skips.
- **Start and inspect.** The test sends F5, observes audience/presenter window visibility and screen placement, and inspects `presenterNotes`. Inspect the command evidence for the expected two-screen assertions.
- **Navigate and leave.** It sends Right, Shift+F5, End, Space and Escape through the presenter window and checks selected slides, animation state and return to editing. Record which actions actually run; setup model calls prove no UI entry point.
- **Cover other entries.** The existing journey does not establish the Present toolbar button, Ctrl+Space, audience-slide clicks, real hotplug or actual video output. Extend QTest input on `presentButton`/`slideFrame` and capture window images for those entries. Use real desktop displays for monitor placement/hotplug claims, with a disposable deck and isolated settings.

## Gotchas

- One offscreen screen cannot satisfy Presenter View preconditions. Do not replace the two-screen expectation with a single-window pass.
- The virtual screen fixture proves window-routing logic. Physical connector discovery, compositor fullscreen behavior, GPU rendering and laptop/external-display placement remain unverified.
- Notes are HTML comments; directives and fenced code are not speaker notes. Preserve a known comment in a notes journey.
- The baseline CLI deck contains no media or notes. A desktop playback/notes journey must seed them first and retain the fixture with its evidence.
