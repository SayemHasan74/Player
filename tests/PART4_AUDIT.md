# Part 4 outage audit and interaction/UI corrections

**Audit completed; perfect/full acceptance is not claimed.** The stored code survived, several real defects were repaired, and the new interaction preferences were implemented. Resume-transition frame drops and real visual/native checks remain open. No full Part 5 panel/auto-advance was started.

## Outage integrity

- Git was clean at audit start; `git fsck --full --no-reflogs` reported no corrupt/missing objects. Dangling objects were left untouched. Deployed executable matched the previous recorded SHA-256 `03912a7bb346bdbadfacd8653aadfe496f0a8822f8bb403af2651488fa8ba774`. This verifies tracked code/deployment, not recovery of unknown unsaved work.
- SQLite amalgamation source/header matched the pinned archive: True. No database existed at the configured user APPDATA/FlowPlayer/flow.db location when audited. There is no pre-outage user history at that location to validate or reconstruct; test histories were kept separate.

## Defects corrected

- JPEG encoding/atomic disk writes no longer hold the mutex used by UI scheduling. The queue lock only protects queue/cancellation state.
- A stop event belonging to the prior thumbnail no longer aborts the next job. Five distinct fresh-cache JPEGs were generated serially and verified as 320×180.
- Recent-file availability/cache checks moved off the database writer to a separate low-priority metadata worker. Slow paths cannot hold the write queue or the identity/open worker. Stale metadata generations are discarded; cached metadata prevents unnecessary placeholder resets.
- Replaying a finished file clears its finished state after meaningful new progress; that new partial viewing resumes independently.
- Safe identity-size failures and promotion of path-only identities preserve fallback behavior. History read failures report an error without trapping file opening.
- Empty-home arrow navigation is guarded. Manual opens cancel delayed auto-resume. Stop/exit cancels pending identity/navigation/load completions and queued render-ready loads, preventing canceled files from restarting behind Home or during close.

- The recent-history view uses a stable QAbstractListModel: thumbnail/availability changes emit row data changes, preserving delegates, selection and pointer grabs. Membership/order changes alone reset the list; a persistent-index unit check covers the update behavior.

## Locked preferences and visual implementation

- Plain Left/Right seeks exactly −5/+5 seconds with exact seeks, independent of the configurable wheel seek step. Home arrows navigate the list; editors/popups keep their keys. Shift/Ctrl seek variants remain.
- Buttons beside play/pause open previous/next media files in natural folder order, through the same resume path. They never call speed adjustment. Speed remains in More → Playback speed.
- The physical key above Tab, grave/tilde, Ctrl+M and the title button toggle compact mode. Native top/bottom-edge resizing collapses a normal window at 200 logical pixels; vertical resizing of Mini past 240 expands to a normal window. Hysteresis prevents flapping. The gesture commits after releasing the resize edge; caption dragging moves the player. Button mode restoration retains exact snapshots.
- Mini still hides video only and retains decoding/audio/title/controls. Gesture transitions issue no engine pause, decoder-reset or seek commands.
- Player controls follow the reference 694×106 floating layout: left volume, centered transport, four right actions and lower timeline. Drawn, cached icons replace glyph substitutes. Stable time-label widths keep the seek track from shifting as time text changes.
- Home follows the reference branding column and teal/purple right area with compact recent rows, opening actions and no IINA logo. Thumbnails/progress remain in rows; full names and folder/time-left/last-played information appear in the focused/hovered detail footer.
- Renderer creation is deferred until a file needs it and retained thereafter. Video, home, menus and controls remain in one native window and Qt Quick scene. No native video child, overlay window, blur, shadow or continuous animation/paint timer was added. The screenshot annotation border is not reproduced.
- Visual pixel-level matching has not been certified from a running-player screenshot; visual tests remain user-operated under the standing instruction.

## Requirement audit

| Part 4 task | Implementation / evidence |
|---|---|
| 1 SQLite | Amalgamation 3.53.4, APPDATA flow.db, WAL/FULL, schema v1, all SQLite on queued writer. WAL/version/retention/reopen tests pass. |
| 2 Identity | Size + first/last 64 KB SHA-256, whole small files, path fallback. Real rename and deny-read lock tested; physical SMB drive not available. |
| 3 Saves | Pause, five-second playback timer, settled seek, replacement, stop, exit; queued single transactions and shutdown drain. Forced kill measured in core suite; no physical power-cut experiment. |
| 4 Resume | Load-time start=position−2; >95%/finished starts at zero; toast and per-file track/delay/speed/volume. Core and replay/keys cases pass. Visual no-jump remains manual. |
| 5 Home | Loader, last ten/newest-first, metadata, thumbnails/progress, continue keyboard focus, context actions, missing state, Mini hiding, auto-resume. Core, empty-home and auto-resume cases pass; actual mouse/fullscreen/context actions remain manual. |
| 6 Thumbnails | Second headless software mpv handle, background priority, serial screenshot-raw → 320×180 JPEG, placeholder/cache. Queue tested. Jobs defer during active playback; strict simultaneous-generation-on-4K T6 remains unverified. |
| 7 Cleanup | 200 retained entries; thumbnail removal on prune/remove verified. Last ten displayed separately. |

## Results and exact scopes

- Current deployed package: `38904d45ac82d9ff835558a5d4aba893aee499cd7aecc9f50c1a2df4d0f42b29`. Seven focused release cases passed on `38904d45ac82d9ff835558a5d4aba893aee499cd7aecc9f50c1a2df4d0f42b29`: finished replay, subsequent resume, ±5-second keys, empty home, previous/next, auto-resume and five-job thumbnail queue. The canceled-open/clean-close check used `38904d45ac82d9ff835558a5d4aba893aee499cd7aecc9f50c1a2df4d0f42b29` and showed position 0 with no video renders.
- Eight initial core audit cases passed on `0da27b2b933a84671fddd36a072261c356a119a073cee37550baa1962d5a8caf` before the later thumbnail/metadata/shutdown corrections. Crash-save age: 2.498 s; core peak resident: 391.88 MB. This older core result is not silently transferred to the final hash.
- CTest: 3/3 passed in 3.24 s, including stable history-row identity, locked fallback, corruption-error fallback, natural neighbors, removal/pruning and compact boundaries/restoration. 1,000 saves: enqueue 1.713 ms, worker drain 2255.964 ms; ten-item query/metadata completion 0.532 ms; unit event-loop max gap 1.971 ms. Event-loop gaps are not GPU frame times.
- Recorded key positions were 12.133 → 7.133 → 12.133 s while a 17-second generic step was stored. Previous/next opened Episode 2 → Episode 1 → Episode 2 → Episode 10; speed stayed 1.0 in those neighbor fixtures.
- Controls at widths 336/496/694 had no volume/transport/action overlap and positive seek widths. Baseline 4K hidden interval: 240 renders/240 swaps, 0 added drops. Paused interval: 0 renders/0 swaps. Both are four-second sampled intervals, not fresh 60-second idle certification.
- The 4K UI baseline used `546cc10ae876e4ae86826f5179dca57a526a5ab4878d63fa49422ce2813929ab`: nvdec, CPU mean 0.404%, adapter GPU mean 10.33% (includes other apps), first window swap 412.83 ms, open→first video swap 129.0 ms. It added **9 dropped frames immediately after resume**, then 0 additional drops over the following four-second phase.
- Later style check `399cfcb5c6f2c51c62a95e760289e7de3762085dc6fab778ec2b5da4aaa14987` ended with **17 drops**; the last scheduled resume-end phase did not occur before auto-quit because initial engine/video startup was delayed. External first window 1229.03 ms; first video 5684 ms from application entry. It is not a full phase pass. These performance builds precede the final metadata/shutdown fixes; no stable-framerate claim is made for the final package.
- A single diagnostic warm-scene-swap experiment still recorded 9 resume drops. It was rejected and removed. mpv timing/decoder options were not weakened to hide drops. Older 408-drop/slow-start and VP9 findings remain historical unresolved evidence.

## Five manual checks

1. Compare the welcome/control layouts to the two saved images. Open each menu and settings in fullscreen; confirm controls remain above video and clickable.
2. Press Left/Right and use the neighboring transport buttons. Confirm ±5 seconds versus previous/next file; speed changes only through its menu/shortcuts or stored per-file settings.
3. Toggle Mini with the key above Tab and title button. Drag the normal top resize edge down and release to collapse; enlarge Mini vertically to expand. Caption dragging should move it. Check audio/video continuity, aspect lock and exact button restoration.
4. Open A/B, change tracks/delay/speed/volume, close/reopen and rename/move A. Check two-second resume/toast, history context actions, Enter/Space and missing-file treatment.
5. Watch 4K motion and especially pause/resume for visible stutter/tearing; test drag/resize, middle-click and end-of-file return. The measured resume-drop failure still requires resolution; no acceptance waiver is assumed.
