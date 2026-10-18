# Neutral gray UI revision

Latest deployed SHA256: `5fc73d5ba4addfdafb702a97fd3f41d543c4eae2d01285f9ffa362c3a92ac9d4`.

- Final user clarification: neutral iOS-style translucent gray, not blue-gray. All QML hex colors are centralized in UiPalette.qml: title, Home, controls, menus, settings, playlist, tooltips, toasts, information, seek/volume, icons and text. Blue is an active-control accent. Reference saved as design/references/neutral-gray-material.png; no Wi-Fi functionality or blur is copied.
- Bottom controls scale uniformly to0.8: 694×106 becomes555.2×84.8 logical pixels at full width. Narrow layout remains responsive. Fullscreen sits immediately left of More, with both controls retained.
- Fullscreen switches to the exit icon; maximize switches to restore in maximized/fullscreen/Mini states. Exposed maximized state uses the existing window state machine. All actions retain the same native window and scene.
- Cursor menus extend down from the cursor and scroll to available space; only the last80px at the bottom require upward placement. Bottom-button menus explicitly open upward. Popups remain within the scene.

Validation was kept practical. Release build/deployment passed. A five-second requested playback smoke (7.96s external process lifetime including deployment/startup/shutdown) used isolated settings/history, muted and never activated. nvdec, zero drops in that brief run, clean exit, no QML ReferenceError/TypeError/resource errors. The in-process popup check instantiated context11/audio5/subtitle12/speed8/aspect6 entries and Settings; sameScene=true. No native mode changes or desktop input were sent.

The earlier layout check used hash7089a793... (same compact bar geometry before the final color changes). Widths288/416/555.2 all passed volume/transport/actions separation, with positive seek widths187/347/521 in authored unscaled units. Additional intermediate builds reflect the user's ongoing color clarification and popup edge placement; they are not broad performance passes.

Final smoke's first updated-deployment launch was1992.822ms external,422ms from application entry; open→frame119ms. No normal-launch timing or RAM/CPU/performance certification was repeated for this UI-only task. The short zero-drop counter does not resolve the prior12-drop pause/resume finding,509.61MB resident peak, tearing or other acceptance limitations in PART5_RESULTS.md.

Manual check: reopen the player, confirm neutral-gray title/Home/bottom/menu/settings/playlist colors, smaller bar, fullscreen directly before More, changing maximize/restore/fullscreen icons and cursor-anchored context menus. Visual/native-mode interaction is left to the user; no screenshot or physical fullscreen pass is claimed.

Open `E:\Codes\FlowPlayer\out\FlowPlayer\FlowPlayer.exe`. Evidence: tests/results/ui-gray-smoke/{summary.json,flowplayer.log}; earlier geometry evidence: tests/results/ui-polish-smoke/flowplayer.log.
