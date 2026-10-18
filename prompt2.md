PART 2 OF 5: WINDOW MODES AND INPUT

GOAL
Fullscreen and mini mode that work flawlessly, controlled by an explicit state machine, plus all mouse and keyboard input. This part SOLVES MY ORIGINAL BUG: in fullscreen, all UI must be visible and fully working because it is the same window and the same scene.

TASKS
1. WindowStateMachine (separate class, unit-tested). States: Windowed, Maximized, Fullscreen, Mini. It stores the "previous state" explicitly and keeps separate remembered geometries for last Windowed and last Mini.
   - Fullscreen toggle (middle-click anywhere / F / button): from Windowed, Maximized or Mini, enter Fullscreen and store where it came from. Toggling again returns EXACTLY to that stored state with exact geometry (position, size, maximized flag). Esc exits fullscreen too.
   - Mini toggle (title-bar button / Ctrl+M): from Windowed, Maximized or Fullscreen, enter Mini and store where it came from. Toggling again restores exactly that state and geometry.
   - Middle-click inside Mini enters Fullscreen (previous = Mini); middle-click again returns to Mini.
   - Handle edge cases: toggling rapidly, monitor changes, the window being on a second monitor, DPI differences, window moved while in fullscreen.
   - Unit tests for every transition and for 100 random toggle sequences (the state must always be consistent and restorable).
2. Fullscreen implementation: the SAME window made borderless over the whole monitor it is on (MonitorFromWindow + GetMonitorInfo rcMonitor, SetWindowPos HWND_TOP; this covers the taskbar). Try Qt's showFullScreen() first. If it causes a flash, flicker, mode switch or black frame with OpenGL, use the manual WS_POPUP + SetWindowPos approach instead and keep the manual approach if it is cleaner. NO second window. Playback and audio must not stutter or flash black when entering or leaving fullscreen.
3. Mini mode: a small floating window (about 420 x 64 px, resizable horizontally only) showing ONLY the title bar (file title + restore button + close) and the seek bar. No video visible by default. Always-on-top while in Mini. Video keeps playing; audio never interrupts.
   - To save resources, set vid=no (disable video decode) while in Mini; on restore, re-enable video and make sure it resumes at the exact current position with no audible glitch and no black flash. Setting "Show tiny video preview in mini mode" (default off).
   - First entry: bottom-right of the monitor work area; afterwards remember the position. Title bar drags the window. The seek bar in Mini supports click and drag seeking (temporary simple version is fine; the polished seek bar comes in Part 3). Wheel = volume, Space = pause.
4. Title bar (final version): title text (file name), buttons: always-on-top (pin), mini-mode (the compact button), minimize, maximize/restore, close. Double-click on the title bar background maximizes/restores (standard).
5. Mouse input, working in ALL states and with the cursor ANYWHERE, including over the title bar, controls and the seek bar:
   - Double-click on the video or empty area = play/pause. Double-click on a button or the title bar does NOT toggle pause. Single click on video does nothing except focus.
   - Middle mouse button anywhere = toggle fullscreen. Implement with an event filter installed on the QQuickWindow (so no child item can swallow the event) AND handle WM_NCMBUTTONDOWN/WM_NCMBUTTONUP in the native filter for non-client zones (resize edges, caption, maximize button).
   - Wheel = volume (+/-5%); over the seek bar = seek.
6. Keyboard: Space pause; F fullscreen; Esc exit fullscreen; Left/Right seek 5 s; Shift+Left/Right 1 s exact; Ctrl+Left/Right 30 s; Up/Down volume +/-5; M mute; [ and ] speed -/+0.1; Backspace reset speed; Ctrl+M mini; Ctrl+T always on top; comma/period frame step back/forward when paused; I toggles an info overlay (resolution, codec, hwdec, fps, dropped frames, bitrate); Ctrl+O open file. (N/P, S, A, Z/X are added in later parts.)
7. Cursor: hide the mouse cursor after 2.5 s of inactivity while playing (including in fullscreen); show on movement. Always show it in Mini and when paused.
8. Simple temporary toasts for volume/seek/speed (final styling comes in Part 3).

TESTS (run all; report results; unit tests must be automated)
T1. State machine unit tests, including the 100 random sequences, all passing.
T2. FULLSCREEN UI TEST (the most important test in the project): enter fullscreen with a 4K video playing. The title overlay, control elements, and seek bar are visible and respond to clicks. Clicking buttons works, hovering works, wheel works, keyboard works. Take screenshots in windowed and in fullscreen and describe both.
T3. Middle-click toggles fullscreen from Windowed, Maximized and Mini, with the cursor over: the video, the seek bar, the title bar, a button, and a resize edge. A second middle-click restores the exact previous state and geometry (verify the numbers).
T4. Double-click on video toggles pause in Windowed, Maximized, Fullscreen and Mini. Double-clicking a button does not pause.
T5. Mini mode: shows only title bar + seek bar; video/audio continue; vid=no is applied; pressing the button again restores the exact previous state; works from Fullscreen; resource use in Mini is lower than in Windowed (report CPU/GPU).
T6. No black flash or audio dropout when entering/leaving fullscreen and mini (record dropped frames before/during/after).
T7. 4K playback + 20 rapid fullscreen toggles: no crash, no leaked windows, no stutter that persists.
T8. Multi-monitor: fullscreen goes to the monitor the window is on (test if a second monitor exists; otherwise state it was not tested).
T9. Regression from Part 1: drag/resize still smooth, idle CPU still about 0%.

DONE WHEN
All tests pass. Specifically confirm in your report: "in fullscreen, every UI element is visible and working." Give me a hand-test checklist. Do not start Part 3.