"""Ten bounded integration cases. Muted, non-activating, isolated data; no desktop input."""
import pathlib,os,subprocess,time,json,sqlite3,hashlib,re,argparse
import psutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--name',default='part5-release');a=parser.parse_args()
if not re.fullmatch(r'[\w-]+',a.name):raise SystemExit('Simple result name required')
OUT=ROOT/'tests/results'/a.name
if OUT.exists():raise SystemExit('Preserve existing evidence; choose a new --name')
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):raise SystemExit('Player open: refusing to disturb it')
OUT.mkdir(parents=True);series=OUT/'series';series.mkdir();exe=ROOT/'out/FlowPlayer/FlowPlayer.exe';cases=[]
for n in range(1,11):
    subprocess.run([str(ROOT/'deps/ffmpeg/ffmpeg.exe'),'-y','-hide_banner','-loglevel','error','-i',str(ROOT/'tests/media/part3-multi.mkv'),'-map','0','-c','copy','-metadata','title=Episode '+str(n),str(series/('Episode '+str(n)+'.mkv'))],check=True)
DB=OUT/'flow.db';settings=OUT/'settings.ini'
settings.write_text('[General]\npreviousRestarts=false\n',encoding='utf-8')
def env(seconds,scenario):
    result=os.environ.copy();result.update(FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1',FLOW_METRICS='1',FLOW_QUIT_MS=str(seconds*1000),FLOW_SCENARIO=scenario,FLOW_HISTORY_DB=str(DB),FLOW_THUMBS_DIR=str(OUT/'thumbs'),FLOW_SETTINGS_FILE=str(settings))
    for key in ['QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','FLOW_INSTANCE_NAME']:result.pop(key,None)
    return result
logpath=pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log'
def collect(name,p):
    text=logpath.read_text(encoding='utf-8');(OUT/(name+'.log')).write_text(text,encoding='utf-8')
    def records(tag):return [json.loads(line.split(tag+' ',1)[1]) for line in text.splitlines() if tag+' ' in line]
    r={'name':name,'exit':p.returncode,'loads':re.findall(r'PART5_LOADED (.*)',text),'options':records('RESUME_LOAD'),'lists':records('PART5_LIST'),'panels':records('PART5_PANEL'),'clean':'CLEAN_EXIT' in text,'neverActivated':'WINDOW_ACTIVE=1' not in text,'qmlErrors':[line for line in text.splitlines() if 'qrc:' in line or 'HISTORY_ERROR' in line],'smtcReady':'SMTC_READY' in text,'sleepInhibitOn':'DISPLAY_SLEEP_INHIBIT=1' in text,'sleepInhibitOff':'DISPLAY_SLEEP_INHIBIT=0' in text,'errorsSkipped':text.count('PLAYLIST_ERROR')}
    cases.append(r);(OUT/'cases.json').write_text(json.dumps(cases,indent=2))
    assert r['exit']==0 and r['clean'] and r['neverActivated'] and not r['qmlErrors'],r
    assert all(v['loaded'] and v['sameScene'] for v in r['panels']),r
    print(name,'PASS',[pathlib.Path(p).name for p in r['loads']],flush=True);return r
def run(name,files,scenario,seconds=6,extra=None):
    variables=env(seconds,scenario);variables.update(extra or {})
    p=subprocess.Popen([str(exe)]+[str(f) for f in files],env=variables);p.wait(timeout=seconds+15);return collect(name,p)
ep=lambda n:series/('Episode '+str(n)+'.mkv')
# First establishes a real identity/schema and saves a position for episode 6.
r=run('seed-episode6',[ep(6)],'part5-pause',3)
with sqlite3.connect(DB) as c:c.execute('update history set position=8,duration=24,finished=0 where path=?',(ep(6).as_posix(),))
r=run('auto-5-to-6',[ep(5)],'part5-auto',5)
assert [pathlib.Path(p).name for p in r['loads']]==['Episode 5.mkv','Episode 6.mkv'],r
assert abs(float(r['options'][1]['start'])-6)<.001,r
assert any(len(v['paths'])==10 and v['current']==4 for v in r['lists']),r
with sqlite3.connect(DB) as c:assert c.execute('select finished from history where path=?',(ep(5).as_posix(),)).fetchone()[0]==1
r=run('neighbors-and-boundaries',[ep(5)],'part5-nav',7)
assert [pathlib.Path(p).name for p in r['loads']]==['Episode 5.mkv','Episode 6.mkv','Episode 5.mkv','Episode 10.mkv','Episode 1.mkv'],r
r=run('custom-reorder',[ep(2),ep(9)],'part5-custom',4)
assert [pathlib.Path(p).name for p in r['loads']]==['Episode 2.mkv','Episode 9.mkv'],r
assert any([pathlib.Path(p).name for p in v['paths']]==['Episode 9.mkv','Episode 2.mkv'] for v in r['lists']),r
zero=OUT/'zero.mkv';zero.write_bytes(b'');corrupt=OUT/'corrupt.mkv';corrupt.write_bytes(b'corrupt unsupported data');missing=OUT/'missing.mkv'
r=run('bad-files-skip',[zero,corrupt,missing,ep(3)],'part5-pause',5)
assert [pathlib.Path(p).name for p in r['loads']]==['Episode 3.mkv'] and r['errorsSkipped']==2,r
e=env(6,'part5-pause');e['FLOW_INSTANCE_NAME']='FlowPlayer-test-'+str(os.getpid());p=subprocess.Popen([str(exe),str(ep(1))],env=e)
time.sleep(2)
second=subprocess.run([str(exe),str(ep(8))],env=e,timeout=5);assert second.returncode==0
p.wait(timeout=15);r=collect('single-instance',p)
assert [pathlib.Path(p).name for p in r['loads']]==['Episode 1.mkv','Episode 8.mkv'],r
settings.write_text('[General]\npreviousRestarts=true\n',encoding='utf-8')
r=run('previous-restarts',[ep(4)],'part5-previous',4)
text=(OUT/'previous-restarts.log').read_text(encoding='utf-8');settled=[float(p) for p in re.findall(r'PART5_SETTLED ([\d.]+)',text)]
assert len(r['loads'])==1 and any(abs(p-8)<.05 for p in settled) and any(p<.05 for p in settled),r
settings.write_text('[General]\nstayOnLastFrame=true\n',encoding='utf-8')
run('end-stays-paused',[ep(10)],'part5-end',4)
assert 'PART5_STATE home=0 current=9 pause=1' in (OUT/'end-stays-paused.log').read_text(encoding='utf-8')
settings.write_text('[General]\nstayOnLastFrame=false\n',encoding='utf-8')
run('end-shows-home',[ep(10)],'part5-end',4)
assert 'PART5_STATE home=1' in (OUT/'end-shows-home.log').read_text(encoding='utf-8')
r=run('resume-A-B-A',[ROOT/'tests/media/part3-multi.mkv'],'part4-switch',8,{'FLOW_PART4_OTHER':str(ROOT/'tests/media/h264-1080p60.mkv')})
assert len(r['options'])==3 and abs(float(r['options'][2]['start'])-10.123)<.04,r
assert r['options'][2]['aid']=='2' and r['options'][2]['sid']=='2' and r['options'][2]['volume']=='61' and r['options'][2]['speed']=='1.25',r
summary={'executableSha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'cases':len(cases),'passed':True,'smtcInitialized':all(r['smtcReady'] for r in cases),'manualVisualAcceptance':'pending','longSoak':'not run: user requested practical testing','fullAcceptance':False}
(OUT/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
