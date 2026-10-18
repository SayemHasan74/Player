# PART 3 OF 5: CONTROLS, SEEK BAR, SUBTITLES, AUDIO, SETTINGS

## Goal
The complete IINA-style control surface and everything about tracks and quality. All of it must be overlay items in the same scene and work in every window state (this was proven in Part 2; keep it true).

## Tasks
1. Control bar (bottom overlay, dark, minimal, subtle gradient background, no blur/shadows): previous, play/pause, next, seek bar, elapsed / remaining time (click to toggle remaining/total), volume icon + slider, audio-track menu, subtitle menu, speed menu, playlist toggle (button present; panel is built in Part 5), fullscreen button. Previous/next buttons are wired in Part 5; for now they are visible but may show "no playlist" toast.
2. Seek bar (SeekBar.qml, reused by Mini mode): smooth drag; shows the target time while dragging; hover shows a time tooltip; chapter ticks when the file has chapters; click to jump. While dragging use fast (keyframe) seeks; on release do one exact seek. Shows buffered range for network sources. Must feel instant with no jitter on 4K.
3. Auto-hide: controls, title overlay and cursor hide after 2.5 s of no mouse movement while playing; reappear instantly on movement; never hide while the cursor is over a control or any menu/popup is open. While paused, controls stay visible. Only a 120-150 ms opacity fade; no animation otherwise. Same behavior in fullscreen.
4. Toasts (Toast.qml): short OSD messages for volume, seek, speed, subtitle/audio change, mute, errors. Lightweight, auto-dismiss, no stacking mess.
5. Right-click context menu: open file, open URL (Ctrl+U from clipboard), audio tracks, subtitle tracks, subtitle delay, playback speed, aspect ratio (auto, 16:9, 4:3, 21:9, fill), video info, always on top, settings. Menus must work and be visible in fullscreen, as popups inside the same scene (not separate native windows).
6. Subtitles (libass through mpv): SRT, ASS/SSA with full styling, VTT, PGS, VobSub, embedded and external. Auto-load matching external files and Subs/ folders. Auto-select by preferred language setting. Subtitle menu: track list, off, load file, delay (Z/X keys -/+0.1 s), size, position. Drag-and-drop a subtitle file onto the window loads it. Key S cycles subtitle tracks. Subtitles must render correctly when resizing, in fullscreen, and after leaving mini mode.
7. Audio: track menu, key A cycles tracks, audio delay adjust, volume with a max of 150, remembers volume between launches. Audio cover art shown for audio-only files (MP3, FLAC, etc.).
8. Settings panel (Loader-based, not instantiated until opened): language preferences (audio/sub), auto-resume-on-launch (default off; used in Part 4), autoplay next (default on; used in Part 5), lock aspect ratio, "High quality" video toggle (applies the HQ option set from the master brief live), hardware decoding on/off, seek step sizes, show mini preview, default speed. Persist to a settings file.
9. Info overlay (I key): resolution, codec, hwdec in use, fps, dropped frames, bitrate, cache.

## Tests
T1. Fullscreen UI test again: every control, the seek bar, all menus, the context menu and the settings panel are visible and working in fullscreen and in Mini restore.
T2. Seek bar: drag and click accuracy, no jitter at 4K, tooltip correctness, exact seek on release (report the position error).
T3. Auto-hide: hides after 2.5 s, never while hovering or with a menu open, always shown when paused, cursor hides and returns. Verify in fullscreen too.
T4. ASS-styled subtitle file (fonts, colors, positions, karaoke if available) renders correctly in windowed, fullscreen, mini-restore and during resize. Test SRT, embedded subtitle track switching, delay, size and position changes, and drag-drop of a subtitle file.
T5. Audio track switching on a multi-audio file; delay; volume memory across restarts; cover art for an MP3/FLAC.
T6. High quality toggle: switch it on/off during playback with no crash or stutter; report GPU/CPU difference on 4K.
T7. Resource check: controls visible vs hidden vs paused: report CPU/GPU. Hidden controls while playing must not cause extra rendering beyond video frames.
T8. Regression: drag/resize smooth, middle-click, double-click, mini mode, state machine tests all still pass.

## Done when
All tests pass with numbers. Give me a hand-test checklist. Do not start Part 4.

The user's prior testing amendments remain in force: minimal muted background automation; the user performs visual playback/tearing/transition tests. Mini keeps video/audio/decoding running and hides only the video, with all title and playback controls retained. Reference images remain locked; do not copy the IINA logo.
