# Caption button, transparency and welcome size — 2026-10-23

Deployed SHA256: `b292b3c4bcfeaf9bf41c08ddc75959bfc6dc6ad382a0aa79cb3aab7a67793f91`.

User authorized the saved Pending UI Preferences and added compact IINA-like startup sizing, replacement deployment, removal of the old executable and GitHub push. The user clarified the square button displayed a little box without maximizing; the previous fix was not considered resolved.

## Implementation

- Preserve Windows HTMAXBUTTON for Snap hover. Forward non-client left press/release/double-click to corresponding client mouse messages on the SAME HWND, with screen→client coordinate conversion. Qt owns normal mouse capture/grab and the QML button click. Removed independent native capture/direct-release toggling, so there are not two competing activation paths. Hover also updates Qt. [Microsoft's Snap guidance](https://learn.microsoft.com/windows/apps/desktop/modernize/ui/apply-snap-layout-menu) requires HTMAXBUTTON for a custom maximize target; the [Qt framelesshelper implementation](https://github.com/wangwenx190/framelesshelper/blob/main/src/core/framelesshelper_win.cpp) demonstrates NC→client forwarding. No additional window/control surface or new dependency was introduced.
- Top/bottom bar alpha179/255 (~70% opaque, ~30% transparent), previously227/255 (~89% opaque). Dark neutral-gray RGB and pale text remain; no blur/shadow/blue tint. Other menu surfaces retain their opacity and the bottom scale remains0.64 (~444x68).
- Default no-file, auto-resume-off startup Home:560x360 logical pixels, centered and bounded by the monitor work area. Playback/command-line launches keep their existing video auto-fit behavior; Home inside an existing fullscreen player does not force a resize.

## Bounded checks

| Check | Result |
|---|---|
| Release build and standalone deployment | Passed |
| Native NC press → NC release → QML click | One activation; duplicate release does not activate again |
| Native NC press → client release → QML click | One activation |
| Release outside button | No activation |
| FinalPass regressions | Arrow seeks and volume100→105→100 kept controls hidden; mouse movement revealed; Escape in URL text focus paused and requested minimize without auto-resume |
| Layout |240/336/444.16 widths: no group overlap, positive seek widths; same-scene menus/Home |
| Standalone default Home | Read-only Win32 client-rectangle/DPI measurement560x360; Home loaded, no QML/resource errors |
| Focus/exit | Muted, non-activating background checks; clean exits; isolated history/settings/thumbnails/logs |
| Old executable cleanup | Removed FlowPlayer-previous-a561ff6bf03f43b6bb2bed51c9e90891.exe after confirming no FlowPlayer process was running. Canonical FlowPlayer.exe retained; desktop shortcut refreshed to that path |

The final input smoke requested10.5s; the Home smoke requested4s. The background caption-forwarding diagnostic uses in-process Qt mouse events instead of taking Windows mouse capture, and maximize/minimize requests are suppressed to avoid changing the user's desktop. Therefore these checks verify event→QML activation and policy, not visible native maximization, Snap flyout correctness or physical resize. The user should click the square button twice after launch and confirm maximize/restore plus Snap hover; appearance/transparency is also a manual check. No desktop input was injected.

An earlier short development check used hash4507fd76... before compact-startup sizing; the accepted input and Home checks both use the final hash above. No five-minute playback, RAM profiling or soak was repeated. Prior pause/resume dropped frames and performance limitations in PART5_RESULTS.md are not re-certified by this UI pass.

Evidence: tests/results/pending-ui-final/{summary.json,logs/flowplayer.log}; tests/results/pending-ui-welcome/{summary.json,logs/flowplayer.log}. Existing source/report history is preserved. Application settings/history were not deleted during replacement deployment.
