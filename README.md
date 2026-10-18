# FlowPlayer — Windows media player

Windows 11 x64. C++20 / CMake / MSVC 2022. Video is rendered by libmpv's OpenGL render API into a `QQuickFramebufferObject`, which Qt Quick composites with plain QML controls inside the same `QQuickWindow`. No native child video window or overlay window exists.

**Parts 1 and 2 are implemented; Part 2 visual/transition acceptance is pending.** See [Part 2 results](tests/PART2_RESULTS.md) for current measurements, limitations and manual checks. [Part 1 results](tests/RESULTS.md) describe the earlier executable, including its two VP9 drops. No Part 3 functionality is implemented.

User revisions on 2026-10-18: resident RAM below **500 MB** is accepted, including cache; the earlier 150/300 MB limits are superseded. A first-deployment-only startup exception is accepted; subsequent fully closed launches must stay below 500 ms. All five manual checks were reported passed. Their original executable hash is retained in `tests/manual-results.json`. No elevated RAM profiling is needed.

## Exact dependencies

| Component | Pin |
|---|---|
| Qt | 6.12.0 LTS, MSVC 2022 x64, package 6.12.0-0-202609280346 |
| MSVC | VS 2022 v143, toolset directory 14.44.35207, compiler 19.44.35228 |
| Windows SDK | Microsoft.Windows.SDK.CPP and .CPP.x64 10.0.26100.3916; headers 10.0.26100.0 |
| CMake | 3.31.6 |
| Ninja | 1.11.1.4 |
| Git verified on development machine | 2.53.0.windows.3 |
| libmpv | shinchiro 20261017 x86_64 dev, mpv git dde0671072 |
| FFmpeg (test generation only) | shinchiro N-127602-g2d49d0e62 |
| 7-Zip extractor | 7zr 26.04; exact binary SHA-256 in dependencies.lock.json |
| Python bootstrap | CPython 3.12.14; exact Python packages in scripts/python-tools.lock.txt |
| NVIDIA GPU / driver tested | RTX 3070 8 GiB / 617.42 |

Qt 6.12 is the newest LTS as verified from [Qt's release announcement](https://www.qt.io/blog/qt-6.12-released) and [supported releases](https://doc.qt.io/qt-6/qt-releases.html). Windows SDK packages are Microsoft's official NuGet packages. The SDK is project-local; no IDE substitution is needed.

Qt application modules are Core, Gui, Qml and Quick. `Qt6::OpenGL` is also linked for `QOpenGLFramebufferObject`, the implementation dependency required by the locked `QQuickFramebufferObject` architecture in Qt 6. It supplies no alternate UI or player. No Quick Controls are used.

## Build

PowerShell, from `E:\Codes\FlowPlayer`:

```powershell
# Bootstrap a project-local Python environment with Python 3.12:
python -m venv .tools
.\.tools\Scripts\python.exe -m pip install -r .\scripts\python-tools.lock.txt
.\.tools\Scripts\python.exe .\scripts\fetch-deps.py
.\scripts\build.ps1 -Target FlowPlayer
.\scripts\run.ps1 -File 'E:\path\to\video.mkv'
# Produce the standalone folder:
.\scripts\deploy.ps1
# Run the standalone application:
.\out\FlowPlayer\FlowPlayer.exe 'E:\path\to\video.mkv'
```

`build.ps1` defaults to the VS 2022 installation at `E:\Applications\VisualStudio\2022\BuildTools`; pass `-VsRoot` for another VS 2022 C++ Build Tools installation. MSVC and Git can be installed with Microsoft's Visual Studio Build Tools installer (Desktop C++ workload) and the official Git for Windows installer. The build checks fail instead of silently changing compilers.

The script makes an MSVC import library from the libmpv DLL's exported C functions, configures Release with Ninja, and builds. Use `-Clean` after restoring source files with older timestamps so stale objects cannot be reused. Archive URLs and SHA-256 values are recorded in `scripts/dependencies.lock.json`; subsequent downloads must match those values.

Spike target:

```powershell
.\scripts\build.ps1 -Target FlowPlayerSpike
$env:PATH = "$PWD\deps\Qt\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$PWD\deps\Qt\plugins"
$env:FLOW_SPIKE_LOG = "$PWD\tests\spike.log"
.\build\FlowPlayerSpike.exe '.\tests\media\hevc-4k60-main10.mkv'
```

## Input and window behavior

- Command-line path, file drag/drop, Ctrl+O native Windows picker; Space and the QML button toggle pause.
- Native caption/video drag and edge/corner resize; Windows maximize hit target supports Snap Layouts.
- Aspect locking defaults on; `playerWindow.lockAspectRatio` is writable and persisted under `HKEY_CURRENT_USER\Software\FlowPlayer\FlowPlayer\window`. A complete settings panel is reserved for a later part.
- Opening a video fits its display dimensions within 80% of the work area and centers it. Normal geometry is remembered.
- Minimizing pauses playback and restores the previous pause state on restore, to meet the Part 1 idle budget. This behavior needs user review if background audio is desired later.

## Threading and lifetime

The event worker creates/initializes libmpv, drains events on wakeup notifications, copies property values into queued UI deliveries, and destroys the handle only after the window and its render resources have been destroyed. UI commands use `mpv_command_async`. The render thread owns the OpenGL render context and frees it before handle termination. FBO sizes are already physical pixels from Qt. Video frame updates originate from mpv callbacks; a replaced FBO is repainted even while paused to avoid stale/empty content after a resize. There is no render timer. Swap notifications use the render-thread `frameSwapped` signal.

Closing sends an asynchronous stop while the window can still service render updates. After mpv reports idle, the window closes and Qt destroys the renderer. This avoids destroying the presentation surface underneath active playback. Native minimum track sizes are expressed in physical pixels; native-size auto-fit converts video pixels to logical window units at the current screen DPI.

Logging uses a worker and rotates at 8 MiB with four backups in `%LOCALAPPDATA%\FlowPlayer\logs`. Each launch starts a new current log. Initialization option failures are logged and treated as errors. Decoder selection is logged as `ACTIVE_HWDEC=` and in mpv's `vd`/`vo` messages.

Additional light defaults: `osd-font-provider=none` prevents font discovery for the disabled mpv OSD; subtitles retain their normal font provider. `hwdec-threads=1` retains the configuration of the fully tested package; restoring the default concurrency did not fix the VP9 failure. `gpu-hwdec-interop=cuda` loads only the zero-copy interop needed by NVDEC; `d3d11va-copy` and software output still use ordinary texture uploads and remain in the fallback chain. `cache-secs=20` makes the requested 20-second readahead effective with `cache=yes`; `demuxer-donate-buffer=no` enforces the 50 MiB back-cache cap independently. The original byte caps remain unchanged. These settings follow the [mpv option semantics](https://mpv.io/manual/master/).

libmpv is delay-loaded on the event worker after the first window swap. QML is compiled ahead of time and initialized after the same dark window has presented its first frame. The window is a `QQuickWindow`; its engine and scene are owned by that window. The scene graph and OpenGL context persist while minimized. Final destruction still frees the render context before terminating libmpv.

During active playback the application asks Windows 11 to honor existing timer-resolution requests even when the muted window is occluded. This prevents Windows from automatically ignoring those requests; automatic power management is restored on pause/minimize. It introduces no polling loop or higher process priority. See [Microsoft's process power-throttling documentation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation).

## Benchmarks

```powershell
.\.tools\Scripts\python.exe .\scripts\benchmark.py .\tests\media\hevc-4k60-main10.mkv --seconds 310 --name T1-hevc
.\.tools\Scripts\python.exe .\scripts\benchmark.py .\tests\media\h264-1080p60.mkv --seconds 310 --name T2-h264
.\.tools\Scripts\python.exe .\scripts\benchmark.py .\tests\media\vp9-4k30.webm --seconds 310 --name T2-vp9
.\.tools\Scripts\python.exe .\scripts\benchmark.py .\tests\media\hevc-4k60-main10.mkv --seconds 75 --name T6-pause-background --scenario pause
# Full sequential automated pass (approximately eighteen minutes):
.\scripts\validate-part1.ps1 -RunName your-unique-run-name -StopOnPlaybackFailure
```

`FLOW_METRICS` enables a test-only 10-second metrics timer. It is absent in ordinary runs. `FLOW_QUIT_MS` enables bounded test exit. Timing markers start at entry to `main`, so include Qt/app initialization but exclude loader time; external launch measurements are needed for full startup timing. Warm operating-system cache is reported separately from a true cold boot. GPU data from nvidia-smi is whole-adapter utilization, not isolated player utilization. Working-set and private memory include cache and decoder allocations; they cannot be presented as cache-excluded memory without additional accounting.

Every benchmark and exit-test launch sets `FLOW_TEST_MUTE=1` and `FLOW_TEST_BACKGROUND=1`. Background windows cannot activate, stay at the bottom, and have no taskbar entry. Tests do not change saved window geometry. Each benchmark records the executable SHA-256, mute state, activation state, raw memory, and actual packet-cache estimates. A background window still renders; minimizing or hiding it would invalidate a playback benchmark. Background timings measure its first swap, not foreground visibility. No test flushes the OS cache or changes the user's desktop. Minimize/restore, dragging, resizing and Snap acceptance are manual.

## Five manual acceptance checks

Keep these checks muted. To launch a normal, interactive window for your own testing:

```powershell
$env:FLOW_TEST_MUTE = '1'
Remove-Item Env:FLOW_TEST_BACKGROUND -ErrorAction SilentlyContinue
.\out\FlowPlayer\FlowPlayer.exe '.\tests\media\hevc-4k60-main10.mkv'
```

1. **Open and overlay:** open via command line, Ctrl+O and file drop. Confirm the title stays above moving video; click the QML Pause/Play button and press Space. Each must change playback immediately.
2. **Drag and resize:** during 4K playback, drag the empty video area, then aggressively resize every edge and corner for sixty seconds. Report flicker, black borders, stale/stretching frames, or delayed input. Muted testing cannot establish audible continuity.
3. **Aspect and size:** open a small native-resolution clip and a 4K clip. Check centering and the 80% work-area cap. Aspect lock defaults on. With the app closed, `Set-ItemProperty HKCU:\Software\FlowPlayer\FlowPlayer\window -Name lockAspectRatio -Type String -Value false` disables it for the next launch; use `true` to restore it. Test on a scaled monitor if available.
4. **Windows integration:** hover over Maximize for Snap Layouts; snap using screen edges and check maximize/restore. The same QML controls must still work.
5. **Lifecycle:** pause for sixty seconds, resume, minimize for sixty seconds, restore, and check that playback resumes only if it was playing before minimize. Close and reopen to check remembered geometry. Report hangs or interrupted restoration. This is the remaining interactive portion of T6.

## References and scope

Render integration follows [mpv-examples/libmpv/qt_opengl](https://github.com/mpv-player/mpv-examples/tree/master/libmpv/qt_opengl) and its [Qt Quick FBO example](https://github.com/mpv-player/mpv-examples/tree/master/libmpv/qml), adapted for event-thread ownership and render-thread teardown. The Qt Widgets host in the former is not used. The official libmpv headers are authoritative for render-context timing and lifetime.

Stub modules reserve the complete requested structure. History, playlists, SMTC, the welcome page and later UI panels remain unimplemented until their authorized parts. Part 2 fills in mini/fullscreen state transitions and basic controls.

## Part 2: modes and input

The same `FramelessWindow`/HWND, mpv controller, render context and QML scene serve Windowed, Maximized, Fullscreen and Mini. Fullscreen uses Qt's `showFullScreen()` plus the current monitor's full native rectangle; it does not request an exclusive display mode. Mini uses native z-order changes rather than recreating the window through Qt flags. Geometry is recorded in Qt logical coordinates with screen name and DPI in each state snapshot. If the original monitor disappears or its work area shrinks, restoration clamps to an available screen.

The title bar and temporary transport bar follow `design/references/player-controls.png`; no IINA logo is used. The welcome page and final Part 3 styling are deferred. Video, title bar, seek bars, info and toasts are ordinary items in this one scene in every mode.

| Input | Action |
|---|---|
| Middle-click anywhere, F, fullscreen button | Toggle fullscreen and restore the previous mode/geometry |
| Esc | Exit fullscreen to the previous mode |
| Ctrl+M, compact/restore button | Toggle Mini and restore the previous mode/geometry |
| Ctrl+T, pin | Always on top; Mini is always on top regardless |
| Space; double-click video/empty area | Pause/play; buttons and title bar do not toggle pause on double-click |
| Double-click title background | Maximize/restore (restore Mini/fullscreen first) |
| Left/Right | Seek 5 seconds |
| Shift+Left/Right | Seek 1 second exactly |
| Ctrl+Left/Right | Seek 30 seconds |
| Up/Down; wheel outside seek bar | Volume +/-5 |
| Wheel over seek bar | Seek +/-5 seconds per notch |
| M | Mute |
| [ / ]; Backspace | Speed +/-0.1; reset 1.0 |
| Comma / period while paused | Frame back/forward |
| I | Resolution, codec, decoder, FPS, dropped-frame count and bitrate |
| Ctrl+O; file drop; command line | Open a file |

Mini is initially 420 x 64 logical pixels at the work area's bottom-right and remembers its position/width. Only horizontal resize is enabled. Its title and seek bar remain interactive; the same video item stays alive underneath an opaque plain-QML cover. With an audio track selected, `vid=no` disables video decoding while the audio timeline continues; restoring reselects the saved video track without issuing an application seek or pausing audio. mpv may refresh-seek and discard frames while bringing video back into sync. Silent files retain hidden decoding because disabling their only selected stream would stop playback. The default-off tiny-preview setting keeps video decoding enabled. With the app closed, enable it using:

```powershell
Set-ItemProperty HKCU:\Software\FlowPlayer\FlowPlayer\window -Name tinyPreview -Type String -Value true
```

The cursor inactivity timer is single-shot, 2.5 seconds. It is stopped while paused, in Mini, or after the pointer leaves. There is no render polling timer. The original swap interval remains 1; visible tearing and transition smoothness must still be checked on the actual display.

Build and run Part 2 from `E:\Codes\FlowPlayer`:

```powershell
.\scripts\build.ps1 -Target FlowPlayer
.\scripts\build.ps1 -Target WindowStateMachineTests
$env:PATH = "$PWD\deps\Qt\bin;$env:PATH"
.\.tools\Scripts\ctest.exe --test-dir build --output-on-failure
.\scripts\deploy.ps1
$env:FLOW_TEST_MUTE = '1'
.\out\FlowPlayer\FlowPlayer.exe 'E:\path\to\video.mkv'
```

See `tests/PART2_RESULTS.md` for actual results and the compact manual checklist. The Part 1 report describes the earlier packaged executable, not this new build. No Part 3 work is included.
