"""Add transparent cache estimates and summarize test artifacts; never substitute caps for usage."""
import json, pathlib, statistics, argparse, datetime, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--names',nargs='*');args=parser.parse_args()
for folder in (ROOT/'tests/results').iterdir():
    if args.names and folder.name not in args.names:continue
    summary=folder/'summary.json'
    if not summary.exists() or not (folder/'metrics.json').exists():continue
    result=json.loads(summary.read_text())
    metrics=json.loads((folder/'metrics.json').read_text())
    samples=json.loads((folder/'samples.json').read_text())
    offset=(result['timings'].get('externalLaunchToFirstSwapMs',0)-result['timings'].get('FIRST_SWAP_MS',0))/1000
    estimates=[]
    for sample in samples:
        if sample['seconds']<30:continue
        previous=[m for m in metrics if m['elapsedMs']/1000+offset<=sample['seconds'] and m.get('cacheBytes',-1)>=0]
        if not previous:continue
        cache=previous[-1]['cacheBytes']/1048576
        if 'privateWorkingSetMiB' in sample:
            estimates.append(dict(private=sample['privateWorkingSetMiB']-cache,total=sample['workingSetMiB']-cache,cache=cache))
    if estimates:
        result['packetCacheEstimateMiB']=statistics.mean(x['cache'] for x in estimates)
        result['privateWsMinusPacketQueueMiB']={'mean':statistics.mean(x['private'] for x in estimates),'max':max(x['private'] for x in estimates)}
        result['totalWsMinusPacketQueueMiB']={'mean':statistics.mean(x['total'] for x in estimates),'max':max(x['total'] for x in estimates)}
        result['cacheAccountingNote']='Subtracts mpv packet-queue estimate, not measured allocator overhead; approximate only.'
        summary.write_text(json.dumps(result,indent=2))
    log=(folder/'flowplayer.log').read_text(encoding='utf-8') if (folder/'flowplayer.log').exists() else ''
    marks={}
    for line in log.splitlines():
        if 'IDLE_MARK ' in line:
            mark=json.loads(line.split('IDLE_MARK ',1)[1]);marks[mark['label']]=mark
    for state in ['pause','minimize']:
        if state+'-start' not in marks or state+'-end' not in marks:continue
        begin,end=marks[state+'-start'],marks[state+'-end']
        window=[s for s in samples if begin['elapsedMs']/1000+offset<=s['seconds']<=end['elapsedMs']/1000+offset]
        result[state+'Interval']={'durationMs':end['elapsedMs']-begin['elapsedMs'],'renderedFrames':end['renders']-begin['renders'],
            'mpvCallbacks':end['callbacks']-begin['callbacks'],'cpuMeanTotalPercent':statistics.mean(s['cpuTotalPercent'] for s in window) if window else None,
            'cpuMaxTotalPercent':max(s['cpuTotalPercent'] for s in window) if window else None,'samples':len(window)}
    if 'TEST_PAUSE_STEADY_BEGIN' in log:
        stamps={}
        for label in ('BEGIN','END'):
            match=re.search(r'^(\S+) .*TEST_PAUSE_STEADY_'+label,log,re.M)
            if match:stamps[label]=datetime.datetime.fromisoformat(match[1].replace('Z','+00:00'))
        if len(stamps)==2:
            startMatch=re.search(r'^(\S+) .*FIRST_SWAP_MS=',log,re.M)
            start=datetime.datetime.fromisoformat(startMatch[1].replace('Z','+00:00'))
            firstSwap=result['timings']['externalLaunchToFirstSwapMs']/1000
            begin=(stamps['BEGIN']-start).total_seconds()+firstSwap
            end=(stamps['END']-start).total_seconds()+firstSwap
            window=[s for s in samples if begin<=s['seconds']<=end]
            intervalMetrics=[m for m in metrics if begin<=m['elapsedMs']/1000+offset<=end]
            result['pauseInterval']={'durationMs':(stamps['END']-stamps['BEGIN']).total_seconds()*1000,
                'cpuMeanTotalPercent':statistics.mean(s['cpuTotalPercent'] for s in window),
                'cpuMaxTotalPercent':max(s['cpuTotalPercent'] for s in window),
                'renderedFrames':intervalMetrics[-1]['renders']-intervalMetrics[0]['renders'],
                'mpvCallbacks':intervalMetrics[-1]['callbacks']-intervalMetrics[0]['callbacks'],'samples':len(window)}
            marks['pause']=True
    if marks:summary.write_text(json.dumps(result,indent=2))
    if folder.name.startswith(('T1','T2','T6')):
        print(folder.name,json.dumps(result),flush=True)
