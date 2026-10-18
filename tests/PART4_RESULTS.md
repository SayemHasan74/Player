# Part 4 — measured results

Implementation available; **full acceptance is not claimed**. Tests were muted, non-activating and isolated from user history/settings. Visual/native interaction and audible continuity remain manual. Part 5 has not started.

Current package SHA-256: `03912a7bb346bdbadfacd8653aadfe496f0a8822f8bb403af2651488fa8ba774`. The final home/caption/focus, auto-resume and EOF/close checks used this package. The eight-case storage suite and 4K measurements used `b5ce773c0036198849f9eb218e867ada1374b7a5eb7df08ad581684757b443b8` immediately before a caption hit-test correction and diagnostic logging; storage, resume, rendering and thumbnail code are unchanged between these packages. No earlier user visual pass is transferred to either package.

## Basic behavior

| Test | Evidence |
|---|---|
| T1 restart | Saved position is passed as `start=position−2` in the loadfile node-map. Observed restart position matches within one video frame; no post-start resume seek. Visual no-jump remains manual. |
| T2 independent A/B + moved identity | Live A→B→A returned at 10.133 s after saving 12.133 s; audio 2, subtitle 2, delay 0.3 s, volume 61 and speed 1.25 restored. Process restart and changed path matched the same identity. Unit test performs a real rename and verifies saved 600→598 s; the live test uses short playback, not ten actual minutes. |
| T3 forced termination | Last committed sample was **2.543 s** old at kill; reopening used its position−2 s. One measured kill, not a crash-proof guarantee for every filesystem. |
| T4 96% / finished | Updating the stored position to 96% produced start=0 and persisted finished=1. |
| T5 home | SQL returns newest 10; 1,002-row database exercised. Missing records marked unavailable. JPEG generation succeeded, including 320×180 output through the second mpv handle. Current package verifies home focus, native caption/client classification, auto-resume setting and EOF return/clean close. Real keyboard/mouse/context-menu/fullscreen checks remain manual. |
| T7 per-file options | Actual mpv observations confirmed audio 2, sub 2, sub-delay 0.3, volume 61, speed 1.25 after restart. |

Core suite peak resident memory: **342.93 MB**. Eight core cases passed their bounded checks. Three CTest targets (state machine, settings, history) passed; the latest full CTest run took 4.52 s.

## Storage and UI timing

1,000 queued save transactions: enqueue 1.793 ms, worker drain 2249.909 ms; retained 200 entries. Ten-item worker read: 0.476 ms. Unit-test main event-loop max gap: 2.071 ms. These are event-loop measurements, not GPU frame times.

History uses SQLite 3.53.4 amalgamation, WAL, synchronous FULL, schema user_version=1. Every SQLite call is on the writer worker. File identity stat/hash operations have their own worker so they cannot hold the writer queue. Snapshot construction on the UI thread only copies observed state; it performs no database/file operations. Writes flush in one transaction on pause, five-second playback timer, settled seek, file replacement, stop and exit. Shutdown drains queued saves.

The regular core launches after deployment measured 379.45–411.75 ms to first swap; the one-time deployment launch took 1,755.15 ms. Home scene creation was 422–426 ms from application entry in the home cases; the query logged ten rows with workerMs=0 (millisecond-resolution measurement).

Maximum whole-run scene synchronization/render time for the 1,002-row home case was **74.218 ms**, and max UI heartbeat gap **126.386 ms**, including initial render-context/startup work. The final caption/focus home check measured **10.106 ms** max sync/render and **126.262 ms** max heartbeat gap. These totals do not prove zero hitches or isolate database-induced frame cost; strict T8 no-hitch acceptance remains open.

## T6 / T9 short 4K measurements

| Case (20 s requested) | Drops at final sample | CPU mean | Adapter GPU mean | External first swap |
|---|---:|---:|---:|---:|
| Baseline | 0 | 0.730% | 23.2% | 399.11 ms |
| Queued thumbnails — failed run | 408 | 0.310% | 3.9% | 3079.70 ms |
| Queued thumbnails — unchanged confirmation | 0 | 0.943% | 22.8% | 456.94 ms |

All used nvdec. GPU data include other applications. The failed queued run also took 6,758 ms to QML load and 815 ms from open to first frame; its normal-startup target failed. The unchanged confirmation took 456.94 ms to first swap and 141 ms from open to first video swap. The failure is retained, and no cause is asserted. Short zero-drop runs do not certify stable cadence, tearing or long-duration playback.

**Thumbnail jobs are deferred throughout active playback**, including these queued 4K runs. They execute serially on a low-priority worker when paused/home, using a second libmpv handle with vo=null, ao=null, hwdec=no, one decoder thread and no cache. Therefore this checks the safe scheduling policy; it does **not** pass the stricter simultaneous-generation-during-4K test. No thumbnail job ran during those playback samples. T6 as written remains unverified.

Part 3 pause/resume drops and the older VP9 drop finding have not been claimed fixed. No fullscreen/drag/resize/middle-click/visible-tearing automation was performed. A five-minute 4K test and fresh long idle test were not repeated under the user instruction to keep testing minimal.

## Five hand checks

1. Open a video, stop around a memorable time, close and reopen it. Check the two-second rewind and toast; switch between two files, then rename/move one and reopen it.
2. Change audio/subtitle track, subtitle delay, speed and volume in each file; reopen both and verify their independent settings.
3. Launch without a file: test the last-ten list, Continue watching with Enter/Space, arrows, thumbnails/progress, missing items and all three right-click actions. Toggle auto-resume and restart.
4. Try home, menus and playback controls in fullscreen and Mini/restore. Check title dragging, middle-click, double-click and resize; Mini hides home/video while retaining title and controls.
5. Watch 4K motion and pause/resume for stutter/tearing. Let a short file end and check return to home. Thumbnail placeholders should populate while paused/home, not steal playback time.
