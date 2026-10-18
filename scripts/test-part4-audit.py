"""Focused audit checks, with isolated data and no desktop/window input."""
import pathlib,os,subprocess,time,json,sqlite3,hashlib,re,argparse,sys
import psutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--name',default='part4-audit-extra');parser.add_argument('--queue-only',action='store_true');args=parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]+',args.name):raise SystemExit('Use a simple result name')
OUT=ROOT/'tests/results'/args.name;OUT.mkdir(parents=True,exist_ok=True)
if (OUT/'cases.json').exists():raise SystemExit('Preserve existing evidence; use --name')
EXE=ROOT/'out/FlowPlayer/FlowPlayer.exe';DB=OUT/'flow.db';SETTINGS=OUT/'settings.ini'
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):raise SystemExit('Player open; refusing to disturb your session')
sourcePath=ROOT/'tests/results'/('part4-audit-extra' if args.queue_only else 'part4-audit-core')/'flow.db'
if not sourcePath.exists():raise SystemExit('Required isolated fixture history is missing: run the core audit first (and the extra audit before queue-only).')
source=sqlite3.connect(sourcePath);dest=sqlite3.connect(DB);source.backup(dest);dest.close();source.close()
cases=[]
def run(name,file=None,seconds=5,scenario='part4-pause',db=DB,auto=False):
    SETTINGS.write_text('[General]\nautoResume='+str(auto).lower()+'\n',encoding='utf-8')
    env=os.environ.copy();env.update(FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1',FLOW_HISTORY_DB=str(db),FLOW_THUMBS_DIR=str(OUT/'thumbs'),FLOW_SETTINGS_FILE=str(SETTINGS),FLOW_QUIT_MS=str(seconds*1000),FLOW_SCENARIO=scenario,FLOW_METRICS='1')
    p=subprocess.Popen([str(EXE)]+([str(file)] if file else []),env=env)
    try:p.wait(timeout=seconds+15)
    except subprocess.TimeoutExpired:p.kill();raise RuntimeError(name+' shutdown hang')
    text=(pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log').read_text(encoding='utf-8');(OUT/(name+'.log')).write_text(text,encoding='utf-8')
    def entries(tag):return [json.loads(line.split(tag+' ',1)[1]) for line in text.splitlines() if tag+' ' in line]
    r={'name':name,'exit':p.returncode,'clean':'CLEAN_EXIT' in text,'neverActivated':'WINDOW_ACTIVE=1' not in text,'loads':entries('RESUME_LOAD'),'settled':entries('PART4_SETTLED'),'nav':[line.split('AUDIT_NAV ',1)[1] for line in text.splitlines() if 'AUDIT_NAV ' in line],'homeInput':re.findall(r'PART4_HOME_INPUT .*',text),'errors':[line for line in text.splitlines() if 'qrc:' in line or 'HISTORY_ERROR' in line or 'mpv command failed' in line]}
    cases.append(r);(OUT/'cases.json').write_text(json.dumps(cases,indent=2),encoding='utf-8')
    assert p.returncode==0 and r['clean'] and r['neverActivated'] and not r['errors'],r
    print(name,'PASS',r['loads'],flush=True);return r
if args.queue_only:
    run('thumbnail-queue',None,6,'part4-home')
    text=(OUT/'thumbnail-queue.log').read_text(encoding='utf-8');successes=text.count('success=1 deferredDuringPlayback=0');assert successes>=3 and 'success=0 deferredDuringPlayback=0' not in text
    summary={'executableSha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'thumbnailQueueSuccesses':successes,'passed':True};(OUT/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2));sys.exit(0)
A=ROOT/'tests/media/part3-multi.mkv'
r=run('finished-replay',A,8,'part4-audit-replay');assert float(r['loads'][0]['start'])==0
with sqlite3.connect(DB) as c:
    saved=c.execute('select position,finished from history where path=?',(str(A).replace('\\','/'),)).fetchone()
assert saved and saved[0]>2 and saved[1]==0, saved
r=run('replay-resumes',A);assert abs(float(r['loads'][0]['start'])-max(0,saved[0]-2))<.002
r=run('five-second-keys',A,5,'part4-audit-keys')
positions=[v['position'] for v in r['settled']]
assert any(abs(v-7.123)<.04 for v in positions) and sum(abs(v-12.123)<.04 for v in positions)>=2,positions
run('empty-home',None,5,'part4-home',OUT/'empty.db')
series=OUT/'series';series.mkdir(exist_ok=True)
for number in (1,2,10):
    subprocess.run([str(ROOT/'deps/ffmpeg/ffmpeg.exe'),'-y','-hide_banner','-loglevel','error','-i',str(A),'-t','2','-map','0:v:0','-map','0:a:0','-c','copy','-metadata','title=Episode '+str(number),str(series/('Episode '+str(number)+'.mkv'))],check=True)
r=run('previous-next',series/'Episode 2.mkv',6,'part4-audit-nav');assert [pathlib.Path(p).name for p in r['nav']]==['Episode 2.mkv','Episode 1.mkv','Episode 2.mkv','Episode 10.mkv'],r['nav']
run('auto-resume',None,5,'part4-pause',auto=True)
for thumb in (OUT/'thumbs').glob('*.jpg'):thumb.unlink()
run('thumbnail-queue',None,6,'part4-home')
queueLog=(OUT/'thumbnail-queue.log').read_text(encoding='utf-8');successes=queueLog.count('success=1 deferredDuringPlayback=0');assert successes>=3 and 'success=0 deferredDuringPlayback=0' not in queueLog,queueLog
summary={'executableSha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'caseCount':len(cases),'passed':True,'thumbnailQueueSuccesses':successes,'physicalFullscreenResizeVisualTests':'manual pending'}
(OUT/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8');print(json.dumps(summary,indent=2))
