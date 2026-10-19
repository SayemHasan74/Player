# Current limitations — Part 5

Latest behavioral build9f3d634a... is covered by tests/FINAL_PASS.md: maximize/restore handling, global Escape, hidden-arrow behavior and scale0.64 controls. Bounded input checks passed; physical native maximize/minimize/fullscreen checks remain manual. No broad playback/RAM/soak acceptance was rerun or transferred from older hashes.

Latest UI-only revision is documented in tests/UI_POLISH.md (hash5fc73d5b...). It changes palette/layout/menu anchors/state icons; it does not resolve or re-certify the older playback/performance findings below. Their hashes remain explicit.

Authority: tests/PART5_RESULTS.md and tests/part5-package.json. Final package SHA256 728bc3bb953d89468e0d52305ebf0ea50bdb8efb8f791418fbdbc17fddfe9485. Part 5 features are implemented, but full acceptance is false.

- Current 4K nvdec check adds 12 dropped frames at pause/resume. Steady hidden, playlist-visible, resumed and video-covered intervals add zero. This does not certify stable visible cadence or tearing. No decoder/counter suppression was used.
- Peak resident RAM was 509.61 MB (486.00 MiB), slightly over the user's 500 MB allowance. No elevated profiling, cache-budget weakening or working-set trimming was performed. Normal startup was 388.487 ms; first updated deployment was 1286.581 ms (accepted deployment exception). Open-to-frame was 136 ms on the normal launch.
- Four unit targets and ten integration cases passed. Integration hash 443f580f... predates two QML-only title-input/drag-handler corrections; current-hash panel/performance/startup checks passed their basic smoke conditions. Exact scopes are in the report.
- Native fullscreen/mini/resize/tearing, actual playlist mouse reordering, ASS appearance, unfocused hardware keys, Explorer activation, physical sleep, disconnected drive, deletion during playback and forced GPU loss remain manual. SMTC initialization and sleep-request API transitions are logged, not substituted for those physical checks.
- Five-minute and one-hour acceptance, 60-second minimized/paused measurements, 8K/4K AV1 and unavailable bitmap subtitle fixtures were not run under the user's practical-test instruction. Historical user passes apply only to the older recorded build.
- HDR is mapped to SDR on this OpenGL path; no native HDR output. Mini keeps video/audio decode warm. Minimize pauses. Thumbnail generation defers during playback. Session playlist ordering is intentionally not persisted.

## Archived Part 4 findings (older executable scopes)

Latest authority: tests/PART4_AUDIT.md and tests/part4-audit-package.json. The outage audit repaired thumbnail lock/event bugs, replay state, metadata-worker blocking and pending-open/shutdown races. Input/UI preferences are implemented and documented in design/interaction-preferences.md.

- Audit 4K checks still measured 9 and 17 drops around pause/resume on their named pre-release hashes. A single warm-swap diagnostic failed and was removed. Stable resume cadence is not certified for the final release; no drop suppression or exception was introduced.
- Final release passed seven focused cases and a canceled-open check; older core/performance cases retain their own hashes. Native top-edge Mini collapse, expansion, fullscreen interaction, tearing and final visual matching need manual checks. Resize transitions commit when the edge is released.
- Thumbnail jobs defer during active playback; strict simultaneous-generation-on-4K remains unverified. Physical SMB availability timing and a real power-cut experiment were not performed. The tracked source survived; unknown unsaved work or absent pre-outage history cannot be reconstructed from the audit.

Earlier findings below remain scoped to their earlier binaries:

Part 4 implementation and basic storage/home checks are available. See tests/PART4_RESULTS.md and tests/part4-package.json for exact executable scopes. Full acceptance is false.

- One 20-second 4K queued-thumbnail run recorded 408 drops and 3,079.70 ms normal startup; the unchanged confirmation recorded zero drops and 456.94 ms startup. No thumbnail ran during either playing interval. Cause is undetermined; the failure is retained rather than called fixed.
- Thumbnail generation is deliberately deferred during playback. Therefore simultaneous generation during 4K (T6 as written) has not been certified.
- Home caption/client/focus, auto-resume, EOF/close, independent resume, moved identity, completion threshold and per-file options passed bounded checks. Real fullscreen, keyboard/mouse/context-menu interaction, visual no-jump, tearing, drag/resize and subtitle/audio continuity remain manual. Locked/network-file behavior is implemented but physical SMB/locked-file cases were not exercised.
- Whole-run home frame/heartbeat maxima include startup/render-context work and do not prove no hitches. Three CTest targets pass; the 1,000-save unit main-loop gap was 2.071 ms, distinct from GPU frame time. A fresh long idle test and ten actual minutes of A playback were not repeated under minimal-testing instructions.
- The final executable adds a home caption guard after the eight-case storage suite; three focused home/auto-resume/EOF checks cover that package. Reports name both hashes. Earlier Part 3/VP9 issues below are not claimed fixed.
- Part 5 is not started. Playlist UI/navigation and autoplay-next remain placeholders. Its right-side slide-over reference and independent resume requirements are saved.

## Retained Part 3 findings

The authoritative current measurements are in tests/PART3_RESULTS.md. The controls, seek bar, scene menus, tracks, toasts and worker-written settings are implemented; full acceptance is pending.

- Pause/resume still registers dropped frames: the final4K check recorded1 before pause and5 before enabling HQ, an increase of4. A short pre-unpause Windows timer-policy experiment did not eliminate this and was reverted. Stable visible cadence after resume is not certified.
- HQ's measured on/playing interval added no drops (5→5); switching off completed without crash/command error, but its separate transition counter and visual continuity were not certified. One drop had also appeared around the initial auto-hide transition. Seeks reset mpv's counter; finalzero values are not claimed as evidence of zero earlier drops.
- Exact seeks measured10ms absolute position error on both4K and the subtitle/multi-audio fixture. Hidden controls produced372renders/372scene swaps; paused produced0/0. Peak resident RAM across all raw samples was484.29MB, within the user-approved500MB limit.
- Real fullscreen/Mini interaction, visible tearing, black flashes, audible continuity, mouse-drag feel, styled ASS/karaoke appearance, physical multi-monitor and network-buffer visualization remain user tests under the explicit minimal-automation instruction. PGS/VobSub and FLAC fixtures were unavailable and are not marked tested.
- Normal-launch startup and long idle stability have not been revalidated as a new full suite. Earlier Part1/Part2 reports apply to their own executable hashes. The older Part1 VP9 two-drop result is not claimed fixed.
- Playlist buttons intentionally show placeholders; playlist behavior is Part5. The older Part3 report predates Part4; auto-resume is now implemented, while autoplay-next remains Part5.

Part3 packaged executable SHA256:3073b3e79f5d11cecaa6a83ab05fb6bc095a6689b9136fb55befd2b89ebce2e5. All three final smoke processes used this hash. Old rejected decoder-off Mini reports remain archived; current Mini keeps video/audio/decoding running and hides only video.
