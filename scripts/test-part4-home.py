"""Focused final-package checks after the home caption hit-test correction."""
import pathlib,os,subprocess,time,json,hashlib,re
ROOT=pathlib.Path(__file__).resolve().parents[1];OUT=ROOT/'tests/results/part4-home-final';OUT.mkdir(parents=True,exist_ok=True)
EXE=ROOT/'out/FlowPlayer/FlowPlayer.exe';prior=ROOT/'tests/results/part4-final';settings=OUT/'settings.ini'
if (OUT/'summary.json').exists():raise SystemExit('Existing final home evidence is preserved; use a fresh output folder for another run.')
clip=prior/'short-eof.mkv'
if not clip.exists():subprocess.run([str(ROOT/'deps/ffmpeg/ffmpeg.exe'),'-y','-hide_banner','-loglevel','error','-i',str(ROOT/'tests/media/part3-multi.mkv'),'-t','2','-map','0:v:0','-map','0:a:0','-c','copy',str(clip)],check=True)
import psutil
if any(p.info['name']=='FlowPlayer.exe' for p in psutil.process_iter(['name'])):raise SystemExit('Player is open; refusing to disturb it.')
cases=[]
def run(name,file=None,auto=False,seconds=6,scenario='part4-home'):
    settings.write_text('[General]\nautoResume='+str(auto).lower()+'\nvolume=61\n',encoding='utf-8')
    env=os.environ.copy();env.update(FLOW_TEST_MUTE='1',FLOW_TEST_BACKGROUND='1',FLOW_HISTORY_DB=str(prior/'flow.db'),FLOW_THUMBS_DIR=str(prior/'thumbs'),FLOW_SETTINGS_FILE=str(settings),FLOW_QUIT_MS=str(seconds*1000),FLOW_SCENARIO=scenario,FLOW_METRICS='1')
    p=subprocess.Popen([str(EXE)]+([str(file)] if file else []),env=env)
    try:p.wait(timeout=seconds+15)
    except subprocess.TimeoutExpired:p.kill();raise RuntimeError(name+' shutdown hang')
    text=(pathlib.Path(os.environ['LOCALAPPDATA'])/'FlowPlayer/logs/flowplayer.log').read_text(encoding='utf-8');(OUT/(name+'.log')).write_text(text,encoding='utf-8')
    loads=[json.loads(l.split('RESUME_LOAD ',1)[1]) for l in text.splitlines() if 'RESUME_LOAD ' in l]
    r={'name':name,'exit':p.returncode,'loads':loads,'clean':'CLEAN_EXIT' in text,'neverActivated':'WINDOW_ACTIVE=1' not in text,'input':re.findall(r'PART4_HOME_INPUT .*',text),'ui':re.findall(r'PART4_UI .*',text),'home':re.findall(r'HOME_SCENE_READY_MS=.*',text)};cases.append(r)
    assert p.returncode==0 and r['clean'] and r['neverActivated']
    assert 'HISTORY_ERROR' not in text and 'qrc:' not in text
    print(json.dumps(r),flush=True);return r
home=run('home-caption-focus');assert any('caption=1 client=1 focusHome=1' in s for s in home['input'])
auto=run('auto-resume',auto=True,scenario='part4-pause');assert len(auto['loads'])==1 and float(auto['loads'][0]['start'])>0
eof=run('eof-home-close',prior/'short-eof.mkv',seconds=7,scenario='part4-eof');assert eof['home'] and len(eof['loads'])==1
summary={'executableSha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'cases':cases,'allBasicChecks':True,'manualVisualAcceptance':'pending'}
(OUT/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
