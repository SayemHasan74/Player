param([string]$Target = 'FlowPlayer', [string]$VsRoot = 'E:\Applications\VisualStudio\2022\BuildTools', [switch]$Clean)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vcvars = Join-Path $VsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (!(Test-Path -LiteralPath $vcvars)) { throw 'VS 2022 C++ Build Tools not found; pass -VsRoot.' }
$vars = & cmd /c "`"$vcvars`" >nul && set"
foreach ($line in $vars) { if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') } }
if ($env:VCToolsVersion.TrimEnd('\') -ne '14.44.35207') { throw 'Expected pinned MSVC toolset 14.44.35207; select that toolset in VS 2022 Build Tools.' }
$sdk = Join-Path $root 'deps\sdk\microsoft.windows.sdk.cpp\c'
$sdkLib = Join-Path $root 'deps\sdk\microsoft.windows.sdk.cpp.x64\c'
$version = (Get-ChildItem (Join-Path $sdk 'Include') -Directory | Select-Object -First 1).Name
$env:INCLUDE = "$sdk\Include\$version\ucrt;$sdk\Include\$version\shared;$sdk\Include\$version\um;$sdk\Include\$version\winrt;$env:INCLUDE"
$env:LIB = "$sdkLib\ucrt\x64;$sdkLib\um\x64;$env:LIB"
$env:PATH = "$root\.tools\Scripts;$sdk\bin\$version\x64;$env:PATH"
$exports = & dumpbin /nologo /exports "$root\deps\mpv\libmpv-2.dll"
$names = @('LIBRARY libmpv-2.dll','EXPORTS') + @($exports | ForEach-Object { if ($_ -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(mpv_\w+)') { $matches[1] } })
$names | Set-Content -LiteralPath "$root\deps\mpv\mpv.def" -Encoding ascii
& lib /nologo /machine:x64 "/def:$root\deps\mpv\mpv.def" "/out:$root\deps\mpv\mpv.lib"
if ($LASTEXITCODE) { throw 'Import library generation failed' }
& cmake -S $root -B "$root\build" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$root/deps/Qt" -DCMAKE_RC_COMPILER=rc
if ($LASTEXITCODE) { throw 'Configure failed' }
$buildArguments=@('--build',"$root\build",'--target',$Target)
if ($Clean) { $buildArguments+='--clean-first' }
& cmake @buildArguments
if ($LASTEXITCODE) { throw 'Build failed' }
