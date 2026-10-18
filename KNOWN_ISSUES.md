# Current limitations — Part 4

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
