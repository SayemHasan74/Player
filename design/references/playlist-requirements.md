# Locked playlist reference and behavior — 2026-10-18

User-provided screenshot: [playlist-panel.png](playlist-panel.png). Apply when the playlist part is authorized; do not start Parts 4/5 merely to implement this reference.

- A playlist button in the bottom control bar opens a panel from the **right side**, sliding over the player inside the existing Qt Quick scene.
- The panel overlays video and controls. It must not push, shrink, reflow or move the video or the other UI. No additional native window or video surface.
- Follow the screenshot's right-side playlist layout and visual organization. Retain the project's lightweight styling rules: no blur/shadows or continuous idle animation. Opening/closing motion is a user-authorized short transition; settle without ongoing animation.
- List playable files from the current folder, show their names, identify the current file, and show which entries have been played and each entry's saved progress. Make the full filename accessible, including long names; do not rely solely on permanently truncated labels.
- Keep persistent history and an independent resume position for **every played file**, including partially played files. Switching entries must save the outgoing file's current position before restoring the incoming file's own saved position.
- Clicking a playlist entry opens that file at its last saved position rather than restarting it. This also applies when returning to earlier episodes and after closing/reopening the player; history must not be limited to the current file or session.
- Required example: A and B are each partially played; C is currently playing. Opening the playlist and choosing A resumes A at A's saved position. Choosing B resumes B at B's saved position. C retains its own position independently.
- Keep this behavior and the panel's controls working in Windowed, Maximized, Fullscreen and Mini/restore without changing the single-window/single-scene architecture. Preserve the latest Mini hide-only playback behavior.

The screenshot is a visual reference. Its sample filenames, branding and macOS decorations are not new requirements. The prohibition on copying the IINA logo still applies. Persistent history/resume belongs with the authorized storage work; playlist navigation/panel belongs with the playlist part.
