# Locked visual references

User instruction, 2026-10-18: retain these two images as the visual prompt for the later UI part. Do not implement the UI now.

User clarification, 2026-10-18: do not use or copy the IINA logo. FlowPlayer will have a different, original logo. Match the rest of the reference appearance as closely as possible within the locked Windows/Qt Quick architecture. Do not create the new logo until requested.

Part 2 was authorized on 2026-10-18: implement its title bar, compact transport/seek controls and temporary toasts using Image 1. The welcome page, recent-file UI and final Part 3 polish remain deferred. Every control stays in the video window's existing Qt Quick scene, including fullscreen and Mini.

Part 3 is now authorized: use Image 1's floating rounded control layout, centered transport, left volume and right action group for the dark minimal control surface. The user-approved Mini revision retains the same controls and hides only video. The welcome/recent-file page remains for a later part; the IINA logo is not used.

- **Image 1 — `player-controls.png`: player UI and playback controls.** Reference the rounded floating control bar, volume control, central transport controls, timeline, time labels and right-side action group. The red rectangle is an annotation highlighting that group, not a requested UI border.
- **Image 2 — `welcome-page.png`: welcome page.** Reference the left branding column and the larger right panel with opening actions and recent files.

Use these saved originals when the user authorizes the UI part. Preserve the locked Qt Quick/libmpv single-scene architecture and the existing project brief. The screenshots are visual references; depicted controls, shortcuts, version numbers and macOS window decorations are not independent feature requirements or instructions to change the Windows architecture. Resolve later UI details against the user's instructions for that part.
