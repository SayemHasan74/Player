MASTER BRIEF: "FlowPlayer", an IINA-style media player for Windows 11

ROLE
You are a senior Windows C++ media-player engineer. You are building FlowPlayer, a clone of the IINA experience (macOS) for Windows. IINA does not exist on Windows and I want it. I only want the features listed in this brief, built extremely well. Work autonomously: install/verify the toolchain, build, run, measure and test. Ask me only if truly blocked. Never claim something works unless you ran it and tested it.

TARGET MACHINE
Windows 11 x64, NVIDIA RTX 3070, 16 GB RAM.

PRIORITIES (in this order)
1. Zero stutter everywhere: playback, window drag, window resize, fullscreen enter/exit, mini mode enter/exit, auto-advance between files.
2. Every UI element and every input works identically in windowed, maximized, fullscreen and mini modes.
3. Very low CPU, GPU and RAM use. Paused or minimized must be about 0% CPU.
4. Plays everything mpv can play: 4K, 8K, HDR, 10-bit, any container and codec.

THE PROBLEM I HAVE FAILED TO SOLVE IN EVERY PREVIOUS PLAYER (READ CAREFULLY)
My earlier players worked in windowed mode, but in true fullscreen the video painted on top of everything and the UI (buttons, seek bar, menus) disappeared or stopped responding. Cause: the video was rendered in a separate native window (mpv --wid / child HWND / a separate video surface) and the UI was a different window or layer on top. On Windows this is the "airspace" problem: a native video surface is always above other layers, so the UI gets hidden or dead input.
THE FIX, WHICH IS THE CORE OF THIS ENTIRE PROJECT: video and UI must live in ONE window, ONE render surface, ONE scene. The video is a texture rendered by mpv into the same scene graph as the UI. Fullscreen is simply that same window made borderless and monitor-sized. No second window, no overlay, no layer. If you ever find yourself creating a second window or embedding a native video HWND, STOP: you are re-creating my old bug.

STRICTLY FORBIDDEN (never do these, even if they seem easier)
- mpv --wid, child HWNDs for video, DirectShow/WMF/MediaFoundation players, QtMultimedia, VLC/libvlc.
- A second window, transparent/layered overlay window, WS_EX_LAYERED tricks, or controls drawn in a separate window over the video.
- Qt Widgets, Qt WebEngine, Electron, WPF, WinForms, any web view.
- Heavy effects: blur, drop shadows, ShaderEffect over the video, continuous animations.
- Substituting any locked technology below without asking me first.

LOCKED TECHNOLOGY
- Language/build: C++20, CMake, MSVC (Visual Studio 2022 Build Tools). Dependencies fetched by a script or vcpkg.
- UI: Qt 6, newest LTS (verify the latest; 6.8 LTS or newer). Modules: Core, Gui, Qml, Quick only. Fully custom lightweight QML; if any Qt Quick Controls are used, use the "Basic" style.
- Engine: libmpv (libmpv-2.dll + headers + import lib; get a current Windows dev build from the shinchiro or zhongfly mpv builds). Use the RENDER API (mpv_render_context, API type opengl). Base reference: mpv-examples/libmpv/qt_opengl.
- Graphics: force OpenGL: QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL) before any window is created. Threaded render loop. Vsync on (swap interval 1).
- Video item: custom QQuickFramebufferObject inside the same QQuickWindow as all controls. Controls are normal QML items stacked above it.
- Storage: SQLite (amalgamation, WAL mode).
- Pin and record exact versions in the README.

THREADING RULES
- mpv events on their own thread (mpv_set_wakeup_callback + mpv_wait_event loop). The UI thread NEVER blocks on mpv. Use mpv_command_async, mpv_observe_property and queued signals to QML.
- mpv_render_context_render only on the render thread with the GL context current. Render only when mpv's render-update callback fires. No polling timers for rendering. Call mpv_render_context_report_swap after each swap. Destroy the render context on the render thread before mpv_terminate_destroy.
- Render at physical pixels (logical size x devicePixelRatio). Set flip_y correctly.
- Disk writes (SQLite, thumbnails) on worker threads only.

MPV OPTIONS (set explicitly at init, config=no so no user mpv.conf interferes)
- Core: vo=libmpv, idle=yes, keep-open=yes (the app handles end-of-file), terminal=no, load-scripts=no, ytdl=no, osc=no, osd-level=0, input-default-bindings=no, input-vo-keyboard=no, input-cursor=no, cursor-autohide=no, resume-playback=no, save-position-on-quit=no.
- Decode: hwdec=nvdec,d3d11va-copy with automatic software fallback; hwdec-codecs=all; vd-lavc-dr=yes. Confirm in mpv's log which hwdec is really active and tell me.
- Light video defaults: scale=bilinear, cscale=bilinear, dscale=bilinear, correct-downscaling=no, deband=no, interpolation=no, video-sync=audio. A "High quality" setting (off by default): scale=ewa_lanczossharp or spline36, cscale=spline36, video-sync=display-resample, interpolation=yes.
- HDR: tone-map to SDR (tone-mapping=bt.2390, hdr-compute-peak=no). Dolby Vision plays via base layer.
- Cache: cache=yes, demuxer-max-bytes=150MiB, demuxer-max-back-bytes=50MiB, demuxer-readahead-secs=20 (network URLs: 300MiB).
- Audio: ao=wasapi, audio-exclusive=no, audio-channels=auto-safe, volume-max=150.
- Subtitles: sub-auto=fuzzy, sub-file-paths=Subs:subs:Subtitles:Sub, slang/alang from settings (default en,eng), sub-ass=yes, blend-subtitles=no.

RESOURCE BUDGET (measure and report; fix anything over)
- Paused or minimized: about 0% CPU, no render frames, no wakeups.
- 4K60 HEVC 10-bit with nvdec: CPU under about 5% total, zero dropped frames in 5 minutes.
- RAM: under about 150 MB for 1080p, under about 300 MB for 4K (excluding the capped demuxer cache).
- Cold start to visible window under 500 ms; first video frame within about 300 ms for local files.
- Keep the deployed folder small: trimmed windeployqt (no translations, no unused plugins/QML modules, no ANGLE, no software rasterizer, no D3D compiler).
- UI lightness: only 120-150 ms opacity fades for showing/hiding controls; no animations when idle; use Loader so panels (playlist, settings, home) are not instantiated until needed.

THE COMPLETE FEATURE LIST (so the architecture anticipates all of it from day one)
1. Frameless window, custom title bar, native-feeling smooth drag and resize, Windows 11 Snap Layouts.
2. Fullscreen: the same window made borderless. Middle mouse button ANYWHERE toggles it. A second middle-click returns to the exact previous state.
3. Mini mode ("tilted/compact" button): the window shrinks to a small floating window showing ONLY the title bar and the seek bar. Video keeps playing (audio never interrupts). Pressing the button again restores the exact previous state. Works from windowed, maximized and fullscreen. Middle-click still toggles fullscreen from mini mode and returns to mini mode.
4. Double-click on the video = play/pause (the UI also has a play/pause button).
5. IINA-style home screen: last 10 played videos with thumbnails, progress, "time left"; click resumes.
6. Resume: every file remembers its position. Play A 10 min, close, play B, later reopen A: it resumes at A's position. TV series episodes resume where I left off after closing and reopening the player.
7. TV series: open any episode in a folder; the folder becomes the playlist (natural sort). When the episode ends, the next one plays automatically. Next/Previous work from any random episode and open the real neighbor.
8. Subtitles (libass: SRT, ASS/SSA, VTT, PGS, VobSub, embedded and external) and audio track selection.
9. Keyboard, mouse wheel and drag-and-drop input; media keys; single instance; sleep prevention; SMTC.
NOT NOW (do not build): yt-dlp/streaming sites, online subtitle download, equalizer, video filters/shaders, installer, themes.

PROJECT STRUCTURE (create in Part 1, fill in over the parts)
src/core (MpvController, MpvRenderItem, PlaybackModel)
src/window (FramelessWindow, NativeEventFilter, WindowStateMachine)
src/data (HistoryDb, ResumeStore, FileIdentity, ThumbnailService)
src/playlist (FolderScanner, NaturalSort)
src/platform (SingleInstance, SleepInhibitor, Smtc)
src/ui (Main.qml, TitleBar.qml, ControlBar.qml, SeekBar.qml, HomeScreen.qml, PlaylistPanel.qml, MiniBar.qml, Toast.qml, SettingsPanel.qml)
tests/, README.md, CMakeLists.txt, scripts/ (dependency fetch + build)

HOW WE WORK
- I will give you ONE part at a time (PART1.md ... PART5.md). Do only that part. Do not start the next part until I say so.
- Inside each part: build, run, test every listed test, and report results with real numbers (CPU %, RAM, dropped frames). Say plainly what passed, what failed, and what you could not test.
- If a test fails, fix it inside the same part. Never skip ahead and never weaken a test.
- Do not refactor away the single-window architecture, ever. Do not change a locked decision without asking me.
- Keep the code clean and modular so later parts plug in without rewrites. Add a short CHANGELOG.md entry at the end of each part.
- Regression rule: at the end of each part, re-run the key tests of all earlier parts (fullscreen UI works, drag/resize smooth, playback has no dropped frames).

Reply to this brief with a short confirmation of: the root cause of my old bug, the forbidden methods, and the architecture. Do not write code yet.