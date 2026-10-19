# Final pass — 2026-10-19

Accepted executable SHA256: `9f3d634ac4a7c9989bb543599366ce45061c4b728a8c3685ab37b8fe3b80685c`.

Implemented all requested tweaks together: maximize/restore, Escape pause-and-minimize, keyboard actions without control reveal, and another20% reduction to the entire bottom bar. User preferences are in Final Pass.md.

## Changes

- Maximize: native HTMAXBUTTON press captures input and accepts either client or non-client release exactly once. Release outside/lost capture cancels the action. Hit target comes from the QML visual item tree, because Repeater delegates do not necessarily have QObject ownership beneath the title row. Guard synchronous geometry/state callbacks and keep a separate saved normal rectangle for restore. Snap hit testing remains HTMAXBUTTON.
- Escape: handled before popup, Home and editable-focus guards; dismisses popups, pauses, then minimizes. Restore does not resume automatically. Preserve existing window-state flags when adding Minimized; Home/idle can minimize without video swaps. [Qt window-state behavior](https://doc.qt.io/qt-6.12/qwindow.html#setWindowStates) and [Windows mouse capture](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setcapture) inform these implementations.
- Hidden controls: keyboard activity no longer calls pointerActivity; UI hold/native-leave changes update only the hide timer. Mouse movement reveals controls. Existing paused/Mini visibility rules remain.
- Bottom UI: uniform0.64 scale, including buttons, icons, time labels, seek/volume controls and spacing. Full-size footprint444.16×67.84 logical pixels; narrow layout is retained. This is20% smaller than the previous0.8 version.

## Practical validation

| Check | Result |
|---|---|
| Release build | Passed |
| Window-state unit target | Passed, including capture pairing, duplicate/outside/canceled release and exact restore rectangle assertions; native desktop operations are not performed |
| Hidden arrows | Right:2.983→8.050s; Left:8.050→3.167s while playback continued; controls stayed hidden throughout |
| Volume arrows |100→105→100; controls stayed hidden |
| Mouse movement | Controls revealed immediately in the in-process event check |
| Escape in URL text focus | Popup=true/textFocus=true before; popup=false/pause=true after; minimize request had resumeOnRestore=0 |
| Actual desktop minimize in automation | Suppressed by background test policy, deliberately not performed |
| Maximize hit target | Center=true, outside=false; physical button/window-state behavior remains manual |
| Layout | Widths240/336/444.16: no volume/transport/action overlaps; positive seek widths202/352/521 in authored units; Home/menus remain in the same scene |
| Clean exit / focus / QML errors | Clean exit, never activated, zero QML errors |
| Deployment | On-disk out/FlowPlayer/FlowPlayer.exe matches accepted hash; existing user PID12252 was not closed or restarted |

The accepted input run requested10.5s and used an independent background process, muted, with separate settings/history/thumbnails/logs. Only synthetic events inside that test window were used. No native mouse/keyboard injection, resizing, maximizing, minimizing, foreground activation or interruption of the user's player. An earlier short development check caught the visual-tree lookup issue; it was corrected before the accepted check.

No full playback benchmark, RAM profiling, five-minute acceptance or soak was run. Prior pause/resume drops and other performance limitations remain scoped to tests/PART5_RESULTS.md; this pass does not claim to fix or re-certify them.

## User checks after restart

1. Click the square title button twice: maximize, then restore the original size/position; check Snap hover still appears.
2. In windowed, maximized, fullscreen and Mini, press Escape: playback pauses and the player minimizes. Taskbar restore should leave it paused. Repeat with a menu or URL field open.
3. Let controls hide, use Left/Right/Up/Down, confirm the bar stays hidden; move the mouse to reveal it. Check the uniformly smaller bar and retained fullscreen/More buttons.

Reproduce the bounded check with `scripts/test-final-pass.py --name <new-name>` after building. Background tests isolate logs through FLOW_LOG_DIRECTORY. Executable-only deployment (`scripts/deploy.ps1 -ExecutableOnly`) is for unchanged pinned runtime dependencies; it preserves an already running image and updates the next-launch path. Close/reopen FlowPlayer to load the new build.

Evidence: tests/results/final-pass-accepted/summary.json and logs/flowplayer.log; tests/results/final-pass-unit.log.
