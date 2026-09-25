$ErrorActionPreference = "Stop"

Write-Host "ArtistStudio Stem Lab - local AI setup" -ForegroundColor Cyan

$python = $null
foreach ($candidate in @("py -3", "python", "python3")) {
    try {
        if ($candidate -eq "py -3") {
            & py -3 --version *> $null
            if ($LASTEXITCODE -eq 0) { $python = "py -3"; break }
        } else {
            & $candidate --version *> $null
            if ($LASTEXITCODE -eq 0) { $python = $candidate; break }
        }
    } catch {}
}

if (-not $python) {
    Write-Host "Python 3 was not found." -ForegroundColor Yellow
    Write-Host "Install Python 3 from https://www.python.org/downloads/ then run this script again."
    exit 1
}

Write-Host "Using: $python" -ForegroundColor Green

if ($python -eq "py -3") {
    & py -3 -m pip install --upgrade pip
    & py -3 -m pip install --upgrade demucs
    & py -3 -m demucs --help *> $null
} else {
    & $python -m pip install --upgrade pip
    & $python -m pip install --upgrade demucs
    & $python -m demucs --help *> $null
}

if ($LASTEXITCODE -ne 0) {
    throw "Demucs installation check failed."
}

Write-Host "Demucs is ready. Open ArtistStudio Stem Lab and separate a track." -ForegroundColor Green
