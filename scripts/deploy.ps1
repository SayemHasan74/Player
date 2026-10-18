$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'out\FlowPlayer'
New-Item -ItemType Directory -Path $out -Force | Out-Null
Copy-Item -LiteralPath "$root\build\FlowPlayer.exe","$root\deps\mpv\libmpv-2.dll" -Destination $out
& "$root\deps\Qt\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw --no-compiler-runtime --skip-plugin-types generic,networkinformation,tls,imageformats,iconengines,qmltooling --qmldir "$root\src\ui" "$out\FlowPlayer.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed' }
New-Item -ItemType Directory -Path "$out\imageformats" -Force | Out-Null
Copy-Item -LiteralPath "$root\deps\Qt\plugins\imageformats\qjpeg.dll" -Destination "$out\imageformats"
# Only the Windows platform plugin is needed. Do not deploy Widgets, multimedia, WebEngine or unused QML modules.
@('[Paths]','Plugins=.','QmlImports=qml') | Set-Content -LiteralPath "$out\qt.conf" -Encoding ascii
$manifest=[ordered]@{part=5;revision='clapperboard-icon';accepted=$false;manualAcceptance='pending';executableSha256=(Get-FileHash -LiteralPath "$out\FlowPlayer.exe" -Algorithm SHA256).Hash.ToLowerInvariant();qt='6.12.0';msvc='19.44.35228';libmpv='dde0671072';sqlite='3.53.4';report='tests/ICON.md';previousUiReport='tests/UI_POLISH.md';previousPartReport='tests/PART5_RESULTS.md'}
$manifest | ConvertTo-Json | Set-Content -LiteralPath "$out\build-manifest.json" -Encoding utf8
Write-Output $out
