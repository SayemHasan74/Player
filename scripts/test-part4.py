"""Bounded muted/background Part 4 checks. No desktop input or screenshots."""
import os,sys,pathlib,subprocess,time,json,sqlite3,hashlib,shutil,re,datetime,argparse
import psutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--name',default='part4-release');args=parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]+',args.name):raise SystemExit('Use a simple result folder name')
OUT=ROOT/'tests/results'/args.name;OUT.mkdir(parents=True,exist_ok=True)
EXE=ROOT/'out/FlowPlayer/FlowPlayer.exe'
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):raise SystemExit('Player is open; refusing to disturb your session.')
DB=OUT/'flow.db';THUMBS=OUT/'thumbs';SETTINGS=OUT/'settings.ini'
if DB.exists():raise SystemExit('Results already exist. Preserve evidence and use a new result directory.')
cases=[]
def run(name,file=None,seconds=6,scenario='part4-pause',kill=False,extra=None):
    env=os.environ.copy();env.update(FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1',FLOW_METRICS='1',FLOW_QUIT_MS=str(seconds*1000),FLOW_SCENARIO=scenario,FLOW_HISTORY_DB=str(DB),FLOW_THUMBS_DIR=str(THUMBS),FLOW_SETTINGS_FILE=str(SETTINGS))
    env.update(extra or {})
    for k in ['QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(k,None)
    begin=time.time();p=subprocess.Popen([str(EXE)]+([str(file)] if file else []),env=env);proc=psutil.Process(p.pid);proc.cpu_percent();samples=[]
    while p.poll() is None:
        time.sleep(.25)
        try:samples.append({'seconds':time.time()-begin,'cpuTotal':proc.cpu_percent()/psutil.cpu_count(),'rss':proc.memory_info().rss})
        except psutil.NoSuchProcess:pass
        if kill and time.time()-begin>=seconds:p.kill();break
        if time.time()-begin>seconds+20:p.kill();raise RuntimeError(name+' shutdown timeout')
    p.wait();log=(pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log').read_text(encoding='utf-8');(OUT/(name+'.log')).write_text(log,encoding='utf-8')
    def entries(tag):return [json.loads(line.split(tag+' ',1)[1]) for line in log.splitlines() if tag+' ' in line]
    errors=[line for line in log.splitlines() if 'HISTORY_ERROR' in line or 'qrc:' in line or 'mpv command failed' in line]
    stamps={k:int(v) for k,v in re.findall(r'(FIRST_SWAP_MS|HOME_SCENE_READY_MS|QML_LOADED_MS)=(\d+)',log)}
    first=re.search(r'^(\S+).*FIRST_SWAP_MS=',log,re.M)
    if first:stamps['externalFirstSwapMs']=(datetime.datetime.fromisoformat(first[1].replace('Z','+00:00')).timestamp()-begin)*1000
    case={'name':name,'exit':p.returncode,'killed':kill,'clean':'CLEAN_EXIT' in log,'neverActivated':'WINDOW_ACTIVE=1' not in log,'loads':entries('RESUME_LOAD'),'settled':entries('PART4_SETTLED'),'commits':entries('PART4_COMMIT'),'metrics':entries('METRIC'),'errors':errors,'timings':stamps,'samples':samples,'thumbnailSuccesses':log.count('success=1 deferredDuringPlayback=0')}
    cases.append(case);(OUT/'cases.json').write_text(json.dumps(cases,indent=2));print(name,'exit',p.returncode,'loads',case['loads'],'errors',errors,flush=True)
    assert kill or (p.returncode==0 and case['clean'])
    assert case['neverActivated'] and not errors
    assert all(m['mute'] for m in case['metrics'])
    return case
def rows():
    with sqlite3.connect(DB) as c:c.row_factory=sqlite3.Row;return [dict(r) for r in c.execute('select * from history order by last_played desc')]
A=ROOT/'tests/media/part3-multi.mkv';B=ROOT/'tests/media/h264-1080p60.mkv'
switch=run('switch-A-B-A',A,8,'part4-switch',extra={'FLOW_PART4_OTHER':str(B)})
assert len(switch['loads'])==3 and abs(float(switch['loads'][2]['start'])-10.123)<.04
assert switch['loads'][2]['aid']=='2' and switch['loads'][2]['sid']=='2' and switch['loads'][2]['volume']=='61' and switch['loads'][2]['speed']=='1.25'
saved=next(r for r in rows() if pathlib.Path(r['path']).name==A.name);expected=max(0,saved['position']-2)
restart=run('restart-A',A);assert abs(float(restart['loads'][0]['start'])-expected)<.0011
assert abs(restart['settled'][0]['position']-expected)<.05
moved=OUT/'moved'/'renamed.mkv';moved.parent.mkdir(exist_ok=True);shutil.copyfile(A,moved)
saved=next(r for r in rows() if pathlib.Path(r['path']).name==A.name);expected=max(0,saved['position']-2)
renamed=run('moved-A',moved);assert abs(float(renamed['loads'][0]['start'])-expected)<.0011
with sqlite3.connect(DB) as c:c.execute('update history set position=duration*.96,finished=0 where file_key=?',(saved['file_key'],))
finished=run('finished-96',moved);assert float(finished['loads'][0]['start'])==0
assert next(r for r in rows() if r['file_key']==saved['file_key'])['finished']==1
crash=run('crash',B,8,'part4-crash',kill=True)
snapshot=next(r for r in rows() if pathlib.Path(r['path']).name==B.name)
age=(time.time()*1000-snapshot['last_played'])/1000;assert age<=5.5
expected=max(0,snapshot['position']-2);recovered=run('crash-reopen',B);assert abs(float(recovered['loads'][0]['start'])-expected)<.0011
for thumb in THUMBS.glob('*.jpg'):thumb.unlink()
home=run('home',None,10,'part4-home')
assert home['thumbnailSuccesses']>=1
with sqlite3.connect(DB) as c:
    base=rows()[0]
    for i in range(1000):
        r=dict(base);r.update(file_key='stress-'+str(i),path=str(OUT/('missing-'+str(i)+'.mkv')),last_played=1000+i)
        c.execute('insert or replace into history values(?,?,?,?,?,?,?,?,?,?,?,?,?)',tuple(r[k] for k in ['file_key','path','size','mtime','duration','position','finished','last_played','audio_track','sub_track','sub_delay','volume','speed']))
stress=run('home-1000',None,5,'part4-home');assert 'rows=10' in (OUT/'home-1000.log').read_text(encoding='utf-8')
summary={'executableSha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'crashSaveAgeSeconds':age,'caseCount':len(cases),'allBasicChecks':True,'peakResidentMB':max(s['rss'] for c in cases for s in c['samples'])/1e6,'manualVisualAcceptance':'pending','fullAcceptance':False}
(OUT/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
