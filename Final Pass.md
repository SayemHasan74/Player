# Final Pass

User preferences recorded on 2026-10-19. Implementation authorized by the subsequent request to fix these together with maximize/restore and the additional control-bar size reduction.

## Escape: pause and minimize

- Pressing **Escape**, regardless of the player's current state, must pause playback and minimize the player to the Windows taskbar.

## Keyboard actions must not reveal hidden controls

- During normal playback, the UI stays hidden when it has auto-hidden.
- Seeking forward or backward with the arrow keys must perform the seek without revealing the UI.
- Adjusting volume up or down with the arrow keys must change the volume without revealing the UI.
- The UI should reappear only when the mouse moves.

## Maximize / restore button

- Fix the square title-bar button so one click maximizes and a second click restores the saved normal geometry reliably.
- Pair native press/release correctly, including captured client-area releases; dragging away or losing capture must not activate the button.
- Preserve Windows Snap Layouts and the single-window/single-scene architecture.

## Additional bottom-bar size reduction

- Make the entire bottom UI another 20% smaller than the current version, including buttons, icons, text, sliders and spacing.
- The cumulative scale is 0.8 × 0.8 = 0.64 of the original, approximately444.16×67.84 logical pixels at full width.

## Implementation status

The changes are implemented together. Validation scope and any remaining manual checks are recorded in tests/FINAL_PASS.md. Escape takes priority over menus and text fields and keeps playback paused after taskbar restore.
