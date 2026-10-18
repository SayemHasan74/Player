"""Generate the acceptance report from preserved bounded-test evidence."""
import pathlib,json,re,statistics,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1];results=ROOT/'tests/results'
core=json.loads((results/'part5-release/summary.json').read_text());cases=json.loads((results/'part5-release/cases.json').read_text())
perf=json.loads((results/'part5-final-performance/summary.json').read_text());samples=json.loads((results/'part5-final-performance/samples.json').read_text())
startup=json.loads((results/'part5-normal-startup/summary.json').read_text());exe=ROOT/'out/FlowPlayer/FlowPlayer.exe'
sha=hashlib.sha256(exe.read_bytes()).hexdigest();assert sha==perf['executableSha256']==startup['executableSha256']
text=(results/'part5-final-performance/flowplayer.log').read_text(encoding='utf-8')
marks={r['phase']:r for line in text.splitlines() if 'PART5_PHASE ' in line for r in [json.loads(line.split('PART5_PHASE ',1)[1])]}
offset=perf['timings']['externalLaunchToFirstSwapMs']-perf['timings']['FIRST_SWAP_MS']
phases=[]
for name in ['hidden','playlist','paused','resumed','covered']:
    begin,end=marks[name+'_begin'],marks[name+'_end'];selection=[s for s in samples if (begin['elapsedMs']+offset)/1000<=s['seconds']<=(end['elapsedMs']+offset)/1000]
    r={'state':name,'seconds':(end['elapsedMs']-begin['elapsedMs'])/1000,'renders':end['renders']-begin['renders'],'swaps':end['swaps']-begin['swaps'],'addedDrops':end['dropped']-begin['dropped'],'samples':len(selection)}
    for key in ['cpuTotalPercent','adapterGpuPercent']:
        values=[s[key] for s in selection if key in s];r[key]=statistics.mean(values) if values else None
    phases.append(r)
peak=max(s['workingSetMiB'] for s in samples)*1048576/1e6
folder=sum(p.stat().st_size for p in exe.parent.rglob('*') if p.is_file())
resumeDrops=marks['resumed_begin']['dropped']-marks['paused_end']['dropped']
package={'part':5,'fullAcceptance':False,'executableSha256':sha,'integrationSha256':core['executableSha256'],'integrationCases':core['cases'],'integrationPassed':core['passed'],'unitTargetsPassed':4,'performanceSeconds':perf['wallSeconds'],'peakResidentMB':peak,'resumeAddedDrops':resumeDrops,'normalLaunchMs':startup['timings']['externalLaunchToFirstSwapMs'],'openToFirstVideoMs':startup['timings']['openToFirstVideoSwapMs'],'deployedBytes':folder,'phases':phases,'notTested':['visible fullscreen/drag/resize/tearing/ASS/mouse reorder/SMTC hardware input','physical device loss/disconnected drive/deletion during playback','5-minute zero-drop acceptance','60-second minimized/paused acceptance','8K or 4K AV1','one-hour soak']}
(ROOT/'tests/part5-package.json').write_text(json.dumps(package,indent=2),encoding='utf-8')
rows='\n'.join(f"| {r['state']} | {r['seconds']:.3f} | {r['cpuTotalPercent']:.3f}% | {r['adapterGpuPercent']:.1f}% | {r['renders']} / {r['swaps']} | {r['addedDrops']} |" for r in phases)
report=f'''# Part 5 results — implementation present; full acceptance NOT passed

Muted, non-activating background checks only. No screenshots, native mode changes, focus activation, desktop dragging/resizing or interruption of an existing player. Latest user instruction limits testing to practical important checks; the full long suites were not run. No user history/settings were modified.

Final deployed executable: `{sha}`. The ten integration cases used `{core['executableSha256']}`. The only subsequent application changes were QML: move the playlist input layer below the 38 px title bar so title buttons stay accessible, and clear drag state before reordering destroys a delegate. The final hash was rebuilt/deployed and used for the final 40-second panel/playback check and normal-startup check. Integration results are not silently transferred between hashes. Current CTest 4/4 passed in 2.39 s.

## Requirement/test results

| Test | Result | Evidence / numbers |
|---|---|---|
| T1 auto-advance | Logic passed; visual modes manual | Open episode 5 of 10 → episode 6; saved 8.000 s loads with start=6.000 s; episode 5 persisted finished=1. No native mode toggles performed. |
| T2 neighbors / boundaries | Passed in process | 5→6→5→10→1, no boundary wrap; Previous after 8 s seeks to 0 without loading a neighbor. Windows natural sort, mixed case, leading zeros, extras filtering and stable model: 11 checks. |
| T3 per-file resume | Passed in process | A→B→A loads A at 10.123 s after its 12.123 s save; audio=2, subtitle=2, volume=61, speed=1.25 restored. Cross-launch episode-6 start=6.000 s from saved 8.000 s. |
| T4 playlist | Logic/scene passed; visual/manual pending | Ten folder rows, real current index 4, Loader in same scene; custom [2,9] reordered to [9,2], navigation opens 9. Full names wrap, watched/progress implemented. Actual mouse drag/fullscreen click checks remain manual. |
| T5 single instance | Passed for background IPC | Second process exits 0; original instance loads episode 1 then episode 8. Foreground activation and fullscreen preservation require manual check. |
| T6 SMTC / sleep | Initialized/API transitions passed; hardware manual | SMTC_READY on all 10 cases; successful display-inhibit on/off calls logged. Real unfocused hardware keys, flyout and physical sleep remain manual. |
| T7 bad files | Partial pass | Zero-byte + corrupt/unsupported data + missing path skip forward to episode 3; clean exits and no QML/SQL errors. Physical disconnected drive, deletion during playback and forced GPU reset not performed. |
| EOF settings | Passed | Last episode → Home by default; stayOnLastFrame=true → paused episode 10, home=0. |
| T8 4K60 HEVC 10-bit | FAILED full acceptance | nvdec; 40-second bounded run. Steady intervals below have 0 added drops, but pause→resume added {resumeDrops}. No five-minute certification. |
| T8 window/input/subtitle/home | Manual / retained scoped evidence | Four unit targets include prior mode restoration/settings/history tests. Native fullscreen UI, middle/double click, title/resize Mini gestures, tearing, ASS styling and history mouse behavior are manual. Earlier reports retain their own hashes. |
| T8 idle | Short interval only | About 5 s paused: 0 renders and 0 swaps. No 60 s minimized/paused test in this pass. |
| T8 startup | Normal launch passed | Fully closed unchanged launch {startup['timings']['externalLaunchToFirstSwapMs']:.3f} ms externally; Qt-entry first swap {startup['timings']['FIRST_SWAP_MS']} ms; open→first video {startup['timings']['openToFirstVideoSwapMs']} ms. Launch→first video {startup['timings']['FIRST_VIDEO_SWAP_MS']} ms from Qt entry. |
| T8 first updated deployment | Accepted exception, retained | {perf['timings']['externalLaunchToFirstSwapMs']:.3f} ms external, {perf['timings']['FIRST_SWAP_MS']} ms from application entry. File loading also began late; open→frame still {perf['timings']['openToFirstVideoSwapMs']} ms. |
| T8 deployment | Measured | {folder:,} bytes = {folder/1e6:.2f} MB / {folder/1048576:.2f} MiB; mpv DLL { (exe.parent/'libmpv-2.dll').stat().st_size/1e6:.2f} MB. 17 DLLs, no Widgets/WebEngine/Multimedia, no translation/software GL/TLS plugin deployment. |
| T8 8K / 4K AV1 | Not tested | No suitable fixture; no codec limit inferred. Earlier VP9 evidence is not a current pass. |
| T9 one-hour soak | Not run | User explicitly requested practical bounded testing; no stability/memory-growth claim for one hour. |

## Current 4K state measurements

GPU is whole-adapter utilization and includes the user's other apps. CPU is process CPU divided by 16 logical CPUs. These short samples are not a GPU frame-time or visual tearing measurement. “covered” is the presentation-only Mini cover without moving/resizing the desktop window; it is not a native Mini-mode test.

| State | Seconds | CPU mean | GPU mean (adapter) | Video renders / scene swaps | Added drops in interval |
|---|---:|---:|---:|---:|---:|
{rows}

Pause→resume transition added **{resumeDrops} drops before resumed_begin**; the resumed interval's zero does not erase those drops. Final counter={perf['mpvLast']['dropped']}. Previous pre-title-input-fix run also added 12 at resume; no decoder/scaler/cache option was changed to hide the failure. Zero additional drops during the open-playlist interval is useful evidence but does not certify five-minute stability or visible transitions.

## Master resource budget

| Item | Result |
|---|---|
| CPU <about 5% while 4K playing | Passed in short sampled states above; whole-run steady-sample mean {perf['cpuTotalPercent']['mean']:.3f}%, maximum {perf['cpuTotalPercent']['max']:.3f}% |
| Zero drops for five minutes | Not passed: 12 at pause/resume; five-minute run omitted |
| Resident RAM <500 MB including cache (user revision) | Slightly over: peak {peak:.2f} MB ({peak/1.048576:.2f} MiB). Original 150/300 MB budgets superseded. No elevated profiling or working-set trimming performed. |
| Paused/minimized ~0% CPU, no video renders | Short paused numbers above; minimized/60-second acceptance manual, not certified |
| Normal visible window <500 ms | Passed once at {startup['timings']['externalLaunchToFirstSwapMs']:.3f} ms; first updated deployment exception retained |
| Local open→video about300 ms | Passed {startup['timings']['openToFirstVideoSwapMs']} ms on normal launch; full launch→frame is reported separately |
| Lightweight scene | Callback-driven rendering, VSync=1, one scene; only finite panel/opacity animations. Stable playlist rows; no normal-runtime diagnostics timer or SMTC/sleep polling. Hidden interval has one scene swap per video frame. |
| Deployment trimmed | {folder/1e6:.2f} MB; only needed Qt runtime/QML/Windows/JPEG components plus the large pinned mpv DLL |

## Five manual checks

1. Open episode 5, press **L**: right panel slides over video/controls without moving them. Read a long full name, click another item, hold/drag to reorder, check progress/watched marks. Verify title buttons remain usable.
2. In fullscreen and Mini/restore, use every control/menu/settings/playlist item; middle-click twice restores the exact state, double-click pauses, above-Tab and vertical resizing enter/leave Mini. Watch for tearing or drag/resize lag.
3. Seek near episode 5's end: 6 starts without a black flash; fullscreen/Mini stay unchanged. Previous after >5 s restarts; within 5 s selects the previous episode. Test the final episode's Home/stay-paused setting.
4. Partially play A and B, switch to C, then choose A/B from playlist; close/reopen and rename/move one. Confirm independent resume and tracks/volume/speed. Check ASS styling/audio continuity across modes and transitions.
5. With the player unfocused, use hardware media keys and Windows media flyout. Open another file through Explorer/second launch while fullscreen. Check display stays awake playing video and the request clears paused. Try the association script only if desired.

## Known limitations

- Pause/resume drop spike and slight resident-budget overrun above remain unresolved; stable visible framerate/tearing is not certified.
- HDR is tone-mapped to SDR; no native HDR output on this OpenGL path. Dolby Vision depends on a playable base layer.
- Mini keeps decoding/audio warm by explicit user preference; minimizing pauses for idle efficiency.
- Thumbnail work defers during active playback; concurrent thumbnail/4K certification is not claimed.
- Physical GPU loss, disconnected shares and OS media-key routing require manual checks. Network filesystem timeouts affect a worker, not the UI thread, but may delay resolution. Session reordering is intentionally not persisted.
- Five-minute, one-hour, physical interaction and unavailable codec/bitmap-subtitle fixtures are untested in this practical pass. Historical manual passes apply only to their recorded older executable.

Sources: tests/results/part5-release/cases.json; part5-final-performance/{{summary,samples,flowplayer.log}}; part5-normal-startup/summary.json; part5-ctest-final.log. Regenerate with `scripts/report-part5.py` after preserving these inputs.
'''
(ROOT/'tests/PART5_RESULTS.md').write_text(report,encoding='utf-8')
print(json.dumps(package,indent=2))
