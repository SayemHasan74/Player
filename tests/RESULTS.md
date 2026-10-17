# Part 1 acceptance report

**NOT ACCEPTED — remaining failures below.** No Part 2 implementation.
The user explicitly accepted a first-deployment-only startup exception. The 500 ms target still applies to subsequent fully closed process launches.


Packaged executable SHA-256: `f8e3d330c6898ea46bda22d06c0d06e830739cad7d15efe1077b80ec5ebf4e7c`. Automated test hashes: `f8e3d330c6898ea46bda22d06c0d06e830739cad7d15efe1077b80ec5ebf4e7c`.

Machine: Windows 11 Pro build 26300, Core i5-13400F (16 logical processors), RTX 3070 8 GiB, NVIDIA driver 617.42.

## User revisions and manual results

- On 2026-10-18 the user accepted resident RAM below **500 MB**, overriding the original 150/300 MB budgets. The limit below includes the cache; no cache subtraction or working-set trimming is used.
- The user reported all five manual checks passed: overlays/input, file opening, drag/resize/aspect, Snap Layouts and pause/minimize/restore.
- Manual evidence applies to executable `879f8ceaa913567c1dcdafd21d9253a028d121c9f13cc76cfea3769b0d3b3269`; it is user-reported, not an automated observation of later performance changes.
- All automated runs are muted, non-activating bottom windows. No automated desktop drag, resize, Snap or minimize/restore is performed. Muted testing cannot establish audible continuity.

## Playback measurements

| Clip | Seconds played | Decoder | Drops | CPU mean / max | Adapter GPU mean / max | Resident RAM mean / max (MB) |
|---|---:|---|---:|---:|---:|---:|
| HEVC 4K60 Main 10 | 319.583 | nvdec | 0 | 0.69% / 1.68% | 27.47% / 87.00% | 473.25 / 479.75 |
| H.264 1080p60 | 319.633 | nvdec | 0 | 0.62% / 1.56% | 29.36% / 79.00% | 356.14 / 361.14 |
| VP9 4K30 | 319.633 | nvdec | 2 | 0.36% / 1.38% | 31.76% / 92.00% | 404.47 / 408.03 |

GPU readings cover the entire adapter, including other applications. CPU is process utilization normalized across sixteen logical processors. Resident RAM is the actual total working set in decimal MB. Private/committed memory and packet-cache bytes remain separately recorded in the raw JSON.

Clips are synthetic motion patterns. HEVC and H.264 repeat twelve-second video patterns with continuous AAC audio. VP9 repeats a six-second pattern and has no audio. This does not establish playback of every possible real-world file.

## Idle, launch and shutdown

- Pause: 60.001 seconds; mean total CPU 0.0130%, max 0.1875%; 0 video renders and 0 mpv render callbacks. Test-only metrics logging was enabled.
- Minimize/restore: user-reported functional pass. No new automated minimize cycle was performed under the background-only restriction. Prior instrumented minimize measurements are retained in earlier result folders; they are not presented as current-binary measurements.
- First launch after deployment: **1745.955 ms** to window first swap.
- Subsequent fully closed process launches: 414.714–474.325 ms, mean 431.177 ms.
- Launch figures include process creation and DLL loading. The window stays behind other applications; first swap is the presentation marker. No OS cache flushing or reboot was performed while the user worked.
- Clean shutdowns: 20/20, with renderer destroyed before engine termination.

- First video after initial deployment: 3337.955 ms from process launch. This includes initial engine/DLL loading; it is distinct from opening a file once the engine is ready.

| Clip | Open request to first video swap | Launch to first video swap |
|---|---:|---:|
| HEVC | 127.000 ms | 744.063 ms |
| H.264 | 88.000 ms | 699.926 ms |
| VP9 | 105.000 ms | 675.848 ms |

## Acceptance checks

| Check | Result |
|---|---|
| singleTestedExecutable | PASS |
| hevcFiveMinutesZeroDrops | PASS |
| allPlaybackNoDrops | FAIL |
| hevcCpuBelowFivePercent | PASS |
| ramWithinUser500MBLimit | PASS |
| pauseSixtySecondsIdle | PASS |
| twentyCleanExits | PASS |
| startupWithinUserApprovedTarget | PASS |
| localOpenFirstFrameWithin300Ms | PASS |
| manualChecksUserReportedPassed | PASS |
| allAutomatedRunsMutedAndNonActivating | PASS |

## Changes and reproducibility

- Uses libmpv render API, OpenGL and the threaded Qt Quick scene. Video and controls share one window and scene; no native video child or separate overlay.
- Windows 11 timer requests are honored during playback even when muted/occluded. Automatic timer power management resumes when paused/minimized. No rendering poll timer was introduced.
- QML is compiled ahead of time. The same dark window presents before its QML scene and mpv initialization are loaded.
- Normal mpv timing is retained. The earlier video-timing-offset=0 experiment failed and was reverted; its raw results remain available.
- Build and run commands, pinned versions and five manual checks are in README.md. Raw per-second samples, logs and hashes are under tests/results.

This report does not redefine failed thresholds as passes. See the verdict JSON for the exact checks and user-authorized limits.
