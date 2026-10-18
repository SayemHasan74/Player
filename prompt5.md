PART 5 OF 5: PLAYLIST, TV SERIES, SYSTEM INTEGRATION, FINAL PERFORMANCE

GOAL
TV-series behavior, system integration, robustness, and a final performance pass over the whole app.

TASKS
1. FolderScanner + NaturalSort: when a file is opened, scan its folder (non-recursive) for video files; sort with Windows StrCmpLogicalW natural sort (Episode 2 before Episode 10; handles S01E05 patterns, mixed case, leading zeros). Ignore non-media files and names like "sample" or "trailer" when they are clearly extras (make this a small configurable ignore list). Build the playlist with the opened file at its real index. Unit tests for NaturalSort and FolderScanner.
2. Auto-advance: when a file reaches its end, automatically start the next file with no user action. Keep the last frame on screen until the next first frame renders (no black flash). Preserve window state: fullscreen stays fullscreen, mini stays mini, window size does not jump unless the next file has a different aspect ratio (then adapt gracefully). If the next file has a saved position below 95%, resume it; otherwise start at 0. Marks the finished file as finished in the DB. At the end of the last file: show the home screen (setting: or stay paused on the last frame). Setting "Autoplay next" (default on, from Part 3).
3. Next/Previous: buttons, N/PgDn and P/PgUp keys. They work from ANY episode, including one opened at random, and jump to the real neighbor in the sorted folder list. Next on the last or Previous on the first shows a toast and does nothing. Previous after 5 s of playback restarts the current file (setting, default on); within 5 s goes to the previous file.
4. Playlist panel (PlaylistPanel.qml, Loader-based): toggle with the control-bar button and a key (L). An overlay inside the same scene (not a separate window), works in fullscreen. Shows folder items with the current one highlighted, watched check marks, per-item progress, click to play, session-only drag-to-reorder. Multiple files dropped together or opened together create a custom playlist.
5. Single instance: a second launch with a file argument forwards the path to the running instance via QLocalServer/QLocalSocket and exits immediately; the running instance brings itself to the front and plays the file (state-aware: stays in fullscreen if it was fullscreen).
6. Sleep prevention: SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED) while video plays; released when paused, stopped, or in audio-only mode as appropriate.
7. Media keys and SMTC: Windows System Media Transport Controls (C++/WinRT) showing the title and playback state; play/pause/next/previous/stop media keys work even when the window is unfocused.
8. Error handling: corrupt or unsupported file = toast, never a crash or hang, and skip to the next in a playlist; hwdec failure falls back silently; lost render context is recreated and playback resumes from the saved position; missing files in history are handled; log everything with rotation.
9. File association support: "Open with" and double-click-open work from the command line path handling (an installer is NOT part of this task; provide a small script or README steps to register the association).
10. Final performance pass over the whole app: profile CPU/GPU/RAM in every state; remove any unnecessary wakeups, timers, allocations in the render path, redundant property updates to QML, or per-frame work. Trim the deployment (windeployqt flags, remove unneeded Qt plugins and QML modules) and report the final folder size. Verify the resource budget from the master brief item by item.
11. README: exact build steps, where libmpv-2.dll goes, pinned versions, keyboard/mouse reference table, settings reference, known limitations (for example HDR is tone-mapped to SDR because the OpenGL render path cannot output HDR on Windows).

TESTS
T1. Folder with 10 episodes: open episode 5, let it end (seek to near the end to save time): episode 6 starts automatically with no black flash, in windowed AND in fullscreen AND in mini mode (state preserved).
T2. Open a random episode, press Next: the correct neighbor opens. Press Previous: correct. Next on the last file and Previous on the first behave as specified. Episode 2 vs 10 natural order is correct.
T3. Close and reopen the player mid-episode: resumes at the exact position (Part 4 still intact). Auto-advance into an episode with a saved position resumes it.
T4. Playlist panel works in fullscreen: click an item, watched marks, progress.
T5. Single instance: opening a second file while the player is running sends it to the running instance; fullscreen state preserved.
T6. Media keys and SMTC work while the window is unfocused. Display does not sleep during playback and can sleep when paused (verify the execution state).
T7. Error cases: corrupt file, 0-byte file, unsupported file, file deleted during playback, file on a disconnected drive: no crash, clear toast, sensible behavior.
T8. FINAL ACCEPTANCE (run everything from all parts and report numbers):
   - 4K60 HEVC 10-bit: zero dropped frames over 5 min, CPU under about 5%, hwdec in use, RAM.
   - 8K file or 4K AV1: smooth, or report the exact limit.
   - Fullscreen: every UI element visible and working (title overlay, control bar, seek, volume, menus, playlist panel, context menu, settings, home screen).
   - Middle-click toggles fullscreen from every mode and cursor position; second press restores the exact previous state.
   - Double-click pauses in every mode.
   - Mini mode: title bar + seek bar only, video continues, button restores exact previous state, also from fullscreen.
   - Aggressive drag and resize during 4K playback: smooth.
   - Resume across restarts and across different files; history shows the last 10 with thumbnails.
   - Series auto-advance and next/previous.
   - ASS subtitles correct in all modes.
   - Idle (paused and minimized, 60 s): about 0% CPU.
   - Cold start under 500 ms; deployed folder size.
T9. Soak test: play for 1 hour with periodic fullscreen/mini toggles and seeks; no memory growth (report RAM at start and end), no crash, no dropped-frame spikes.

DONE WHEN
Every test passes and a final report table (test, result, numbers) is delivered, listing anything not tested and why. Give me a final hand-test checklist and a list of known limitations.