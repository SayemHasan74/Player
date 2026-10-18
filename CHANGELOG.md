# Changelog

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
