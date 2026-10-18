$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$ff="$root\deps\ffmpeg\ffmpeg.exe"
if (!(Test-Path "$root\tests\media\h264-1080p60-short.mkv")) { throw 'Run scripts/generate-test-media.ps1 first.' }
Push-Location $root
try {
    & $ff -y -hide_banner -loglevel error -stream_loop 1 -i tests/media/h264-1080p60-short.mkv -f lavfi -i 'sine=frequency=220:sample_rate=48000' -f lavfi -i 'sine=frequency=440:sample_rate=48000' -i tests/fixtures/part3/sample.srt -i tests/fixtures/part3/sample.ass -f ffmetadata -i tests/fixtures/part3/chapters.ffmetadata -map 0:v -map 1:a -map 2:a -map 3:s -map 4:s -map_metadata 5 -map_chapters 5 -c:v copy -c:a aac -c:s copy -metadata:s:a:0 language=eng -metadata:s:a:1 language=jpn -metadata:s:s:0 language=eng -metadata:s:s:1 language=jpn -t 24 tests/media/part3-multi.mkv
    if ($LASTEXITCODE) { throw 'Multi-track fixture mux failed' }
    New-Item -ItemType Directory -Force tests/media/Subs | Out-Null
    Copy-Item tests/fixtures/part3/sample.srt tests/media/Subs/part3-multi.en.srt
    & $ff -y -hide_banner -loglevel error -f lavfi -i 'color=c=0x193c5a:s=400x400' -frames:v 1 -threads 1 tests/media/part3-cover.png
    if ($LASTEXITCODE) { throw 'Cover fixture generation failed' }
    & $ff -y -hide_banner -loglevel error -f lavfi -i 'sine=frequency=220:sample_rate=48000' -i tests/media/part3-cover.png -map 0:a -map 1:v -c:a libmp3lame -c:v copy -disposition:v attached_pic -metadata:s:v title='FlowPlayer test cover' -t 24 tests/media/part3-cover.mp3
    if ($LASTEXITCODE) { throw 'MP3 fixture generation failed' }
} finally { Pop-Location }
