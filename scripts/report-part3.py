"""Report bounded Part 3 checks, retaining pre-seek drops and manual limits."""
import argparse, hashlib, json, pathlib, re, statistics
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--prefix',default='part3-release');args=parser.parse_args()
names=[f'{args.prefix}-{kind}-20261018' for kind in ('controls','tracks','restart')]
def read(path):return path.read_text(encoding='utf-8-sig')
cases=[ROOT/'tests/results'/name for name in names]
summaries=[json.loads(read(p/'summary.json')) for p in cases]
logs=[read(p/'flowplayer.log') for p in cases]
def stages(log):return {v['phase']:v for line in log.splitlines() if 'PART3_STAGE ' in line for v in [json.loads(line.split('PART3_STAGE ',1)[1])]}
control,tracks=stages(logs[0]),stages(logs[1])
seek=[]
for log in logs[:2]:
    seek += [dict(requested=float(a),actual=float(b),error=float(c)) for a,b,c in re.findall(r'EXACT_SEEK target=([\d.]+) actual=([\d.]+) error=([\d.]+)',log)]
popups=json.loads(re.search(r'PART3_POPUPS invoked=1 result=(.+)',logs[0])[1])
phases={}
samples=json.loads(read(cases[0]/'samples.json'))
offset=(summaries[0]['timings']['externalLaunchToFirstSwapMs']-summaries[0]['timings']['FIRST_SWAP_MS'])/1000
for label,start,end in [('visible','visible_begin','visible_end'),('hidden','hidden_begin','hidden_end'),('paused','paused_begin','paused_end'),('quality','quality_begin','quality_end')]:
    a,b=control[start],control[end];lo=a['elapsedMs']/1000+offset;hi=b['elapsedMs']/1000+offset
    selected=[v for v in samples if lo+.15<=v['seconds']<=hi-.15]
    phases[label]=dict(durationSeconds=(b['elapsedMs']-a['elapsedMs'])/1000,videoRenders=b['renders']-a['renders'],sceneSwaps=b['swaps']-a['swaps'],dropsBefore=a['dropped'],dropsAfter=b['dropped'],samples=len(selected))
    for key in ('cpuTotalPercent','adapterGpuPercent','adapterDecoderPercent'):
        values=[v[key] for v in selected if key in v]
        phases[label][key]=statistics.mean(values) if values else None
initial,switched,cover=tracks['tracks_loaded'],tracks['tracks_switched'],tracks['cover_loaded']
external=[str(t.get('external-filename','')).replace('\\','/').lower() for t in initial['tracks'] if t.get('external')]
allSamples=[v for case in cases for v in json.loads(read(case/'samples.json'))]
ram=max(v['workingSetMiB'] for v in allSamples)*1048576/1e6
sha=hashlib.sha256((ROOT/'out/FlowPlayer/FlowPlayer.exe').read_bytes()).hexdigest()
checks={
    'singlePackagedExecutable':all(s['executableSha256']==sha for s in summaries),
    'unitTests':'100% tests passed' in read(ROOT/'tests/part3-unit-results.txt'),
    'sceneMenusAndLazySettings':popups['settingsLoaded'] and popups['sameScene'] and all(popups[k]>0 for k in ('context','audio','sub','speed','aspect')),
    'controlsHideAndStayShownPaused':not control['hidden_begin']['controlsVisible'] and not control['hidden_end']['controlsVisible'] and control['paused_begin']['controlsVisible'],
    'programmaticUiHold':control['visible_begin']['controlsVisible'] and control['visible_end']['controlsVisible'],
    'hiddenNoExtraSceneSwaps':phases['hidden']['sceneSwaps']<=phases['hidden']['videoRenders'],
    'pausedNoRendersOrSwaps':phases['paused']['sceneSwaps']==0 and phases['paused']['videoRenders']==0,
    'exactSeekWithinOne60fpsFrame':len(seek)>=2 and max(v['error'] for v in seek)<=1/60,
    'audioTrackChanged':switched['options']['aid']!=initial['options']['aid'],
    'subtitleAdjustmentsApplied':abs(switched['options']['sub-delay']-.1)<.001 and abs(switched['options']['sub-scale']-1.2)<.001 and switched['options']['sub-pos']==90,
    'audioDelayApplied':abs(switched['options']['audio-delay']-.1)<.001,
    'srtAssVttDecoderEvidence':all(value in logs[1] for value in ('Using subtitle decoder srt','fontselect: (Segoe UI, 700','Using subtitle decoder webvtt')),
    'autoLoadedSubsDirectory':any('/subs/' in p for p in external),
    'noDuplicateSidecars':len(external)==len(set(external)),
    'chapterMetadata':len(initial.get('chapters',[]))==3,
    'embeddedCoverSelected':cover['audioOnly'] and any(t.get('albumart') and t.get('selected') for t in cover['tracks']),
    'volumeAcrossProcessRestart':summaries[2]['mpvLast']['volume']==73,
    'qualityOnIntervalNoAddedDrops':control['quality_end']['dropped']==control['before_quality']['dropped'],
    'pauseResumeNoAddedDrops':control['before_quality']['dropped']==control['paused_end']['dropped'],
    'allObservedResidentRamBelow500MB':ram<500,
    'cleanMutedNonActivating':all(s['cleanExit'] and s['mutedThroughoutSamples'] and s['neverActivated'] for s in summaries),
    'noQmlErrors':not any(text in log for log in logs for text in ('ReferenceError:','TypeError:','is not a type','Cannot assign to non-existent property')),
}
maximumDrops={names[i]:max(v['dropped'] for v in (control if i==0 else tracks).values()) for i in range(2)}
result=dict(part=3,accepted=False,manualAcceptance='pending user tests',checks=checks,executableSha256=sha,cases=names,phases=phases,seekMeasurements=seek,popupChecks=popups,maximumObservedDropsBeforeSeeks=maximumDrops,peakAllSampleResidentMB=ram)
verdict=ROOT/'tests/results'/f'{args.prefix}-verdict.json'
verdict.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
lines=['# Part 3 controls, tracks and settings — measured results','',
 '**Implementation available; full acceptance is not claimed.** Visual/fullscreen/subtitle styling tests remain with the user under the minimal-automation instruction. The checks below retain any measured failures. No Part 4 work.', '',
 f'Packaged executable SHA-256: `{sha}`. All three final process runs used this executable.', '',
 '## Basic checks','', '| Check | Result |','|---|---|']
lines += [f"| {key} | {'PASS' if value else 'FAIL'} |" for key,value in checks.items()]
lines += ['', 'The state-machine and settings CTest targets passed. Settings tests cover defaults, clamping, worker flush and persistence through destruction/recreation. A separate actual application restart recovered volume73. Settings files for tests were isolated from user preferences.', '',
 '## Resource and render samples','',
 '| Phase | Seconds | CPU mean | Adapter GPU mean | Adapter decoder mean | Video renders | Scene swaps | Drops start → end |',
 '|---|---:|---:|---:|---:|---:|---:|---|']
for name,p in phases.items():
    def fmt(value):return 'n/a' if value is None else f'{value:.3f}%'
    lines.append(f"| {name} | {p['durationSeconds']:.3f} | {fmt(p['cpuTotalPercent'])} | {fmt(p['adapterGpuPercent'])} | {fmt(p['adapterDecoderPercent'])} | {p['videoRenders']} | {p['sceneSwaps']} | {p['dropsBefore']} → {p['dropsAfter']} |")
lines += ['', f'Peak resident working set across every raw process sample, including startup/cache: **{ram:.2f} MB**. Adapter GPU data include other applications and are not isolated player utilization. These short intervals do not replace long stability or 60-second idle tests.', '',
 f"HQ sample minus visible normal sample: CPU {phases['quality']['cpuTotalPercent']-phases['visible']['cpuTotalPercent']:+.3f} percentage points; whole-adapter GPU {phases['quality']['adapterGpuPercent']-phases['visible']['adapterGpuPercent']:+.3f} percentage points. These are observed differences under concurrent desktop load, not isolated causal costs of HQ.", '',
 f"Observed pause/resume counters: {control['paused_end']['dropped']} before resume → {control['before_quality']['dropped']} before enabling HQ. HQ-on/playing: {control['before_quality']['dropped']} → {control['quality_begin']['dropped']} → {control['quality_end']['dropped']}. Resume and HQ activation were separated by two seconds. HQ was switched off before the final paused seek; no crash or asynchronous option error was observed, but visual continuity and a separate off-transition counter were not certified.", '',
 'The mpv frame-drop counter resets on seeks/file changes. Final zero values after seeking do not mean earlier playback had zero drops. Maximum observed pre-seek counters: '+str(maximumDrops)+'. A short experiment moving the Windows timer-policy request before unpause did not eliminate the resume drops and was reverted. Its evidence is retained separately; no timing workaround was promoted.', '',
 '## Seeking and media','',
 '| Requested | Observed | Absolute error |','|---:|---:|---:|']
lines += [f"| {v['requested']:.3f}s | {v['actual']:.3f}s | {v['error']:.3f}s |" for v in seek]
lines += ['', 'These are asynchronous position reads after playback-restart while paused, not a claim of visual mouse-drag precision. Dragging, hover tooltip alignment, bitmap subtitles and network buffer visualization remain manual.', '',
 'The generated fixture contains two AAC audio languages, embedded SRT/ASS, a matching sidecar under Subs and three chapters. Switching audio, selecting styled ASS (Segoe UI Bold font logged), loading external WebVTT, subtitle delay/scale/position and audio delay were exercised. This establishes decoder/control operation, not visual correctness of colors, positioning or karaoke. MP3 cover art was selected as an attached video/image track in the same renderer. PGS/VobSub and FLAC samples were unavailable and are not marked passed.', '',
 '## Scene architecture and settings','',
 'Video, title, controls, seek tooltip, toasts, menus, URL editor, info and the Loader-created settings panel are items in the same existing Qt Quick scene. No native video child or popup/overlay window is introduced. Native caption right-click is routed to that scene too. Menu/settings lists scroll in Mini. The user-approved Mini behavior keeps decoding/audio running and hides only video.', '',
 'Auto-hide uses one2.5-second single-shot timer and140ms opacity fades. Hover/menu/drag holds suppress hiding; paused/Mini controls stay shown. Invisible cache-info bindings were removed after an earlier diagnostic revealed extra swaps. The seek throttle runs only during dragging and is not a video-render polling timer.', '',
 'Preferences/volume are stored in %LOCALAPPDATA%/FlowPlayer/settings.ini with worker-thread writes. Defaults: auto-resume off, autoplay-next on, hardware decode on, HQ off, Mini preview off, speed1.0 and seek steps5/1/30seconds. Auto-resume/autoplay actions remain reserved for Parts4/5. Native geometry storage from earlier parts remains separate.', '',
 '## Five manual checks','',
 '1. **Fullscreen/Mini UI:** in each mode, use every button, seek bar, audio/subtitle/speed menus, right-click context menu, URL editor and Settings. Confirm hover/clicks work above video and no tearing/black flashes appear. Menus must stay inside the player.',
 '2. **Seeking/auto-hide:** click and drag4K, check preview/hover time, chapter ticks and buffered network ranges. Confirm exactly one accurate release seek. Wait2.5seconds, then move/hover/open a menu/pause; verify matching cursor/title/control behavior in fullscreen.',
 '3. **Subtitles:** use provided SRT/ASS/VTT fixtures, embedded tracks, S, off/load and subtitle drop; test Z/X, size/position, ASS colors/top text/karaoke through resize/fullscreen/Mini restore. Use your own PGS/VobSub samples if available.',
 '4. **Audio/settings:** cycle A and menu tracks, adjust delay/volume, restart and check memory, open MP3/FLAC art. Toggle HQ/hardware decode live and inspect I. Check languages/default speed/aspect/seek-step/Mini-preview persistence. Auto-resume/autoplay are saved flags for later parts.',
 '5. **Regression/stability:** middle-click over controls/title/resize edges, video double-click versus button double-click, exact mode restoration, twenty fullscreen toggles, dragging/resizing and pause/resume. Pay particular attention to the measured resume drops; stable visible cadence is not certified.','',
 'No screenshots, audible continuity, real fullscreen interactions, physical multi-monitor checks or visual styling checks were performed automatically. Earlier Part1/Part2 manual passes apply to their earlier executables.']
(ROOT/'tests/PART3_RESULTS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
manifest=dict(part='3',accepted=False,manualAcceptance='pending',executableSha256=sha,qt='6.12.0',msvc='19.44.35228',libmpv='dde0671072',report='tests/PART3_RESULTS.md',sourceSha256={})
for p in sorted((ROOT/'src').rglob('*')):
    if p.is_file():manifest['sourceSha256'][str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
(ROOT/'out/FlowPlayer/build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(executableSha256=sha,failedChecks=[k for k,v in checks.items() if not v],phases=phases,seekMeasurements=seek,peakResidentMB=ram,verdict=str(verdict)),indent=2))
