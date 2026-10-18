# Part 2 implementation and basic verification — 2026-10-18

**Implementation available; visual and transition acceptance pending.** The user requested minimal automated checks and will perform visual playback tests. No Part 3 work is included.

The pre-Part-2 project was pushed to GitHub main at `64391a06c0c719282c8dd5f9af182450bd1e2173` before saving prompt2.md and starting implementation.

## Automated results

- Release application and all four compiled QML components build successfully.
- State tests: 61,341 checks, exact direct/nested restoration, independent Mini geometry, screen/DPI snapshots, twenty rapid state toggles and 100 seeded random sequences of 200 actions with full unwind. CTest: 1/1 passed in 0.02 seconds. These are model tests, not display transitions.
- Final packaged smoke: HEVC 4K60 Main 10 with AAC; 35-second app timer. QML and NVDEC loaded, vid=no was observed, the timeline advanced while video was disabled, the saved video track was restored, pause/resume worked, and shutdown freed the GL renderer before terminating mpv.
- All automated launches were muted and non-activating. No fullscreen, raising, minimizing, moving or resizing of the user desktop was automated.
- The first diagnostic fired before first video presentation on a slow initial launch. The scenario was corrected to arm after first presentation; failed/intermediate evidence remains in its result folders. No repeated long suite was run.

Executable SHA-256: `2a05a30a4625cbd3b1313c48df9357db08a9da5e559f6f7b42f8f3b9e607d0f9`.

| Measurement | Result |
|---|---:|
| Wall time including loader/startup | 39.569 s |
| Open request to first video swap | 182.000 ms |
| First window swap after deployment | 4050.833 ms |
| Process CPU mean / max | 0.527% / 3.919% |
| Peak resident RAM, including cache | 512.09 MB |
| Drops before disabling video | 0 |
| Drops at first paused sample after restoring video | 60 |
| Raw final dropped frames | 67 |

The final counter is retained without resets or subtraction. mpv logged a video-track refresh seek about one second behind the audio clock when video was reselected, followed by an **Audio/Video desynchronisation detected warning**. Mini restore therefore has an unresolved playback regression; it is not an acceptance pass. Seven further drops appeared after pause/resume. Adapter GPU reached 99% and includes other applications; no cause is established for every drop. The transient resident peak exceeds the revised 500 MB limit. No additional RAM profiling was performed.

### Short resource samples

| Phase | Samples | CPU mean | Adapter GPU mean | Adapter decoder mean |
|---|---:|---:|---:|---:|
| video | 4 | 0.745% | 7.00% | 12.25% |
| video disabled | 4 | 0.000% | 8.00% | 0.00% |
| paused | 2 | 0.000% | 12.00% | 0.00% |

These small samples exercise decoder disable/restore without changing window mode. They are not actual Mini/fullscreen display measurements. Adapter GPU figures include other apps.

## Architecture and scope

One existing QQuickWindow/HWND, engine, render context and scene serve all modes. Fullscreen uses showFullScreen plus the full monitor rectangle, without exclusive resolution switching. Mini changes the same window size/z-order; a plain QML item covers the existing framebuffer item. Controls remain above video in that scene. No video HWND or overlay window was added.

With selected audio, Mini applies vid=no and reselects the saved video track on restore without an application seek or audio pause. Silent files retain hidden decoding to preserve their only playback clock; this is an explicit resource-saving exception. Tiny preview defaults off. OpenGL, threaded rendering, physical-pixel FBOs, swap interval 1 and swap reporting remain enabled. No render polling timer was added.

Visible tearing, black flashes and audible continuity are not independently verified. It is not yet verified that **in fullscreen, every UI element is visible and working**. That statement requires the manual test below. Earlier Part 1 manual results apply to an earlier executable.

## Five manual checks (T2–T9)

1. **Fullscreen/UI:** play 4K, press F, click play/pause, scrub, hover and change volume; use keyboard shortcuts. Watch for tearing, lag and black flashes. Repeat from maximized. Capture windowed/fullscreen screenshots if useful.
2. **Exact return:** from Windowed, Maximized and Mini, middle-click over video, seek bar, title, buttons and resize edges. Toggle again/Esc; compare size, position and state. Mode logs include geometry and hwndUnchanged in %LOCALAPPDATA%/FlowPlayer/logs/flowplayer.log.
3. **Mini/nesting:** Ctrl+M from each mode; title+seek only, horizontal resize, remembered position, always-on-top, seeking, wheel and Space. Mini → fullscreen → Mini must restore exactly. Check audible continuity yourself if desired; automation stays muted. Try a silent clip and optional tiny preview.
4. **Input/cursor:** video/empty-area double-click pauses; title double-click maximizes/restores; button double-click does not pause. Try the README key map. Cursor hides after 2.5 seconds playing, stays visible paused/in Mini, returns on movement.
5. **Rapid transitions/regression:** twenty actual fullscreen toggles during 4K, then drag/resize, pause/minimize/restore and exit. Check persistent stutter, black borders and responsiveness. Try another monitor/DPI if available. No physical multi-monitor test was performed here.

Manual display tests, screenshots, actual transition HWND stability, 20 real fullscreen toggles and a long idle regression were deliberately not automated under the user's latest testing instructions. Full acceptance is not claimed.
