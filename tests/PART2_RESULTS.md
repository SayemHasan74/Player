# Part 2 current verification — Mini hides video, keeps playback running

The user approved this change on 2026-10-18; prompt2-amendments.md supersedes the original vid=no/title-and-seek-only Mini requirements. Part 3 is not started.

## Current implementation

- One existing native window, Qt Quick scene, framebuffer video item and mpv engine in every mode. Fullscreen does not create a video or overlay HWND.
- Mini hides video behind an ordinary scene item. Decoding, audio and the playback timeline continue unchanged. No vid=no, track reselection, seek or pause occurs when entering/leaving Mini.
- Mini starts at 560 × 160 logical pixels, resizes horizontally, and retains the full title button row and bottom playback/volume/seek controls. It is always on top. The height accommodates the requested controls. Tiny preview remains off by default.
- OpenGL, threaded rendering, physical-pixel FBOs, swap interval 1 and swap reporting remain enabled. No render polling timer.

## Automated basic results

- State-model tests passed, including 100 random sequences of 200 actions, direct/nested exact restoration, geometry/screen/DPI snapshots and twenty model fullscreen toggles. CTest output is in part2-unit-results.txt. These are model tests, not actual desktop transitions.
- One short muted non-activating packaged HEVC 4K60 Main 10/AAC smoke hid and revealed the same video scene item. No desktop fullscreen, topmost, minimize, move or resize was automated.

Executable SHA-256: `c9e5eb68e4b934ab95aada5b746deb9a29674c7f54c0aae2ec6a59fbb29be3b6`.

| Measurement | Result |
|---|---:|
| Wall time including startup | 29.021 s |
| Video played | 24.200 s |
| Decoder | nvdec |
| Dropped frames | 0 |
| CPU mean / max | 0.760% / 1.400% |
| Adapter GPU mean / max | 22.60% / 69.00% |
| Peak resident RAM including cache | 477.64 MB |
| Open request to first video frame | 141.000 ms |
| A/V desync warning | None |
| Clean exit, renderer freed before mpv | Yes |
| Muted, never activated | Yes |

Cover markers: [('1', '1', '6.017'), ('0', '1', '12.017')]. The position advanced continuously by 6.000 seconds while hidden, with the video track still selected.

GPU readings cover the entire adapter, including other apps. Resident RAM is decimal MB. This short check validates the hide/reveal path, not a five-minute playback benchmark or visible tearing test.

The prior decoder-off design produced 67 drops and an A/V desync warning. That failed design/results are archived in PART2_DECODER_RESTART_RESULTS.md and its original raw result folder. Its vid=no path has been removed; do not present old failed measurements as measurements of this executable.

## Five manual checks

1. **Fullscreen UI/tearing:** play 4K, press F and test every visible control, hover, seek, volume and keyboard. Repeat from maximized; watch for tearing, lag and flashes.
2. **Middle-click/exact return:** from Windowed, Maximized and Mini, middle-click over video, seek, title, buttons and resize edges, then again/Esc. Compare previous position/size/state. Mode logs include geometry and hwndUnchanged.
3. **Mini:** Ctrl+M instantly hides video; playback/time/audio continue. Title buttons and bottom controls remain visible and work. Test horizontal resize, topmost, nested Mini → fullscreen → Mini and restore. Audio continuity is your manual check; automation is muted.
4. **Input/cursor:** video double-click pauses; button double-click does not. Title double-click maximizes/restores. Try the README shortcuts; cursor hides after 2.5 seconds playing and stays visible paused/in Mini.
5. **Rapid transitions/regression:** twenty actual fullscreen toggles, drag/resize, pause/minimize/restore and exit. Try another monitor/DPI if available. Check for persistent stutter, flashes or hangs.

Actual mode transitions, screenshots, visible tearing, audible continuity and physical multi-monitor behavior remain user tests under the explicit minimal-automation instruction. We cannot yet certify that in fullscreen every UI element is visible and working. The single-scene implementation is preserved specifically to keep those controls with the video.
