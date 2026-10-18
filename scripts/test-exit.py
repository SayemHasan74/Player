import json, os, pathlib, subprocess, time, datetime, re, argparse, hashlib, psutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--exe',type=pathlib.Path,default=ROOT/'build/FlowPlayer.exe');parser.add_argument('--standalone',action='store_true');parser.add_argument('--name',default='T8-exit');args=parser.parse_args()
out=ROOT/'tests/results'/args.name;out.mkdir(parents=True,exist_ok=True)
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):
    parser.error('FlowPlayer is already running; refusing to disturb it or share its log.')
env=os.environ.copy();env.update(FLOW_QUIT_MS='3000',FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1')
env.update(FLOW_SETTINGS_FILE=str(out/'settings.ini'),FLOW_HISTORY_DB=str(out/'flow.db'),FLOW_THUMBS_DIR=str(out/'thumbs'))
if args.standalone:
    for key in ['QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(key,None)
else:
    env['PATH']=str(ROOT/'deps/Qt/bin')+';'+env['PATH']
    env['QT_PLUGIN_PATH']=str(ROOT/'deps/Qt/plugins');env['QML_IMPORT_PATH']=str(ROOT/'deps/Qt/qml')
results=[]
for i in range(20):
    start=time.perf_counter()
    startUtc=datetime.datetime.now(datetime.timezone.utc)
    p=subprocess.Popen([str(args.exe.resolve()),str(ROOT/'tests/media/hevc-4k60-main10.mkv')],env=env)
    try:code=p.wait(timeout=20)
    except subprocess.TimeoutExpired:p.kill();code='hang'
    log=(pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log').read_text(encoding='utf-8')
    (out/f'{i+1:02d}.log').write_text(log,encoding='utf-8')
    freed=log.find('mpv_render_context_free')
    terminated=log.find('mpv_terminate_destroy on event worker')
    result=dict(run=i+1,exitCode=code,seconds=time.perf_counter()-start,renderFreedFirst=0<=freed<terminated,cleanExit='CLEAN_EXIT' in log,neverActivated='WINDOW_ACTIVE=1' not in log,executableSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest())
    match=re.search(r'^(\S+) .*?FIRST_SWAP_MS=(\d+)',log,re.M)
    if match:
        stamp=datetime.datetime.fromisoformat(match[1].replace('Z','+00:00'))
        result['mainToFirstSwapMs']=int(match[2]);result['launchToFirstSwapMs']=(stamp-startUtc).total_seconds()*1000
    results.append(result);print(json.dumps(result),flush=True)
(out/'summary.json').write_text(json.dumps(results,indent=2))
if not all(r['exitCode']==0 and r['renderFreedFirst'] and r['cleanExit'] and r['neverActivated'] for r in results):raise SystemExit(1)
