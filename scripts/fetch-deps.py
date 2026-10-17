"""Fetch exact upstream SDK archives; record and enforce SHA-256 on subsequent runs."""
import hashlib, json, pathlib, subprocess, urllib.request, zipfile, os
ROOT = pathlib.Path(__file__).resolve().parents[1]
DEPS = ROOT / 'deps'
DL = DEPS / 'downloads'
DL.mkdir(parents=True, exist_ok=True)
lockfile = ROOT / 'scripts/dependencies.lock.json'
lock = json.loads(lockfile.read_text()) if lockfile.exists() else {}
def fetch(name, url):
    path = DL / name
    if not path.exists() or not path.stat().st_size:
        print('Downloading', name, flush=True)
        temporary = path.with_suffix(path.suffix+'.part')
        with urllib.request.urlopen(url, timeout=90) as response, temporary.open('wb') as out:
            while data := response.read(1024*1024): out.write(data)
        os.replace(temporary,path)
    digest = hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()
    if name in lock and lock[name]['sha256'] != digest: raise RuntimeError('Checksum mismatch: '+name)
    lock[name] = dict(url=url, sha256=digest)
    lockfile.write_text(json.dumps(lock, indent=2)+'\n')
    return path
seven = fetch('7zr.exe', 'https://github.com/ip7z/7zip/releases/download/26.04/7zr.exe')
def unpack7(path, dest):
    subprocess.run([str(seven), 'x', str(path), '-o'+str(dest), '-y', '-bso0', '-bsp0'], check=True)
base = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6120/qt6_6120_msvc2022_64/qt.qt6.6120.win64_msvc2022_64/'
for module in ['qtbase','qtdeclarative']:
    name = module+'-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z'
    archive = fetch(name, base+'6.12.0-0-202609280346'+name)
    unpack7(archive, DEPS/'Qt')
for package in ['microsoft.windows.sdk.cpp','microsoft.windows.sdk.cpp.x64']:
    version = '10.0.26100.3916'
    archive = fetch(package+'.nupkg', f'https://api.nuget.org/v3-flatcontainer/{package}/{version}/{package}.{version}.nupkg')
    with zipfile.ZipFile(archive) as z: z.extractall(DEPS/'sdk'/package)
for name, filename in [('mpv','mpv-dev-x86_64-20261017-git-dde0671072.7z'),('ffmpeg','ffmpeg-x86_64-git-2d49d0e62.7z')]:
    archive = fetch(('mpv-dev' if name=='mpv' else name)+'.7z', 'https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20261017/'+filename)
    unpack7(archive, DEPS/name)
print('Dependencies verified and extracted.', flush=True)
