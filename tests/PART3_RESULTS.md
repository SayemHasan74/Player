# Part 3 controls, tracks and settings — measured results

**Implementation available; full acceptance is not claimed.** Visual/fullscreen/subtitle styling tests remain with the user under the minimal-automation instruction. The checks below retain any measured failures. No Part 4 work.

Packaged executable SHA-256: `3073b3e79f5d11cecaa6a83ab05fb6bc095a6689b9136fb55befd2b89ebce2e5`. All three final process runs used this executable.

## Basic checks

| Check | Result |
|---|---|
| singlePackagedExecutable | PASS |
| unitTests | PASS |
| sceneMenusAndLazySettings | PASS |
| controlsHideAndStayShownPaused | PASS |
| programmaticUiHold | PASS |
| hiddenNoExtraSceneSwaps | PASS |
| pausedNoRendersOrSwaps | PASS |
| exactSeekWithinOne60fpsFrame | PASS |
| audioTrackChanged | PASS |
| subtitleAdjustmentsApplied | PASS |
| audioDelayApplied | PASS |
| srtAssVttDecoderEvidence | PASS |
| autoLoadedSubsDirectory | PASS |
| noDuplicateSidecars | PASS |
| chapterMetadata | PASS |
| embeddedCoverSelected | PASS |
| volumeAcrossProcessRestart | PASS |
| qualityOnIntervalNoAddedDrops | PASS |
| pauseResumeNoAddedDrops | FAIL |
| allObservedResidentRamBelow500MB | PASS |
| cleanMutedNonActivating | PASS |
| noQmlErrors | PASS |

The state-machine and settings CTest targets passed. Settings tests cover defaults, clamping, worker flush and persistence through destruction/recreation. A separate actual application restart recovered volume73. Settings files for tests were isolated from user preferences.

## Resource and render samples

| Phase | Seconds | CPU mean | Adapter GPU mean | Adapter decoder mean | Video renders | Scene swaps | Drops start → end |
|---|---:|---:|---:|---:|---:|---:|---|
| visible | 6.002 | 1.387% | 8.400% | 13.800% | 360 | 364 | 0 → 0 |
| hidden | 6.212 | 0.834% | 19.500% | 22.000% | 372 | 372 | 1 → 1 |
| paused | 4.992 | 0.000% | 19.600% | 0.000% | 0 | 0 | 1 → 1 |
| quality | 5.989 | 0.852% | 33.000% | 19.667% | 359 | 363 | 5 → 5 |

Peak resident working set across every raw process sample, including startup/cache: **484.29 MB**. Adapter GPU data include other applications and are not isolated player utilization. These short intervals do not replace long stability or 60-second idle tests.

HQ sample minus visible normal sample: CPU -0.535 percentage points; whole-adapter GPU +24.600 percentage points. These are observed differences under concurrent desktop load, not isolated causal costs of HQ.

Observed pause/resume counters: 1 before resume → 5 before enabling HQ. HQ-on/playing: 5 → 5 → 5. Resume and HQ activation were separated by two seconds. HQ was switched off before the final paused seek; no crash or asynchronous option error was observed, but visual continuity and a separate off-transition counter were not certified.

The mpv frame-drop counter resets on seeks/file changes. Final zero values after seeking do not mean earlier playback had zero drops. Maximum observed pre-seek counters: {'part3-release-controls-20261018': 5, 'part3-release-tracks-20261018': 7}. A short experiment moving the Windows timer-policy request before unpause did not eliminate the resume drops and was reverted. Its evidence is retained separately; no timing workaround was promoted.

## Seeking and media

| Requested | Observed | Absolute error |
|---:|---:|---:|
| 10.123s | 10.133s | 0.010s |
| 4.123s | 4.133s | 0.010s |

These are asynchronous position reads after playback-restart while paused, not a claim of visual mouse-drag precision. Dragging, hover tooltip alignment, bitmap subtitles and network buffer visualization remain manual.

The generated fixture contains two AAC audio languages, embedded SRT/ASS, a matching sidecar under Subs and three chapters. Switching audio, selecting styled ASS (Segoe UI Bold font logged), loading external WebVTT, subtitle delay/scale/position and audio delay were exercised. This establishes decoder/control operation, not visual correctness of colors, positioning or karaoke. MP3 cover art was selected as an attached video/image track in the same renderer. PGS/VobSub and FLAC samples were unavailable and are not marked passed.

## Scene architecture and settings

Video, title, controls, seek tooltip, toasts, menus, URL editor, info and the Loader-created settings panel are items in the same existing Qt Quick scene. No native video child or popup/overlay window is introduced. Native caption right-click is routed to that scene too. Menu/settings lists scroll in Mini. The user-approved Mini behavior keeps decoding/audio running and hides only video.

Auto-hide uses one2.5-second single-shot timer and140ms opacity fades. Hover/menu/drag holds suppress hiding; paused/Mini controls stay shown. Invisible cache-info bindings were removed after an earlier diagnostic revealed extra swaps. The seek throttle runs only during dragging and is not a video-render polling timer.

Preferences/volume are stored in %LOCALAPPDATA%/FlowPlayer/settings.ini with worker-thread writes. Defaults: auto-resume off, autoplay-next on, hardware decode on, HQ off, Mini preview off, speed1.0 and seek steps5/1/30seconds. Auto-resume/autoplay actions remain reserved for Parts4/5. Native geometry storage from earlier parts remains separate.

## Five manual checks

1. **Fullscreen/Mini UI:** in each mode, use every button, seek bar, audio/subtitle/speed menus, right-click context menu, URL editor and Settings. Confirm hover/clicks work above video and no tearing/black flashes appear. Menus must stay inside the player.
2. **Seeking/auto-hide:** click and drag4K, check preview/hover time, chapter ticks and buffered network ranges. Confirm exactly one accurate release seek. Wait2.5seconds, then move/hover/open a menu/pause; verify matching cursor/title/control behavior in fullscreen.
3. **Subtitles:** use provided SRT/ASS/VTT fixtures, embedded tracks, S, off/load and subtitle drop; test Z/X, size/position, ASS colors/top text/karaoke through resize/fullscreen/Mini restore. Use your own PGS/VobSub samples if available.
4. **Audio/settings:** cycle A and menu tracks, adjust delay/volume, restart and check memory, open MP3/FLAC art. Toggle HQ/hardware decode live and inspect I. Check languages/default speed/aspect/seek-step/Mini-preview persistence. Auto-resume/autoplay are saved flags for later parts.
5. **Regression/stability:** middle-click over controls/title/resize edges, video double-click versus button double-click, exact mode restoration, twenty fullscreen toggles, dragging/resizing and pause/resume. Pay particular attention to the measured resume drops; stable visible cadence is not certified.

No screenshots, audible continuity, real fullscreen interactions, physical multi-monitor checks or visual styling checks were performed automatically. Earlier Part1/Part2 manual passes apply to their earlier executables.
