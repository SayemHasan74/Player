param([string]$Player = (Join-Path (Split-Path $PSScriptRoot -Parent) 'out\FlowPlayer\FlowPlayer.exe'), [switch]$Remove)
$ErrorActionPreference='Stop'
$extensions=@('.mkv','.mp4','.webm','.avi','.mov','.m4v','.ts','.m2ts','.mp3','.flac','.wav','.ogg','.opus','.m4a')
$root='HKCU:\Software\Classes'
$prog='FlowPlayer.Media'
if ($Remove) {
    foreach ($extension in $extensions) {
        $key="$root\$extension\OpenWithProgids"
        if (Test-Path -LiteralPath $key) { Remove-ItemProperty -LiteralPath $key -Name $prog -ErrorAction SilentlyContinue }
    }
    if (Test-Path -LiteralPath "$root\$prog") { Remove-Item -LiteralPath "$root\$prog" -Recurse }
    return
}
$resolved=(Resolve-Path -LiteralPath $Player).Path
New-Item -Path "$root\$prog\shell\open\command" -Force | Out-Null
Set-Item -LiteralPath "$root\$prog" -Value 'FlowPlayer media'
Set-Item -LiteralPath "$root\$prog\shell\open\command" -Value ('"'+$resolved+'" "%1"')
foreach ($extension in $extensions) {
    $key="$root\$extension\OpenWithProgids"
    New-Item -Path $key -Force | Out-Null
    New-ItemProperty -LiteralPath $key -Name $prog -PropertyType String -Value '' -Force | Out-Null
}
Write-Output 'Registered for Open with. Choose FlowPlayer in Windows Settings > Apps > Default apps to make it the default.'
