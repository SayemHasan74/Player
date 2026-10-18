# User constraints

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
