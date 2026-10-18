# Changelog

## Part 4 outage audit and user interaction revision

- Verified tracked source, previous deployed hash and pinned SQLite sources; no user database existed at the configured path at audit start. Saved the audit and locked preferences; no unverified data-recovery claim.
- Removed JPEG/disk I/O from the thumbnail scheduling mutex and filtered stale stop/restart events between thumbnail jobs. Verified five fresh 320×180 JPEGs in sequence.
- Moved recent-file metadata checks to their own background worker, independent of SQLite writes and identity/open requests. Added generation guards and cached metadata. Replayed finished videos now retain new unfinished progress; corrupt-history fallback, empty selection, delayed auto-resume and canceled-open/stop races are handled.
- Thumbnail/availability changes now update stable history-model rows instead of recreating the list, preserving selection and pointer interaction. Added a persistent-index test.
- Added exact ±5-second plain arrows, working naturally sorted previous/next media-file transport, grave/physical-above-Tab Mini toggle and native resize-driven collapse/expansion. The full Part5 playlist/auto-advance remains deferred.
- Rebuilt the control/welcome layouts around the saved references using cached drawn icons, stable timeline label widths and compact metadata-aware history rows. Deferred initial framebuffer creation until a file opens; retained the same renderer thereafter.
- Expanded bounded tests. Kept pause/resume drop failures, rejected warm-swap experiment and unperformed visual/physical tests explicit in tests/PART4_AUDIT.md; full acceptance is false.

## Part 4 resume, history and home — manual/performance acceptance pending

- Saved prompt4.md; added SQLite 3.53.4 amalgamation with WAL/FULL transactions and schema versioning. All SQLite access uses a queued writer; file identity hashing uses a separate worker.
- Added independent content-based per-file resume, two-second load-time preroll, finished detection, track/delay/speed/volume restoration and saves on pause, five-second playback intervals, settled seeks, replacement, stop and exit.
- Added the reference-based Loader home screen: last ten entries, progress/time left, Continue watching, keyboard focus/navigation, missing state and scene context actions. Auto-resume is off by default; EOF returns to home. No IINA logo or extra window is used.
- Added serial low-priority headless libmpv screenshot-raw JPEG thumbnails, deferred during playback; retained 200 resume records and removed thumbnails with pruned/removed entries. Added the Qt JPEG plugin to deployment.
- Fixed home caption hit-testing and idle close handling; isolated all automated history/cache files. Bounded storage/home checks pass. Retained the failed 4K run, successful unchanged confirmation and unperformed visual/concurrent-generation checks in tests/PART4_RESULTS.md. Part 5 remains unstarted.

## Part 3 controls, tracks and settings — manual acceptance pending

- Saved prompt3.md and retained the single native window/Qt Quick scene in every mode, including hide-only Mini playback.
- Added the dark reference-based transport surface, shared drag/hover/chapter/network-cache seek bar, remaining/total toggle, track/speed menus, playlist placeholder and single-message toasts.
- Added in-scene right-click/URL menus and a Loader-created settings panel, including keyboard-safe text editing and scrolling in Mini. No native popup or second video/overlay window is used.
- Added coordinated 2.5-second controls/title/cursor auto-hide, hover/menu/drag holds, 140 ms fades, subtitle loading/cycling/adjustments, audio selection/delay, embedded cover art, cache info and live quality/hardware options.
- Added worker-written INI settings, volume persistence and settings unit tests. Corrected subtitle search folder separators for Windows per the mpv manual. Auto-resume and autoplay settings are saved only; their behavior remains for Parts 4/5.
- Removed an invisible cache-info binding that caused periodic scene swaps after controls hid. Basic evidence and remaining limitations are reported in tests/PART3_RESULTS.md; visual/fullscreen/subtitle styling acceptance remains user testing.

## Part 2 Mini revision — user approved hide-only playback

- Removed decoder disabling, video track reselection and Mini-related engine commands. Video/audio and decoding continue normally beneath the existing scene's cover, avoiding the rejected decoder restart path.
- Kept all title buttons and the bottom playback/volume/seek controls in Mini; adjusted compact size to 560 × 160 logical pixels, fixed height and horizontal resize.
- Recorded the requirement change in prompt2-amendments.md. Archived the failed decoder-off report and retained its raw counters.
- One short muted background hide/reveal check: zero drops, no A/V desync warning, continuous timeline, clean exit and 477.64 MB peak resident RAM. State tests passed; actual visual/native transitions remain user tests.

## Part 2 window modes and input — implementation, manual acceptance pending

- Saved prompt2.md after pushing the Part 1 project and visual references to GitHub.
- Added an explicit window state machine with nested previous-state snapshots, independent windowed/Mini geometries and automated deterministic/random round-trip tests.
- Added same-window Qt fullscreen, monitor-size native positioning, compact always-on-top Mini, native/client middle-click handling, title controls and complete Part 2 keyboard/wheel/double-click routing.
- Mini disables video decoding when a selected audio track can maintain playback. Silent videos retain hidden decoding so their timeline cannot freeze. The tiny-preview setting defaults off.
- Added reference-inspired plain QML transport/volume/seek controls, diagnostics, temporary toasts and a one-shot inactivity cursor timer. Preserved OpenGL, threaded rendering, swap interval 1 and the existing framebuffer item.
- Automated checks remain muted/background and do not fullscreen, raise, minimize, drag or resize windows on the user's desktop. Visual interaction, tearing and audible continuity remain user tests; see tests/PART2_RESULTS.md.

## Part 1 foundation — acceptance pending

- Added pinned, project-local Qt/libmpv/SDK dependency fetching, MSVC build, render-API spike, application and trimmed deployment scripts.
- Implemented worker-owned mpv events/options, asynchronous commands, queued QML state and an OpenGL framebuffer video item in the controls' Qt Quick scene.
- Added native frameless hit testing, Snap maximize target, aspect locking, DPI-aware fit, geometry persistence and basic file/input handling.
- Added rotating worker logging and asynchronous playback stop before render-thread teardown and final engine termination.
- Enforced muted, non-activating background benchmarks and executable hashes. Removed the unsuccessful multimedia scheduling experiment.
- Honored Windows 11 timer-resolution requests during active playback; restored automatic power policy on pause/minimize.
- Compiled QML ahead of time and deferred engine/scene initialization until the same dark window's first swap.
- Recorded the user's five passed manual checks and revised RAM acceptance limit of 500 MB on 2026-10-18. Removed the unused elevated heap-profiling helper.
- Recorded the explicitly approved first-deployment startup exception; normal launches on the packaged build measured 414.714–474.325 ms.
- Completed five-minute HEVC/H.264/VP9 measurements, 60-second pause measurement and twenty clean exits on one packaged executable. HEVC/H.264 had zero drops; VP9 had two. Full acceptance remains incomplete.
- Reverted unsuccessful deadline scheduling, decoder-concurrency and deferred type-registration experiments. Preserved failed evidence; restored the exact executable used for the complete pass. Added clean rebuild support to prevent stale objects after timestamp-preserving source restoration.
- Acceptance results and unresolved requirements are tracked in tests/RESULTS.md. No Part 2 features are implemented.
