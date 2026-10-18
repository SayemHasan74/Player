# Part 5 results — implementation present; full acceptance NOT passed

Muted, non-activating background checks only. No screenshots, native mode changes, focus activation, desktop dragging/resizing or interruption of an existing player. Latest user instruction limits testing to practical important checks; the full long suites were not run. No user history/settings were modified.

Final deployed executable: `728bc3bb953d89468e0d52305ebf0ea50bdb8efb8f791418fbdbc17fddfe9485`. The ten integration cases used `443f580f8d49a9db0761038cec533bbd45591a011161ca7950cceef9bc20f67c`. The only subsequent application changes were QML: move the playlist input layer below the 38 px title bar so title buttons stay accessible, and clear drag state before reordering destroys a delegate. The final hash was rebuilt/deployed and used for the final 40-second panel/playback check and normal-startup check. Integration results are not silently transferred between hashes. Current CTest 4/4 passed in 2.39 s.

## Requirement/test results

| Test | Result | Evidence / numbers |
|---|---|---|
| T1 auto-advance | Logic passed; visual modes manual | Open episode 5 of 10 → episode 6; saved 8.000 s loads with start=6.000 s; episode 5 persisted finished=1. No native mode toggles performed. |
| T2 neighbors / boundaries | Passed in process | 5→6→5→10→1, no boundary wrap; Previous after 8 s seeks to 0 without loading a neighbor. Windows natural sort, mixed case, leading zeros, extras filtering and stable model: 11 checks. |
| T3 per-file resume | Passed in process | A→B→A loads A at 10.123 s after its 12.123 s save; audio=2, subtitle=2, volume=61, speed=1.25 restored. Cross-launch episode-6 start=6.000 s from saved 8.000 s. |
| T4 playlist | Logic/scene passed; visual/manual pending | Ten folder rows, real current index 4, Loader in same scene; custom [2,9] reordered to [9,2], navigation opens 9. Full names wrap, watched/progress implemented. Actual mouse drag/fullscreen click checks remain manual. |
| T5 single instance | Passed for background IPC | Second process exits 0; original instance loads episode 1 then episode 8. Foreground activation and fullscreen preservation require manual check. |
| T6 SMTC / sleep | Initialized/API transitions passed; hardware manual | SMTC_READY on all 10 cases; successful display-inhibit on/off calls logged. Real unfocused hardware keys, flyout and physical sleep remain manual. |
| T7 bad files | Partial pass | Zero-byte + corrupt/unsupported data + missing path skip forward to episode 3; clean exits and no QML/SQL errors. Physical disconnected drive, deletion during playback and forced GPU reset not performed. |
| EOF settings | Passed | Last episode → Home by default; stayOnLastFrame=true → paused episode 10, home=0. |
| T8 4K60 HEVC 10-bit | FAILED full acceptance | nvdec; 40-second bounded run. Steady intervals below have 0 added drops, but pause→resume added 12. No five-minute certification. |
| T8 window/input/subtitle/home | Manual / retained scoped evidence | Four unit targets include prior mode restoration/settings/history tests. Native fullscreen UI, middle/double click, title/resize Mini gestures, tearing, ASS styling and history mouse behavior are manual. Earlier reports retain their own hashes. |
| T8 idle | Short interval only | About 5 s paused: 0 renders and 0 swaps. No 60 s minimized/paused test in this pass. |
| T8 startup | Normal launch passed | Fully closed unchanged launch 388.487 ms externally; Qt-entry first swap 372 ms; open→first video 136.0 ms. Launch→first video 665 ms from Qt entry. |
| T8 first updated deployment | Accepted exception, retained | 1286.581 ms external, 399 ms from application entry. File loading also began late; open→frame still 132.0 ms. |
| T8 deployment | Measured | 163,204,075 bytes = 163.20 MB / 155.64 MiB; mpv DLL 121.05 MB. 17 DLLs, no Widgets/WebEngine/Multimedia, no translation/software GL/TLS plugin deployment. |
| T8 8K / 4K AV1 | Not tested | No suitable fixture; no codec limit inferred. Earlier VP9 evidence is not a current pass. |
| T9 one-hour soak | Not run | User explicitly requested practical bounded testing; no stability/memory-growth claim for one hour. |

## Current 4K state measurements

GPU is whole-adapter utilization and includes the user's other apps. CPU is process CPU divided by 16 logical CPUs. These short samples are not a GPU frame-time or visual tearing measurement. “covered” is the presentation-only Mini cover without moving/resizing the desktop window; it is not a native Mini-mode test.

| State | Seconds | CPU mean | GPU mean (adapter) | Video renders / scene swaps | Added drops in interval |
|---|---:|---:|---:|---:|---:|
| hidden | 3.999 | 0.420% | 29.5% | 240 / 240 | 0 |
| playlist | 3.991 | 0.677% | 14.0% | 240 / 241 | 0 |
| paused | 4.990 | 0.019% | 2.4% | 0 / 0 | 0 |
| resumed | 4.004 | 0.720% | 28.5% | 241 / 241 | 0 |
| covered | 4.001 | 0.653% | 36.2% | 240 / 241 | 0 |

Pause→resume transition added **12 drops before resumed_begin**; the resumed interval's zero does not erase those drops. Final counter=12. Previous pre-title-input-fix run also added 12 at resume; no decoder/scaler/cache option was changed to hide the failure. Zero additional drops during the open-playlist interval is useful evidence but does not certify five-minute stability or visible transitions.

## Master resource budget

| Item | Result |
|---|---|
| CPU <about 5% while 4K playing | Passed in short sampled states above; whole-run steady-sample mean 0.623%, maximum 2.237% |
| Zero drops for five minutes | Not passed: 12 at pause/resume; five-minute run omitted |
| Resident RAM <500 MB including cache (user revision) | Slightly over: peak 509.61 MB (486.00 MiB). Original 150/300 MB budgets superseded. No elevated profiling or working-set trimming performed. |
| Paused/minimized ~0% CPU, no video renders | Short paused numbers above; minimized/60-second acceptance manual, not certified |
| Normal visible window <500 ms | Passed once at 388.487 ms; first updated deployment exception retained |
| Local open→video about300 ms | Passed 136.0 ms on normal launch; full launch→frame is reported separately |
| Lightweight scene | Callback-driven rendering, VSync=1, one scene; only finite panel/opacity animations. Stable playlist rows; no normal-runtime diagnostics timer or SMTC/sleep polling. Hidden interval has one scene swap per video frame. |
| Deployment trimmed | 163.20 MB; only needed Qt runtime/QML/Windows/JPEG components plus the large pinned mpv DLL |

## Five manual checks

1. Open episode 5, press **L**: right panel slides over video/controls without moving them. Read a long full name, click another item, hold/drag to reorder, check progress/watched marks. Verify title buttons remain usable.
2. In fullscreen and Mini/restore, use every control/menu/settings/playlist item; middle-click twice restores the exact state, double-click pauses, above-Tab and vertical resizing enter/leave Mini. Watch for tearing or drag/resize lag.
3. Seek near episode 5's end: 6 starts without a black flash; fullscreen/Mini stay unchanged. Previous after >5 s restarts; within 5 s selects the previous episode. Test the final episode's Home/stay-paused setting.
4. Partially play A and B, switch to C, then choose A/B from playlist; close/reopen and rename/move one. Confirm independent resume and tracks/volume/speed. Check ASS styling/audio continuity across modes and transitions.
5. With the player unfocused, use hardware media keys and Windows media flyout. Open another file through Explorer/second launch while fullscreen. Check display stays awake playing video and the request clears paused. Try the association script only if desired.

## Known limitations

- Pause/resume drop spike and slight resident-budget overrun above remain unresolved; stable visible framerate/tearing is not certified.
- HDR is tone-mapped to SDR; no native HDR output on this OpenGL path. Dolby Vision depends on a playable base layer.
- Mini keeps decoding/audio warm by explicit user preference; minimizing pauses for idle efficiency.
- Thumbnail work defers during active playback; concurrent thumbnail/4K certification is not claimed.
- Physical GPU loss, disconnected shares and OS media-key routing require manual checks. Network filesystem timeouts affect a worker, not the UI thread, but may delay resolution. Session reordering is intentionally not persisted.
- Five-minute, one-hour, physical interaction and unavailable codec/bitmap-subtitle fixtures are untested in this practical pass. Historical manual passes apply only to their recorded older executable.

Sources: tests/results/part5-release/cases.json; part5-final-performance/{summary,samples,flowplayer.log}; part5-normal-startup/summary.json; part5-ctest-final.log. Regenerate with `scripts/report-part5.py` after preserving these inputs.
