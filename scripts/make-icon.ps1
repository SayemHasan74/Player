# Package the approved PNG into standard multi-resolution Windows ICO entries.
param([string]$Source=(Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\flowplayer.png'),[string]$Destination=(Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\flowplayer.ico'))
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$image=[Drawing.Bitmap]::FromFile($Source)
try {
    if ($image.GetPixel(0,0).A -ne 0) { throw 'Expected a transparent PNG background' }
    $sizes=@(16,24,32,48,64,128,256)
    $entries=@()
    foreach($size in $sizes){
        $bitmap=[Drawing.Bitmap]::new($size,$size,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics=[Drawing.Graphics]::FromImage($bitmap)
        $stream=[IO.MemoryStream]::new()
        try {
            $graphics.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.DrawImage($image,0,0,$size,$size)
            $bitmap.Save($stream,[Drawing.Imaging.ImageFormat]::Png)
            $entries+=,@{Size=$size;Bytes=$stream.ToArray()}
        } finally {$stream.Dispose();$graphics.Dispose();$bitmap.Dispose()}
    }
    $file=[IO.File]::Create($Destination);$writer=[IO.BinaryWriter]::new($file)
    try {
        $writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$entries.Count)
        $offset=6+16*$entries.Count
        foreach($entry in $entries){$dimension=if($entry.Size -eq 256){0}else{$entry.Size};$writer.Write([byte]$dimension);$writer.Write([byte]$dimension);$writer.Write([byte]0);$writer.Write([byte]0);$writer.Write([uint16]1);$writer.Write([uint16]32);$writer.Write([uint32]$entry.Bytes.Length);$writer.Write([uint32]$offset);$offset+=$entry.Bytes.Length}
        foreach($entry in $entries){$writer.Write([byte[]]$entry.Bytes)}
    } finally {$writer.Dispose();$file.Dispose()}
    Write-Output "Transparent icon packaged: $($sizes -join ', ') px"
} finally {$image.Dispose()}
