"""Summarize the approved hide-only Mini smoke without certifying visual tests."""
import hashlib,json,pathlib,re
ROOT=pathlib.Path(__file__).resolve().parents[1]
CASE=ROOT/'tests/results/part2-warm-mini-20261018'
s=json.loads((CASE/'summary.json').read_text(encoding='utf-8-sig'))
log=(CASE/'flowplayer.log').read_text(encoding='utf-8-sig')
markers=re.findall(r'MINI_VIDEO_COVER hidden=(\d) track=(\S+) position=([\d.]+)',log)
checks={
 'stateUnitTests':'100% tests passed' in (ROOT/'tests/part2-unit-results.txt').read_text(encoding='utf-8-sig'),
 'hideAndReveal':len(markers)==2 and [m[0] for m in markers]==['1','0'],
 'timelineContinues':len(markers)==2 and float(markers[1][2])-float(markers[0][2])>=5.5,
 'videoRemainsSelected':'VIDEO_TRACK=no' not in log and all(m[1]!='no' for m in markers),
 'zeroDroppedFrames':s['mpvLast']['dropped']==0,
 'noAvSyncWarning':'Audio/Video desynchronisation detected' not in log,
 'cleanExitRenderFreedFirst':s['cleanExit'] and log.index('mpv_render_context_free')<log.index('mpv_terminate_destroy on event worker'),
 'mutedAndNonActivating':s['mutedThroughoutSamples'] and s['neverActivated'],
}
ram=s['workingSetMiB']['max']*1048576/1e6
result=dict(checks=checks,shortSmokePassed=all(checks.values()),manualVisualAcceptance='pending user tests',executableSha256=s['executableSha256'],rawDroppedFrames=s['mpvLast']['dropped'],peakWorkingSetMB=ram,coverMarkers=markers)
(CASE/'basic-checks.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
lines=['# Part 2 current verification — Mini hides video, keeps playback running','',
 'The user approved this change on 2026-10-18; prompt2-amendments.md supersedes the original vid=no/title-and-seek-only Mini requirements. Part 3 is not started.','',
 '## Current implementation','',
 '- One existing native window, Qt Quick scene, framebuffer video item and mpv engine in every mode. Fullscreen does not create a video or overlay HWND.',
 '- Mini hides video behind an ordinary scene item. Decoding, audio and the playback timeline continue unchanged. No vid=no, track reselection, seek or pause occurs when entering/leaving Mini.',
 '- Mini starts at 560 × 160 logical pixels, resizes horizontally, and retains the full title button row and bottom playback/volume/seek controls. It is always on top. The height accommodates the requested controls. Tiny preview remains off by default.',
 '- OpenGL, threaded rendering, physical-pixel FBOs, swap interval 1 and swap reporting remain enabled. No render polling timer.','',
 '## Automated basic results','',
 '- State-model tests passed, including 100 random sequences of 200 actions, direct/nested exact restoration, geometry/screen/DPI snapshots and twenty model fullscreen toggles. CTest output is in part2-unit-results.txt. These are model tests, not actual desktop transitions.',
 '- One short muted non-activating packaged HEVC 4K60 Main 10/AAC smoke hid and revealed the same video scene item. No desktop fullscreen, topmost, minimize, move or resize was automated.','',
 f"Executable SHA-256: `{s['executableSha256']}`.",'',
 '| Measurement | Result |','|---|---:|',
 f"| Wall time including startup | {s['wallSeconds']:.3f} s |",
 f"| Video played | {s['mpvLast']['position']:.3f} s |",
 f"| Decoder | {s['mpvLast']['hwdec']} |",
 f"| Dropped frames | {s['mpvLast']['dropped']} |",
 f"| CPU mean / max | {s['cpuTotalPercent']['mean']:.3f}% / {s['cpuTotalPercent']['max']:.3f}% |",
 f"| Adapter GPU mean / max | {s['adapterGpuPercent']['mean']:.2f}% / {s['adapterGpuPercent']['max']:.2f}% |",
 f'| Peak resident RAM including cache | {ram:.2f} MB |',
 f"| Open request to first video frame | {s['timings']['openToFirstVideoSwapMs']:.3f} ms |",
 '| A/V desync warning | None |','| Clean exit, renderer freed before mpv | Yes |','| Muted, never activated | Yes |','',
 f"Cover markers: {markers}. The position advanced continuously by {float(markers[1][2])-float(markers[0][2]):.3f} seconds while hidden, with the video track still selected.",'',
 'GPU readings cover the entire adapter, including other apps. Resident RAM is decimal MB. This short check validates the hide/reveal path, not a five-minute playback benchmark or visible tearing test.', '',
 'The prior decoder-off design produced 67 drops and an A/V desync warning. That failed design/results are archived in PART2_DECODER_RESTART_RESULTS.md and its original raw result folder. Its vid=no path has been removed; do not present old failed measurements as measurements of this executable.','',
 '## Five manual checks','',
 '1. **Fullscreen UI/tearing:** play 4K, press F and test every visible control, hover, seek, volume and keyboard. Repeat from maximized; watch for tearing, lag and flashes.',
 '2. **Middle-click/exact return:** from Windowed, Maximized and Mini, middle-click over video, seek, title, buttons and resize edges, then again/Esc. Compare previous position/size/state. Mode logs include geometry and hwndUnchanged.',
 '3. **Mini:** Ctrl+M instantly hides video; playback/time/audio continue. Title buttons and bottom controls remain visible and work. Test horizontal resize, topmost, nested Mini → fullscreen → Mini and restore. Audio continuity is your manual check; automation is muted.',
 '4. **Input/cursor:** video double-click pauses; button double-click does not. Title double-click maximizes/restores. Try the README shortcuts; cursor hides after 2.5 seconds playing and stays visible paused/in Mini.',
 '5. **Rapid transitions/regression:** twenty actual fullscreen toggles, drag/resize, pause/minimize/restore and exit. Try another monitor/DPI if available. Check for persistent stutter, flashes or hangs.','',
 'Actual mode transitions, screenshots, visible tearing, audible continuity and physical multi-monitor behavior remain user tests under the explicit minimal-automation instruction. We cannot yet certify that in fullscreen every UI element is visible and working. The single-scene implementation is preserved specifically to keep those controls with the video.']
(ROOT/'tests/PART2_RESULTS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
manifest=dict(part='2',part2Accepted=False,manualAcceptance='pending',automatedBasicPassed=all(checks.values()),executableSha256=s['executableSha256'],report='tests/PART2_RESULTS.md',qt='6.12.0',msvc='19.44.35228',libmpv='dde0671072',miniPolicy='hide-only; decoding and audio continue',sourceSha256={})
for p in sorted((ROOT/'src').rglob('*')):
 if p.is_file():manifest['sourceSha256'][str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
(ROOT/'out/FlowPlayer/build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
if not all(checks.values()):raise SystemExit(1)
