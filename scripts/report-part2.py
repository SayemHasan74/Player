"""Report the bounded Part 2 smoke; never certify unperformed visual checks."""
import hashlib, json, pathlib, statistics
ROOT = pathlib.Path(__file__).resolve().parents[1]
CASE = ROOT / 'tests/results/part2-final-basic-20261018'
def read(path): return path.read_text(encoding='utf-8-sig')
s = json.loads(read(CASE/'summary.json'))
samples = json.loads(read(CASE/'samples.json'))
log = read(CASE/'flowplayer.log')
t = s['timings']
first = (t['externalLaunchToFirstSwapMs']-t['FIRST_SWAP_MS']+t['FIRST_VIDEO_SWAP_MS'])/1000
phases = {}
for label, lo, hi in [('video',first+1,first+5), ('video disabled',first+7,first+11), ('paused',first+19,first+21)]:
    rows = [v for v in samples if lo <= v['seconds'] <= hi]
    phases[label] = dict(samples=len(rows), **{key: statistics.mean(v[key] for v in rows) if rows else None for key in ('cpuTotalPercent','adapterGpuPercent','adapterDecoderPercent')})
checks = {
    'stateUnitTests': '100% tests passed' in read(ROOT/'tests/part2-unit-results.txt'),
    'qmlLoaded': t.get('QML_LOADED_MS',0)>0,
    'firstVideoFrame': t.get('FIRST_VIDEO_SWAP_MS',0)>0,
    'vidNoObserved': 'VIDEO_TRACK=no' in log,
    'videoRestored': 'disabled=0' in log and s['mpvLast']['videoTrack']=='1',
    'cleanExitRenderFreedFirst': s['cleanExit'] and log.index('mpv_render_context_free')<log.index('mpv_terminate_destroy on event worker'),
    'mutedAndNonActivating': s['mutedThroughoutSamples'] and s['neverActivated'],
    'noAvSyncWarning': 'Audio/Video desynchronisation detected' not in log,
}
ram = s['workingSetMiB']['max']*1048576/1e6
result = dict(implementationChecks=checks, manualAcceptance='pending user tests',
              playbackAcceptance='not established; drops observed after video restore',
              executableSha256=s['executableSha256'], phases=phases,
              rawDroppedFrames=s['mpvLast']['dropped'], peakWorkingSetMB=ram)
(CASE/'basic-checks.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
lines = ['# Part 2 implementation and basic verification — 2026-10-18','',
    '**Implementation available; visual and transition acceptance pending.** The user requested minimal automated checks and will perform visual playback tests. No Part 3 work is included.', '',
    'The pre-Part-2 project was pushed to GitHub main at `64391a06c0c719282c8dd5f9af182450bd1e2173` before saving prompt2.md and starting implementation.', '',
    '## Automated results','',
    '- Release application and all four compiled QML components build successfully.',
    '- State tests: 61,341 checks, exact direct/nested restoration, independent Mini geometry, screen/DPI snapshots, twenty rapid state toggles and 100 seeded random sequences of 200 actions with full unwind. CTest: 1/1 passed in 0.02 seconds. These are model tests, not display transitions.',
    '- Final packaged smoke: HEVC 4K60 Main 10 with AAC; 35-second app timer. QML and NVDEC loaded, vid=no was observed, the timeline advanced while video was disabled, the saved video track was restored, pause/resume worked, and shutdown freed the GL renderer before terminating mpv.',
    '- All automated launches were muted and non-activating. No fullscreen, raising, minimizing, moving or resizing of the user desktop was automated.',
    '- The first diagnostic fired before first video presentation on a slow initial launch. The scenario was corrected to arm after first presentation; failed/intermediate evidence remains in its result folders. No repeated long suite was run.', '',
    f"Executable SHA-256: `{s['executableSha256']}`.", '',
    '| Measurement | Result |','|---|---:|',
    f"| Wall time including loader/startup | {s['wallSeconds']:.3f} s |",
    f"| Open request to first video swap | {t['openToFirstVideoSwapMs']:.3f} ms |",
    f"| First window swap after deployment | {t['externalLaunchToFirstSwapMs']:.3f} ms |",
    f"| Process CPU mean / max | {s['cpuTotalPercent']['mean']:.3f}% / {s['cpuTotalPercent']['max']:.3f}% |",
    f'| Peak resident RAM, including cache | {ram:.2f} MB |',
    '| Drops before disabling video | 0 |',
    '| Drops at first paused sample after restoring video | 60 |',
    f"| Raw final dropped frames | {s['mpvLast']['dropped']} |", '',
    'The final counter is retained without resets or subtraction. mpv logged a video-track refresh seek about one second behind the audio clock when video was reselected, followed by an **Audio/Video desynchronisation detected warning**. Mini restore therefore has an unresolved playback regression; it is not an acceptance pass. Seven further drops appeared after pause/resume. Adapter GPU reached 99% and includes other applications; no cause is established for every drop. The transient resident peak exceeds the revised 500 MB limit. No additional RAM profiling was performed.', '',
    '### Short resource samples','',
    '| Phase | Samples | CPU mean | Adapter GPU mean | Adapter decoder mean |',
    '|---|---:|---:|---:|---:|']
for label,p in phases.items():
    lines.append(f"| {label} | {p['samples']} | {p['cpuTotalPercent']:.3f}% | {p['adapterGpuPercent']:.2f}% | {p['adapterDecoderPercent']:.2f}% |")
lines += ['', 'These small samples exercise decoder disable/restore without changing window mode. They are not actual Mini/fullscreen display measurements. Adapter GPU figures include other apps.', '',
    '## Architecture and scope','',
    'One existing QQuickWindow/HWND, engine, render context and scene serve all modes. Fullscreen uses showFullScreen plus the full monitor rectangle, without exclusive resolution switching. Mini changes the same window size/z-order; a plain QML item covers the existing framebuffer item. Controls remain above video in that scene. No video HWND or overlay window was added.', '',
    'With selected audio, Mini applies vid=no and reselects the saved video track on restore without an application seek or audio pause. Silent files retain hidden decoding to preserve their only playback clock; this is an explicit resource-saving exception. Tiny preview defaults off. OpenGL, threaded rendering, physical-pixel FBOs, swap interval 1 and swap reporting remain enabled. No render polling timer was added.', '',
    'Visible tearing, black flashes and audible continuity are not independently verified. It is not yet verified that **in fullscreen, every UI element is visible and working**. That statement requires the manual test below. Earlier Part 1 manual results apply to an earlier executable.', '',
    '## Five manual checks (T2–T9)','',
    '1. **Fullscreen/UI:** play 4K, press F, click play/pause, scrub, hover and change volume; use keyboard shortcuts. Watch for tearing, lag and black flashes. Repeat from maximized. Capture windowed/fullscreen screenshots if useful.',
    '2. **Exact return:** from Windowed, Maximized and Mini, middle-click over video, seek bar, title, buttons and resize edges. Toggle again/Esc; compare size, position and state. Mode logs include geometry and hwndUnchanged in %LOCALAPPDATA%/FlowPlayer/logs/flowplayer.log.',
    '3. **Mini/nesting:** Ctrl+M from each mode; title+seek only, horizontal resize, remembered position, always-on-top, seeking, wheel and Space. Mini → fullscreen → Mini must restore exactly. Check audible continuity yourself if desired; automation stays muted. Try a silent clip and optional tiny preview.',
    '4. **Input/cursor:** video/empty-area double-click pauses; title double-click maximizes/restores; button double-click does not pause. Try the README key map. Cursor hides after 2.5 seconds playing, stays visible paused/in Mini, returns on movement.',
    '5. **Rapid transitions/regression:** twenty actual fullscreen toggles during 4K, then drag/resize, pause/minimize/restore and exit. Check persistent stutter, black borders and responsiveness. Try another monitor/DPI if available. No physical multi-monitor test was performed here.', '',
    'Manual display tests, screenshots, actual transition HWND stability, 20 real fullscreen toggles and a long idle regression were deliberately not automated under the user\'s latest testing instructions. Full acceptance is not claimed.']
(ROOT/'tests/PART2_RESULTS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
manifest = dict(part='2',part2Accepted=False,executableSha256=s['executableSha256'],report='tests/PART2_RESULTS.md',qt='6.12.0',msvc='19.44.35228',libmpv='dde0671072',sourceSha256={})
for p in sorted((ROOT/'src').rglob('*')):
    if p.is_file(): manifest['sourceSha256'][str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
(ROOT/'out/FlowPlayer/build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
