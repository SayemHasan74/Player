"""External process metrics. GPU numbers are adapter-wide (WDDM), explicitly labeled."""
import argparse, json, os, pathlib, re, statistics, subprocess, time, datetime
import psutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('file',type=pathlib.Path)
parser.add_argument('--seconds',type=int,default=310)
parser.add_argument('--name',default='playback')
parser.add_argument('--scenario',default='')
parser.add_argument('--profile',default='')
parser.add_argument('--exe',type=pathlib.Path,default=ROOT/'build/FlowPlayer.exe')
parser.add_argument('--standalone',action='store_true')
parser.add_argument('--settings-file',type=pathlib.Path,help='Explicit isolated settings file for restart verification')
parser.add_argument('--diagnostic-no-cache',action='store_true',help='Diagnostic memory baseline, never an acceptance result')
a=parser.parse_args()
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):
    parser.error('FlowPlayer is already running; refusing to disturb it or share its log.')
if a.scenario in ('idle','lifecycle'):
    parser.error('Minimize/restore is manual while the user works. Use --scenario pause for background idle measurement.')
out=ROOT/'tests/results'/a.name;out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env.update(FLOW_METRICS='1',FLOW_QUIT_MS=str(a.seconds*1000),FLOW_SCENARIO=a.scenario,FLOW_PROFILE=a.profile,FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1')
env['FLOW_SETTINGS_FILE']=str((a.settings_file or (out/'settings.ini')).resolve())
env['FLOW_HISTORY_DB']=str((out/'flow.db').resolve())
env['FLOW_THUMBS_DIR']=str((out/'thumbs').resolve())
if a.diagnostic_no_cache:env['FLOW_DIAGNOSTIC_NO_CACHE']='1'
else:env.pop('FLOW_DIAGNOSTIC_NO_CACHE',None)
if a.standalone:
    for key in ['QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(key,None)
else:
    env['PATH']=str(ROOT/'deps/Qt/bin')+';'+env['PATH']
    env['QT_PLUGIN_PATH']=str(ROOT/'deps/Qt/plugins')
    env['QML_IMPORT_PATH']=str(ROOT/'deps/Qt/qml')
start=time.perf_counter()
startUtc=datetime.datetime.now(datetime.timezone.utc)
process=subprocess.Popen([str(a.exe.resolve()),str(a.file.resolve())],env=env)
p=psutil.Process(process.pid);p.cpu_percent()
samples=[]
nvidia=pathlib.Path('C:/Windows/System32/nvidia-smi.exe')
while process.poll() is None:
    time.sleep(1)
    try:
        memory=p.memory_info()
        row=dict(seconds=round(time.perf_counter()-start,3),cpuTotalPercent=p.cpu_percent()/psutil.cpu_count(),workingSetMiB=memory.rss/1048576,privateMiB=memory.private/1048576)
        try:row['privateWorkingSetMiB']=p.memory_full_info().uss/1048576
        except psutil.AccessDenied:pass
        if nvidia.exists():
            result=subprocess.run([str(nvidia),'--query-gpu=utilization.gpu,utilization.decoder,memory.used','--format=csv,noheader,nounits'],capture_output=True,text=True)
            if result.returncode==0:
                gpu,decode,mem=map(float,result.stdout.strip().split(','));row.update(adapterGpuPercent=gpu,adapterDecoderPercent=decode,adapterMemoryMiB=mem)
        samples.append(row)
        if len(samples)%30==0:print(json.dumps({'progressSeconds':row['seconds'],'latest':row}),flush=True)
    except psutil.NoSuchProcess: break
    if time.perf_counter()-start>a.seconds+30:
        process.kill();raise RuntimeError('Shutdown timeout')
process.wait(timeout=10)
log=pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log'
text=log.read_text(encoding='utf-8')
(out/'flowplayer.log').write_text(text,encoding='utf-8')
metrics=[json.loads(line.split('METRIC ',1)[1]) for line in text.splitlines() if 'METRIC ' in line]
steady=[s for s in samples if 5<=s['seconds']<=a.seconds-5]
import hashlib
summary=dict(file=str(a.file),executable=str(a.exe.resolve()),executableSha256=hashlib.sha256(a.exe.read_bytes()).hexdigest(),testMode='muted, non-activating bottom window; desktop interaction not tested',exitCode=process.returncode,wallSeconds=time.perf_counter()-start,samples=len(steady),logicalProcessors=psutil.cpu_count(),gpuScope='whole adapter, includes desktop and other applications')
for key in ['cpuTotalPercent','workingSetMiB','privateMiB','privateWorkingSetMiB','adapterGpuPercent','adapterDecoderPercent']:
    values=[s[key] for s in steady if key in s]
    if values:summary[key]={'mean':statistics.mean(values),'max':max(values),'min':min(values)}
summary['timings']={k:int(v) for k,v in re.findall(r'(QT_APP_READY_MS|WINDOW_CREATED_MS|QML_LOADED_MS|SHOW_CALLED_MS|FIRST_SWAP_MS|FIRST_VIDEO_SWAP_MS)=(\d+)',text)}
stamps={}
for key in ['OPEN_FILE','FIRST_SWAP_MS','FIRST_VIDEO_SWAP_MS']:
    match=re.search(r'^(\S+) .*?'+key,text,re.M)
    if match:stamps[key]=datetime.datetime.fromisoformat(match[1].replace('Z','+00:00'))
if 'FIRST_SWAP_MS' in stamps:summary['timings']['externalLaunchToFirstSwapMs']=(stamps['FIRST_SWAP_MS']-startUtc).total_seconds()*1000
if 'FIRST_VIDEO_SWAP_MS' in stamps and 'OPEN_FILE' in stamps:summary['timings']['openToFirstVideoSwapMs']=(stamps['FIRST_VIDEO_SWAP_MS']-stamps['OPEN_FILE']).total_seconds()*1000
summary['mpvFirst']=metrics[0] if metrics else None
summary['mpvLast']=metrics[-1] if metrics else None
summary['cleanExit']='CLEAN_EXIT' in text and process.returncode==0
summary['diagnosticOnly']=a.diagnostic_no_cache
summary['mutedThroughoutSamples']=bool(metrics) and all(m.get('mute') is True for m in metrics)
summary['neverActivated']='WINDOW_ACTIVE=1' not in text and all(not m.get('windowActive',True) for m in metrics)
summary['completedRequestedDuration']=summary['wallSeconds']>=a.seconds-2
(out/'samples.json').write_text(json.dumps(samples,indent=2))
(out/'metrics.json').write_text(json.dumps(metrics,indent=2))
(out/'summary.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(summary,indent=2),flush=True)
if not summary['cleanExit'] or not summary['completedRequestedDuration'] or not summary['mutedThroughoutSamples'] or not summary['neverActivated']:raise SystemExit(1)
if not a.profile and not a.scenario:
    if not metrics or metrics[-1]['pause'] or metrics[-1]['dropped']!=0 or metrics[-1]['position']<a.seconds-5:raise SystemExit(2)
