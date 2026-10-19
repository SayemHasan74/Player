"""One isolated input smoke. Never moves/resizes/minimizes the desktop window."""
import argparse,hashlib,json,os,pathlib,re,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--name',default='final-pass');a=p.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]+',a.name):raise SystemExit('Simple result name required')
out=ROOT/'tests/results'/a.name
if out.exists():raise SystemExit('Preserve existing evidence; choose a new --name')
out.mkdir(parents=True);exe=ROOT/'build/FlowPlayer.exe'
env=os.environ.copy();env.update(FLOW_TEST_BACKGROUND='1',FLOW_TEST_MUTE='1',FLOW_SCENARIO='final-pass',FLOW_QUIT_MS='10500',FLOW_LOG_DIRECTORY=str(out/'logs'),FLOW_SETTINGS_FILE=str(out/'settings.ini'),FLOW_HISTORY_DB=str(out/'flow.db'),FLOW_THUMBS_DIR=str(out/'thumbs'),PATH=str(ROOT/'deps/Qt/bin')+';'+env['PATH'],QT_PLUGIN_PATH=str(ROOT/'deps/Qt/plugins'),QML_IMPORT_PATH=str(ROOT/'deps/Qt/qml'))
env.pop('FLOW_INSTANCE_NAME',None)
process=subprocess.Popen([str(exe),str(ROOT/'tests/media/part3-multi.mkv')],env=env)
try:process.wait(timeout=25)
except BaseException:
    if process.poll() is None:process.kill();process.wait()
    raise
log=(out/'logs/flowplayer.log').read_text(encoding='utf-8')
stages={r['phase']:r for line in log.splitlines() if 'FINAL_PASS ' in line for r in [json.loads(line.split('FINAL_PASS ',1)[1])]}
errors=[line for line in log.splitlines() if 'qrc:' in line or 'ReferenceError' in line or 'TypeError' in line]
assert process.returncode==0 and 'CLEAN_EXIT' in log and not errors,errors
for phase in ['hidden_before','right_sent','right_done','left_done','up_done','down_done']:assert not stages[phase]['visible'],stages
assert stages['right_done']['position']-stages['hidden_before']['position']>4.5,stages
assert stages['right_done']['position']-stages['left_done']['position']>4,stages
assert stages['up_done']['volume']==105 and stages['down_done']['volume']==100,stages
assert stages['mouse_moved']['visible'],stages
assert stages['url_focus']['popup'] and stages['url_focus']['textFocus'],stages
assert stages['escape_done']['pause'] and not stages['escape_done']['popup'],stages
assert 'BACKGROUND_MINIMIZE_SUPPRESSED resumeOnRestore=0' in log
assert 'FINAL_MAX_HIT center=1 outside=0' in log
layout=json.loads(next(line.split('FINAL_LAYOUT ',1)[1] for line in log.splitlines() if 'FINAL_LAYOUT ' in line))
assert all(r['volumeClear'] and r['actionsClear'] and r['seekWidth']>0 for r in layout['layouts']),layout
assert 'WINDOW_ACTIVE=1' not in log
result={'executableSha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'passed':True,'stages':stages,'layout':layout,'cleanExit':True,'neverActivated':True,'nativeDesktopMinimize':'suppressed; production request/pause policy checked','nativeMaximizeRestore':'manual; capture pairing tested in unit target','errors':errors}
(out/'summary.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
