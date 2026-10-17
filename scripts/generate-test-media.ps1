$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$ff = "$root\deps\ffmpeg\ffmpeg.exe"
$media = "$root\tests\media"
New-Item -ItemType Directory -Path $media -Force | Out-Null
& $ff -y -hide_banner -loglevel warning -f lavfi -i 'testsrc2=size=3840x2160:rate=60' -t 12 -c:v hevc_nvenc -preset p1 -profile:v main10 -pix_fmt p010le -b:v 20M -an "$media\hevc-4k60-main10-short.mkv"
if ($LASTEXITCODE) { throw 'HEVC generation failed' }
& $ff -y -hide_banner -loglevel warning -f lavfi -i 'testsrc2=size=1920x1080:rate=60' -t 12 -c:v h264_nvenc -preset p1 -b:v 8M -an "$media\h264-1080p60-short.mkv"
if ($LASTEXITCODE) { throw 'H264 generation failed' }
& $ff -y -hide_banner -loglevel warning -f lavfi -i 'testsrc2=size=3840x2160:rate=30' -t 6 -c:v libvpx-vp9 -deadline realtime -cpu-used 8 -row-mt 1 -threads 8 -b:v 12M "$media\vp9-4k30-short.webm"
if ($LASTEXITCODE) { throw 'VP9 generation failed' }
foreach ($name in @('hevc-4k60-main10','h264-1080p60')) {
    & $ff -y -hide_banner -loglevel warning -stream_loop 29 -i "$media\$name-short.mkv" -f lavfi -i 'sine=frequency=220:sample_rate=48000' -map 0:v -map 1:a -c:v copy -c:a aac -t 360 "$media\$name.mkv"
    if ($LASTEXITCODE) { throw 'Clip expansion failed' }
}
& $ff -y -hide_banner -loglevel warning -stream_loop 59 -i "$media\vp9-4k30-short.webm" -c copy -t 360 "$media\vp9-4k30.webm"
if ($LASTEXITCODE) { throw 'VP9 expansion failed' }
