# Locked user preferences — Part 4 audit

- Playback Left/Right seeks exactly -5/+5 seconds. Home-screen arrows navigate history. Text fields and menus retain their own keys.
- The controls beside play/pause mean previous/next media file, never playback speed. Folder-neighbor navigation is explicitly authorized by this revision; the full playlist panel and auto-advance remain Part 5.
- The key physically above Tab (grave/backtick on US keyboards), the title button and Ctrl+M toggle compact mode.
- Dragging a normal window's top/bottom resize edge down to compact height enters compact mode. Pulling a compact window's vertical resize edge outward expands back to a normal player. Caption dragging still moves the window. Playback/decoder/audio remain untouched.
- Reproduce design/references/player-controls.png and welcome-page.png closely: compact clean geometry, drawn icons rather than miscellaneous text glyphs, four right-hand control actions, compact welcome rows. No IINA logo or red annotation border. Keep required metadata accessible without oversized cards.
- Preserve one native window and one Qt Quick scene. Static gradients, no blur/shadows, no continuous animations or paint timers; cache icons and defer image/DB work to workers.
- Audit all Part 4 requirements after the outage; preserve reports of failed/unverified performance checks. Do not claim perfection or a visual pass without evidence.
