"""Render acceptance evidence without discarding failures or changing thresholds."""
import argparse, hashlib, json, pathlib, statistics, sys, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
RESULTS=ROOT/'tests/results'
parser=argparse.ArgumentParser();parser.add_argument('--prefix');args=parser.parse_args()
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
names=([f'{args.prefix}-T1-hevc',f'{args.prefix}-T2-h264',f'{args.prefix}-T2-vp9'] if args.prefix else
       ['T1-validated','T2-h264-validated','T2-vp9-validated'])
pauseName=f'{args.prefix}-T6-pause' if args.prefix else 'T6-pause-background'
exitName=f'{args.prefix}-T8-exit' if args.prefix else 'T8-restored-timing'
playback=[read(RESULTS/name/'summary.json') for name in names]
pause=read(RESULTS/pauseName/'summary.json')
exits=read(RESULTS/exitName/'summary.json')
manual=read(ROOT/'tests/manual-results.json')
limits=read(ROOT/'tests/acceptance-overrides.json')
current=hashlib.sha256((ROOT/'out/FlowPlayer/FlowPlayer.exe').read_bytes()).hexdigest()
hashes={d['executableSha256'] for d in playback+[pause]}|{e['executableSha256'] for e in exits}
idle=pause['pauseInterval']
startup=[e['launchToFirstSwapMs'] for e in exits]
startupToCheck=startup[1:] if limits.get('allowFirstDeploymentStartupDelay',False) else startup
checks={
    'singleTestedExecutable':hashes=={current},
    'hevcFiveMinutesZeroDrops':playback[0]['mpvLast']['position']>=300 and playback[0]['mpvLast']['dropped']==0,
    'allPlaybackNoDrops':all(d['mpvLast']['dropped']==0 and d['cleanExit'] for d in playback),
    'hevcCpuBelowFivePercent':playback[0]['cpuTotalPercent']['max']<5,
    'ramWithinUser500MBLimit':all(d['workingSetMiB']['max']*1048576<limits['residentRamLimitBytes'] for d in playback),
    'pauseSixtySecondsIdle':idle['durationMs']>=59900 and idle['renderedFrames']==0 and idle['mpvCallbacks']==0 and idle['cpuMeanTotalPercent']<0.1,
    'twentyCleanExits':len(exits)==20 and all(e['exitCode']==0 and e['renderFreedFirst'] and e['cleanExit'] and e['neverActivated'] for e in exits),
    'startupWithinUserApprovedTarget':max(startupToCheck)<limits['startupTargetMs'],
    'localOpenFirstFrameWithin300Ms':all(d['timings']['openToFirstVideoSwapMs']<limits['firstFrameAfterOpenTargetMs'] for d in playback),
    'manualChecksUserReportedPassed':all(manual[k]=='passed' for k in ('overlayAndInput','fileOpening','dragResizeAspect','snapLayouts','pauseMinimizeRestore')),
    'allAutomatedRunsMutedAndNonActivating':all(d['mutedThroughoutSamples'] and d['neverActivated'] for d in playback+[pause])
}
accepted=all(checks.values())
verdict={'accepted':accepted,'checks':checks,'executableSha256':current,'cases':names,'pauseCase':pauseName,'exitCase':exitName,'manualEvidence':manual,'limits':limits}
verdictPath=RESULTS/(f'{args.prefix}-verdict.json' if args.prefix else 'verdict.json')
verdictPath.write_text(json.dumps(verdict,indent=2)+'\n')
lines=['# Part 1 acceptance report','',f"**{'ACCEPTED' if accepted else 'NOT ACCEPTED — remaining failures below'}.** No Part 2 implementation.",'',
       f'Packaged executable SHA-256: `{current}`. Automated test hashes: '+', '.join(f'`{h}`' for h in sorted(hashes))+'.','',
       'Machine: Windows 11 Pro build 26300, Core i5-13400F (16 logical processors), RTX 3070 8 GiB, NVIDIA driver 617.42.','',
       '## User revisions and manual results','',
       '- On 2026-10-18 the user accepted resident RAM below **500 MB**, overriding the original 150/300 MB budgets. The limit below includes the cache; no cache subtraction or working-set trimming is used.',
       '- The user reported all five manual checks passed: overlays/input, file opening, drag/resize/aspect, Snap Layouts and pause/minimize/restore.',
       f"- Manual evidence applies to executable `{manual['executableSha256']}`; it is user-reported, not an automated observation of later performance changes.",
       '- All automated runs are muted, non-activating bottom windows. No automated desktop drag, resize, Snap or minimize/restore is performed. Muted testing cannot establish audible continuity.','',
       '## Playback measurements','',
       '| Clip | Seconds played | Decoder | Drops | CPU mean / max | Adapter GPU mean / max | Resident RAM mean / max (MB) |',
       '|---|---:|---|---:|---:|---:|---:|']
for label,d in zip(['HEVC 4K60 Main 10','H.264 1080p60','VP9 4K30'],playback):
    m=d['mpvLast'];cpu=d['cpuTotalPercent'];gpu=d['adapterGpuPercent'];ram=d['workingSetMiB']
    lines.append(f"| {label} | {m['position']:.3f} | {m['hwdec']} | {m['dropped']} | {cpu['mean']:.2f}% / {cpu['max']:.2f}% | {gpu['mean']:.2f}% / {gpu['max']:.2f}% | {ram['mean']*1048576/1e6:.2f} / {ram['max']*1048576/1e6:.2f} |")
lines+=['','GPU readings cover the entire adapter, including other applications. CPU is process utilization normalized across sixteen logical processors. Resident RAM is the actual total working set in decimal MB. Private/committed memory and packet-cache bytes remain separately recorded in the raw JSON.','',
        'Clips are synthetic motion patterns. HEVC and H.264 repeat twelve-second video patterns with continuous AAC audio. VP9 repeats a six-second pattern and has no audio. This does not establish playback of every possible real-world file.','',
        '## Idle, launch and shutdown','',
        f"- Pause: {idle['durationMs']/1000:.3f} seconds; mean total CPU {idle['cpuMeanTotalPercent']:.4f}%, max {idle['cpuMaxTotalPercent']:.4f}%; {idle['renderedFrames']} video renders and {idle['mpvCallbacks']} mpv render callbacks. Test-only metrics logging was enabled.",
        '- Minimize/restore: user-reported functional pass. No new automated minimize cycle was performed under the background-only restriction. Prior instrumented minimize measurements are retained in earlier result folders; they are not presented as current-binary measurements.',
        f'- First launch after deployment: **{startup[0]:.3f} ms** to window first swap.',
        f'- Subsequent fully closed process launches: {min(startup[1:]):.3f}–{max(startup[1:]):.3f} ms, mean {statistics.mean(startup[1:]):.3f} ms.',
        '- Launch figures include process creation and DLL loading. The window stays behind other applications; first swap is the presentation marker. No OS cache flushing or reboot was performed while the user worked.',
        f"- Clean shutdowns: {sum(e['exitCode']==0 and e['cleanExit'] and e['renderFreedFirst'] for e in exits)}/{len(exits)}, with renderer destroyed before engine termination.",'',
        '| Clip | Open request to first video swap | Launch to first video swap |',
        '|---|---:|---:|']
firstLog=(RESULTS/exitName/'01.log').read_text(encoding='utf-8')
firstVideo=re.search(r'FIRST_VIDEO_SWAP_MS=(\d+)',firstLog)
if firstVideo:
    initialVideoMs=startup[0]-exits[0]['mainToFirstSwapMs']+int(firstVideo[1])
    lines.insert(lines.index('| Clip | Open request to first video swap | Launch to first video swap |'),
        f'- First video after initial deployment: {initialVideoMs:.3f} ms from process launch. This includes initial engine/DLL loading; it is distinct from opening a file once the engine is ready.\n')
if limits.get('allowFirstDeploymentStartupDelay',False):
    lines.insert(3,'The user explicitly accepted a first-deployment-only startup exception. The 500 ms target still applies to subsequent fully closed process launches.\n')
for label,d in zip(['HEVC','H.264','VP9'],playback):
    t=d['timings'];external=t['externalLaunchToFirstSwapMs']-t['FIRST_SWAP_MS']+t['FIRST_VIDEO_SWAP_MS']
    lines.append(f"| {label} | {t['openToFirstVideoSwapMs']:.3f} ms | {external:.3f} ms |")
lines+=['','## Acceptance checks','', '| Check | Result |','|---|---|']
lines += [f"| {key} | {'PASS' if value else 'FAIL'} |" for key,value in checks.items()]
lines+=['','## Changes and reproducibility','',
        '- Uses libmpv render API, OpenGL and the threaded Qt Quick scene. Video and controls share one window and scene; no native video child or separate overlay.',
        '- Windows 11 timer requests are honored during playback even when muted/occluded. Automatic timer power management resumes when paused/minimized. No rendering poll timer was introduced.',
        '- QML is compiled ahead of time. The same dark window presents before its QML scene and mpv initialization are loaded.',
        '- Normal mpv timing is retained. The earlier video-timing-offset=0 experiment failed and was reverted; its raw results remain available.',
        '- Build and run commands, pinned versions and five manual checks are in README.md. Raw per-second samples, logs and hashes are under tests/results.','',
        'This report does not redefine failed thresholds as passes. See the verdict JSON for the exact checks and user-authorized limits.']
(ROOT/'tests/RESULTS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps({'accepted':accepted,'failedChecks':[k for k,v in checks.items() if not v],'verdict':str(verdictPath)},indent=2))
sys.exit(0 if accepted else 1)
