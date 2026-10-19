param([switch]$ExecutableOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'out\FlowPlayer'
New-Item -ItemType Directory -Path $out -Force | Out-Null
if($ExecutableOnly){
    # Same pinned runtime only. Preserve an open player's mapped executable;
    # replace the on-disk path without closing or restarting that process.
    if(!(Test-Path -LiteralPath "$out\Qt6Quick.dll") -or !(Test-Path -LiteralPath "$out\libmpv-2.dll")){throw 'Run a full deployment first.'}
    $token=[Guid]::NewGuid().ToString('N')
    $target=Join-Path $out 'FlowPlayer.exe'
    $stage=Join-Path $out "FlowPlayer-update-$token.exe"
    $backup=Join-Path $out "FlowPlayer-previous-$token.exe"
    $allowed=[IO.Path]::GetFullPath($out).TrimEnd('\')+'\'
    foreach($path in @($target,$stage,$backup)){if(![IO.Path]::GetFullPath($path).StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Deployment path outside output directory'}}
    Copy-Item -LiteralPath "$root\build\FlowPlayer.exe" -Destination $stage
    if(Test-Path -LiteralPath $target){Move-Item -LiteralPath $target -Destination $backup}
    try{Move-Item -LiteralPath $stage -Destination $target}
    catch{if((Test-Path -LiteralPath $backup) -and !(Test-Path -LiteralPath $target)){Move-Item -LiteralPath $backup -Destination $target};throw}
}else{
Copy-Item -LiteralPath "$root\build\FlowPlayer.exe","$root\deps\mpv\libmpv-2.dll" -Destination $out
& "$root\deps\Qt\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw --no-compiler-runtime --skip-plugin-types generic,networkinformation,tls,imageformats,iconengines,qmltooling --qmldir "$root\src\ui" "$out\FlowPlayer.exe"
if ($LASTEXITCODE) { throw 'Qt deployment failed' }
New-Item -ItemType Directory -Path "$out\imageformats" -Force | Out-Null
Copy-Item -LiteralPath "$root\deps\Qt\plugins\imageformats\qjpeg.dll" -Destination "$out\imageformats"
}
# Only the Windows platform plugin is needed. Do not deploy Widgets, multimedia, WebEngine or unused QML modules.
@('[Paths]','Plugins=.','QmlImports=qml') | Set-Content -LiteralPath "$out\qt.conf" -Encoding ascii
$manifest=[ordered]@{part=5;revision='final-pass';accepted=$false;manualAcceptance='pending';executableSha256=(Get-FileHash -LiteralPath "$out\FlowPlayer.exe" -Algorithm SHA256).Hash.ToLowerInvariant();qt='6.12.0';msvc='19.44.35228';libmpv='dde0671072';sqlite='3.53.4';report='tests/FINAL_PASS.md';previousUiReport='tests/UI_POLISH.md';previousPartReport='tests/PART5_RESULTS.md'}
$manifest | ConvertTo-Json | Set-Content -LiteralPath "$out\build-manifest.json" -Encoding utf8
Write-Output $out
