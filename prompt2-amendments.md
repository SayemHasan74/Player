# Part 2 amendments — 2026-10-18

The user's later instructions take precedence over the original prompt2.md.

- Keep video and audio playing normally in Mini, including video decoding. Mini instantly hides the video using a plain item in the existing scene; do not apply vid=no, unload/reselect video tracks, pause playback or seek when entering/leaving Mini.
- Preserve the title bar with minimize, extend/restore, fullscreen, pin and close buttons, always-on-top behavior, and the bottom playback/control/seek bar. Mini is therefore taller than the original 64-pixel title-and-seek-only design.
- Smooth continuous playback takes priority over saving decoder resources. Video preview stays off by default; normal video is revealed again on restore without a decoder restart.
- Keep one native window, one render surface and one Qt Quick scene in all states. No separate fullscreen video layer.
- Keep automated checks minimal, muted and non-activating in the background. User performs visual playback/tearing/transition tests. Do not begin Part 3.
