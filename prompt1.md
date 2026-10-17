PART 1 OF 5: FOUNDATION

GOAL
A solid base: toolchain working, libmpv rendering video INSIDE a Qt Quick scene with hardware decoding on the RTX 3070, in a frameless window that drags and resizes with zero lag. Everything later depends on this being perfect.

TASKS
1. Toolchain: verify or install Visual Studio 2022 Build Tools (C++ workload), CMake, Git, Qt 6 (newest LTS) with MSVC kit, and download a libmpv dev build. Pin exact versions in README. Write scripts/ so the build is reproducible. Create the full project structure from the brief with empty/stub files where needed.
2. Spike first: a minimal Qt Quick window playing a local file through libmpv's render API. Base it on mpv-examples/libmpv/qt_opengl. Confirm from mpv's log (msg-level vd=v, vo=v) which hwdec is active. nvdec is expected on this machine. If the OpenGL interop fails, fall back to d3d11va-copy, then software, and tell me which one works here and why.
3. Real implementation:
   - MpvController: owns the mpv_handle, applies ALL options from the brief, runs the event thread, exposes properties to QML (position, duration, pause, volume, mute, speed, title, tracks, hwdec-current, dropped frame count) via queued signals, accepts commands via async calls. Never blocks the UI thread.
   - MpvRenderItem: QQuickFramebufferObject with the render API. Renders only on mpv's update callback. Physical-pixel sizing, correct flip_y, swap reporting, clean teardown order.
   - Main.qml: the video item fills the entire window. A temporary minimal play/pause button and a title label sit on top as plain QML items, to PROVE the overlay-on-video architecture works.
4. Frameless window:
   - Remove the standard frame with a native event filter: WM_NCCALCSIZE, WM_NCHITTEST (resize zones about 6 px on all edges and corners; HTMAXBUTTON over the maximize button for Windows 11 Snap Layouts; HTCAPTION over the title-bar drag area), WM_ERASEBKGND returns 1.
   - DwmExtendFrameIntoClientArea (1 px margin), Windows 11 rounded corners (DWMWA_WINDOW_CORNER_PREFERENCE), DWMWA_USE_IMMERSIVE_DARK_MODE, no white flash on startup (set a dark background before first show).
   - Dragging: native move (startSystemMove or HTCAPTION). Left-drag on empty video area also moves the window (like IINA). Aero Snap works.
   - Resize: native edge hit-testing; the video re-renders at the new size EVERY frame during the resize loop (no waiting for mouse release); no black borders, flicker, tearing or stretched stale frames.
   - Lock-aspect-ratio (default on) via WM_SIZING so the window keeps the video's aspect ratio when resizing by hand. Setting can turn it off.
   - On opening a file, auto-fit the window to the video's native size, capped at 80% of the monitor work area, centered. Remember window geometry in settings.
5. Basic open: command-line file argument, drag-and-drop a file onto the window, Ctrl+O file dialog (native Windows dialog is fine). Space toggles pause. Nothing else yet.
6. Logging to %LOCALAPPDATA%/FlowPlayer/logs with rotation. A clean shutdown (no crash on exit, render context freed on the render thread first).

TESTS (run all; report results with numbers)
T1. 4K60 HEVC 10-bit file: report the hwdec in use, dropped frames over 5 minutes, CPU %, GPU %, RAM.
T2. 1080p H.264 file: same metrics. Plus an AV1 or VP9 4K file if available.
T3. Overlay proof: the QML button and title label render above the video and respond to clicks while video plays.
T4. Aggressive window drag and edge/corner resize for 60 seconds during 4K playback: no flicker, no black borders, no lag, audio uninterrupted.
T5. Windows 11 Snap Layouts appear when hovering the maximize button; snapping to edges works.
T6. Pause for 60 seconds: about 0% CPU. Minimize for 60 seconds: about 0% CPU.
T7. Cold start time to visible window (target under 500 ms) and first video frame time.
T8. Exit cleanly 20 times in a row with no crash or hang.

DONE WHEN
All tests pass and are reported with numbers. Give me the exact commands to build and run, and tell me the 5 things you want me to test by hand. Do not start Part 2. Save this as prompt1.md, then read it, work on it.
