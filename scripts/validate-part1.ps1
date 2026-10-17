param([switch]$Build,[string]$RunName='part1-final-20261018',[switch]$StopOnPlaybackFailure)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if ($Build) { & "$PSScriptRoot\build.ps1" -Target FlowPlayer }
& "$PSScriptRoot\deploy.ps1"
$python="$root\.tools\Scripts\python.exe"
$exe="$root\out\FlowPlayer\FlowPlayer.exe"
$caseNames=@("$RunName-T1-hevc","$RunName-T2-h264","$RunName-T2-vp9","$RunName-T6-pause","$RunName-T8-exit")
foreach ($name in $caseNames) { if (Test-Path -LiteralPath "$root\tests\results\$name") { throw "Results already exist: $name. Use a new -RunName; preserve earlier results." } }
$failures=[System.Collections.Generic.List[string]]::new()
function Invoke-Benchmark([string]$File,[string]$Name,[int]$Seconds,[string]$Scenario='') {
    $arguments=@("$PSScriptRoot\benchmark.py","$root\tests\media\$File",'--seconds',"$Seconds",'--name',$Name,'--exe',$exe,'--standalone')
    if ($Scenario) { $arguments+=@('--scenario',$Scenario) }
    & $python @arguments
    if ($LASTEXITCODE) {
        $failures.Add("$Name (exit $LASTEXITCODE)")
        if ($StopOnPlaybackFailure) { throw "Playback failed: $Name; raw evidence retained. Remaining cases not run." }
    }
}
& $python "$PSScriptRoot\test-exit.py" --exe $exe --standalone --name "$RunName-T8-exit"
if ($LASTEXITCODE) { $failures.Add('T8 exit test') }
# Run the previously failing codec first, so a regression does not waste a full pass.
Invoke-Benchmark 'vp9-4k30.webm' "$RunName-T2-vp9" 320
Invoke-Benchmark 'hevc-4k60-main10.mkv' "$RunName-T1-hevc" 320
Invoke-Benchmark 'h264-1080p60.mkv' "$RunName-T2-h264" 320
Invoke-Benchmark 'hevc-4k60-main10.mkv' "$RunName-T6-pause" 75 'pause'
& $python "$PSScriptRoot\summarize-results.py" --names @($caseNames[0..3])
if ($LASTEXITCODE) { throw 'Summary failed' }
foreach ($name in $caseNames[0..2]) {
    $result=Get-Content -LiteralPath "$root\tests\results\$name\summary.json" -Raw | ConvertFrom-Json
    if ($result.workingSetMiB.max*1048576 -ge 500000000) { $failures.Add("$name resident RAM exceeds user's revised 500 MB limit") }
}
& $python "$PSScriptRoot\report-results.py" --prefix $RunName
if ($LASTEXITCODE) { $failures.Add('Acceptance verdict has unresolved checks; see tests/RESULTS.md') }
# Visual/input acceptance remains a separate manual step, recorded in RESULTS.md.
if ($failures.Count) { throw ('Acceptance failures: '+($failures -join ', ')) }
