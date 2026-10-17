"""Count resident pages by backing file using Windows PSAPI, without trimming memory."""
import ctypes as c
from ctypes import wintypes as w
import json, pathlib, psutil

class Region(c.Structure):
    _fields_=[('base',c.c_void_p),('allocation',c.c_void_p),('allocationProtect',w.DWORD),
              ('partition',w.WORD),('size',c.c_size_t),('state',w.DWORD),('protect',w.DWORD),('type',w.DWORD)]

kernel=c.WinDLL('kernel32',use_last_error=True)
psapi=c.WinDLL('psapi',use_last_error=True)
kernel.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];kernel.OpenProcess.restype=w.HANDLE
kernel.VirtualQueryEx.argtypes=[w.HANDLE,c.c_void_p,c.POINTER(Region),c.c_size_t]
kernel.VirtualQueryEx.restype=c.c_size_t
kernel.CloseHandle.argtypes=[w.HANDLE]
psapi.QueryWorkingSet.argtypes=[w.HANDLE,c.c_void_p,w.DWORD]
psapi.GetMappedFileNameW.argtypes=[w.HANDLE,c.c_void_p,w.LPWSTR,w.DWORD]
players=[p for p in psutil.process_iter(['name']) if p.info['name']=='FlowPlayer.exe']
if len(players)!=1:raise SystemExit('Expected exactly one running FlowPlayer test')
process=players[0];handle=kernel.OpenProcess(0x410,False,process.pid)
if not handle:raise c.WinError(c.get_last_error())
try:
    capacity=process.memory_info().rss//4096+32768
    while True:
        pages=(c.c_size_t*(capacity+1))()
        if psapi.QueryWorkingSet(handle,pages,c.sizeof(pages)):break
        if c.get_last_error()!=24:raise c.WinError(c.get_last_error())
        capacity*=2
    addresses=sorted((int(p)&~4095,bool(p&256)) for p in pages[1:pages[0]+1])
    region=Region();limit=0;name='';groups={}
    for address,shared in addresses:
        if address>=limit:
            if not kernel.VirtualQueryEx(handle,address,c.byref(region),c.sizeof(region)):continue
            limit=region.base+region.size
            name='<private allocations>'
            if region.type in (0x40000,0x1000000):
                path=c.create_unicode_buffer(32768)
                if psapi.GetMappedFileNameW(handle,address,path,len(path)):name=path.value
                else:name='<mapped pages>'
        group=groups.setdefault(name,{'residentMiB':0,'privateResidentMiB':0})
        group['residentMiB']+=4096/1048576
        if not shared:group['privateResidentMiB']+=4096/1048576
    print(json.dumps({'pid':process.pid,'groups':dict(sorted(groups.items(),key=lambda kv:kv[1]['privateResidentMiB'],reverse=True))},indent=2))
finally:kernel.CloseHandle(handle)
