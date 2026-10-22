# User constraints

- 2026-10-23: Pending UI Preferences.txt is authorized and implemented. User clarified maximize click shows a small box but does nothing; route HTMAXBUTTON clicks into normal Qt client mouse/QML handling, without independent native capture/direct release toggle. Preserve Snap hover. Top and bottom bars use about70% opaque neutral gray (about30% transparent); other menus retain their existing opacity. Default no-file Home is560x360. User requested replacement deployment, obsolete executable removal and GitHub push. No additional broad benchmarks.

- Final Pass.md is now authorized for implementation: Escape globally pauses and minimizes (including menus/text focus), without auto-resuming on restore; playback arrow seek/volume keys do not reveal hidden controls. Mouse movement reveals controls. Fix native maximize/restore press capture and preserve restore geometry. Bottom UI shrinks another20%, cumulative scale0.64. This supersedes earlier Esc/fullscreen behavior and bottom-bar scale0.8. Background validation must suppress actual desktop minimize/resize; isolate logs with FLOW_LOG_DIRECTORY while a user player is open.

- App branding is now authorized: use assets/flowplayer.png and the multi-resolution assets/flowplayer.ico. The user supplied a clapperboard/reel icon and requested removal of the circular reel. Keep only the clapperboard with alpha transparency; no IINA logo. Source edit/prompt recorded in assets/README.md.

- Latest UI clarification: all UI surfaces, explicitly title bar and Home, use neutral iOS-style translucent gray from UiPalette.qml. Not blue-gray; blue only for active accents. Reference design/references/neutral-gray-material.png supplies colors, not Wi-Fi features. Bottom controls are 20% smaller; fullscreen immediately left of More; cursor menus open down/scroll-to-fit, bottom-button menus open up. Earlier colors and four-action limits are superseded.

- Part 5 is now authorized, saved as prompt5.md. This supersedes every earlier Part 5 deferral below. Keep testing practical, muted and non-activating; visual acceptance remains manual.

- Latest audit/input/UI preferences are locked in design/interaction-preferences.md. Previous/next media-file navigation is now authorized as a scoped addition; do not build the full Part 5 playlist/auto-advance. Implement resize-driven compact transitions and the grave/backtick key. Preserve native caption dragging.

- Part 4 is now authorized and saved as prompt4.md. Earlier deferrals of Part 4 below are superseded. The playlist panel/folder navigation remain Part 5; do not start Part 5. Keep history databases and thumbnail caches isolated in automated tests.

- All future tests must be muted and run in the background while the user works.
- Never activate test windows, steal focus, or automatically minimize/restore windows on the user's desktop. Tests requiring visible interaction remain manual unless the user explicitly authorizes a dedicated session.
- Keep validation bounded. Use one short reproduction for a specific hypothesis; freeze changes before expensive acceptance runs. Do not enter repeated full benchmark/tuning loops.
- Part 4 is explicitly authorized by the user. Part 1's remaining VP9 measurement and Part 3 resume-drop regression are documented; do not claim them fixed without evidence. Do not begin Part 5.
- Preserve every locked architecture and technology decision in master prompt.md.
- Locked visual references for the later UI part are saved in design/references/: player-controls.png (user Image 1, player UI) and welcome-page.png (user Image 2, welcome page). Read design/references/README.md and inspect both originals when implementing that part. Do not start UI implementation until authorized.
- Match those visual references as closely as possible, but do not use or copy the IINA logo. FlowPlayer will use a different, original logo; logo creation is deferred until requested.
- User revision on 2026-10-18: resident RAM below 500 MB is accepted. This overrides the original 150/300 MB budgets. Do not continue RAM profiling or ask for elevated heap tracing.
- User reported all five manual checks passed on 2026-10-18; evidence is in tests/manual-results.json. Preserve their reported status.
- User explicitly accepted a first-deployment-only startup exception on 2026-10-18. Normal fully closed launches must remain under 500 ms; retain measured initial timing in reports.
- Part 2 testing revision: automate state-machine unit tests and minimal muted, non-activating basic checks only. User performs visual playback, fullscreen, tearing, dragging, resizing and audible-continuity checks. Never substitute screenshot claims for unperformed manual tests.
- User revision on 2026-10-18: Mini hides only the video; video/audio playback and decoding continue normally. This supersedes Part 2's vid=no requirement. Keep the title buttons (including minimize/restore/close), always-on-top behavior and bottom playback/seek controls available in Mini. Favor smooth instant hide/restore over decoder resource savings.
- Locked playlist requirements: read design/references/playlist-requirements.md and inspect playlist-panel.png when implementing history/resume and playlist work. The bottom playlist button opens a right-side slide-over in the same scene without pushing/reflowing video or controls. Folder entries show names/current/played/progress state; every played file has persistent independent history/resume, including partially watched entries and across restarts. Do not start Parts 4/5 until authorized.
