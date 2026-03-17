$Url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-devel-2.30.0-mingw.tar.gz"
$Output = "SDL2-devel.tar.gz"
$Destination = "external"

if (-not (Test-Path $Destination)) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
}

Write-Host "Downloading SDL2 from $Url..."
Invoke-WebRequest -Uri $Url -OutFile $Output

Write-Host "Extracting..."
tar -xzf $Output -C $Destination

# Rename/Move for easier access
$ExtractedDir = Get-ChildItem -Path $Destination -Filter "SDL2-*-mingw" | Select-Object -First 1
if ($ExtractedDir) {
    $Target = "$Destination/SDL2"
    if (Test-Path $Target) { Remove-Item -Recurse -Force $Target }
    Rename-Item -Path $ExtractedDir.FullName -NewName "SDL2"
    Write-Host "SDL2 installed to $Target"
} else {
    Write-Error "Extraction failed or directory structure unexpected."
}

Remove-Item $Output
