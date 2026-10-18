PART 4 OF 5: RESUME, HISTORY, HOME SCREEN

GOAL
IINA-style memory. The player remembers where I stopped in every file, and the start screen shows my last 10 videos. This must never cause any UI hitch.

TASKS
1. Database: SQLite (amalgamation, WAL mode) at %APPDATA%/FlowPlayer/flow.db. Table: file_key, path, size, mtime, duration, position, finished, last_played, audio_track, sub_track, sub_delay, volume, speed. All writes happen on a dedicated writer thread through a queue. The UI thread never touches the database. Schema versioning for future migrations.
2. FileIdentity: key = file size + hash of the first and last 64 KB (fast), with path as a secondary match, so a moved or renamed file still resumes. Handle files that are locked, on network drives, or very small (hash the whole file).
3. Saving position: on pause, every 5 seconds while playing, on seek end, on file change, on stop, on window close, and on app exit. Must survive a crash or kill (data at most ~5 s old). Each save is a single transaction.
4. Resuming: opening any file that has a saved position starts playback at (saved position minus 2 s). If the saved position is above 95% of the duration, or the file was finished, start at 0 and mark it finished. Show a toast "Resumed from 12:34". The resume seek must not cause a visible jump (use the start option when loading, not a seek after playback starts).
   - Across files: play A for 10 min, close, play B for a bit, then reopen A: it resumes at A's saved position. Same for series episodes after closing and reopening the player.
   - Also store and restore per file: audio track, subtitle track, subtitle delay, speed and volume.
5. Home screen (HomeScreen.qml, loaded by Loader; shown when launched with no file and when the playlist ends):
   - The last 10 played items like IINA: thumbnail, title, folder name, progress bar, "x min left", last-played time ("2 hours ago").
   - The most recent unfinished item is highlighted as "Continue watching" and focused so Enter or Space resumes it immediately. Click any item to resume. Keyboard navigation (arrows + Enter).
   - Right-click an item: remove from history, open containing folder, play from start. Missing files are shown dimmed with an "unavailable" note.
   - Works in fullscreen and Mini like everything else (Mini hides the home screen).
   - Setting "Auto-resume last video on launch" (default off) uses the setting from Part 3.
6. ThumbnailService: low-priority background thread using a SECOND libmpv handle (vo=null, ao=null, hwdec=no), seeks to about 10% of the duration and uses screenshot-raw; resize to 320x180; cache as JPEG in %LOCALAPPDATA%/FlowPlayer/thumbs; one at a time, and only when playback is not demanding. Show a placeholder until ready. Never block the UI or steal frame time from the playing video.
7. History cleanup: keep the last 10 on screen but store more (e.g., 200) for resume; prune the thumbnail cache of removed entries.

TESTS
T1. Close the app mid-video, reopen the same file: it starts at the saved position minus 2 s with no visible jump. Check the exact values.
T2. Play A 10 min, play B, reopen A: resumes A's position. Rename and move A to another folder, open it: it still resumes.
T3. Kill the process with Task Manager during playback; the saved position is at most ~5 s old.
T4. A file watched to 96% starts from 0 and is marked finished.
T5. Home screen shows exactly the last 10, newest first, with thumbnails and progress; Enter on the highlighted item resumes; right-click actions work; missing files are handled.
T6. Thumbnail generation during 4K playback causes zero dropped frames and no UI hitch (report dropped frames with and without generation).
T7. Per-file audio/subtitle track, delay, speed and volume are restored.
T8. Database stress: 1000 history rows, startup time and home screen load stay fast; UI thread never blocked by writes (report max frame time).
T9. Regression: fullscreen UI works, middle-click, double-click, mini mode, drag/resize smooth, idle CPU about 0%, cold start under 500 ms.

DONE WHEN
All tests pass with numbers. Give me a hand-test checklist. Do not start Part 5.
