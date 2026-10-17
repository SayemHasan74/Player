param([string]$File = '')
$root = Split-Path $PSScriptRoot -Parent
$env:PATH = "$root\deps\Qt\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$root\deps\Qt\plugins"
$env:QML_IMPORT_PATH = "$root\deps\Qt\qml"
if ($File) { & "$root\build\FlowPlayer.exe" $File } else { & "$root\build\FlowPlayer.exe" }
