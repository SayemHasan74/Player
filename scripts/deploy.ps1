$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'out\FlowPlayer'
New-Item -ItemType Directory -Path $out -Force | Out-Null
Copy-Item -LiteralPath "$root\build\FlowPlayer.exe","$root\deps\mpv\libmpv-2.dll" -Destination $out
& "$root\deps\Qt\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw --no-compiler-runtime --skip-plugin-types generic,networkinformation,tls,imageformats,iconengines,qmltooling --qmldir "$root\src\ui" "$out\FlowPlayer.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed' }
# Only the Windows platform plugin is needed. Do not deploy Widgets, multimedia, WebEngine or unused QML modules.
@('[Paths]','Plugins=.','QmlImports=qml') | Set-Content -LiteralPath "$out\qt.conf" -Encoding ascii
Write-Output $out
