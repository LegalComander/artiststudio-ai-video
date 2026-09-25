$ErrorActionPreference = "Stop"

Write-Host "ArtistStudio Stem Lab - local AI setup" -ForegroundColor Cyan

$engineRoot = Join-Path $env:APPDATA "ArtistStudio\StemLab\engine"
$venv = Join-Path $engineRoot "venv"
$managedPython = Join-Path $venv "Scripts\python.exe"

New-Item -ItemType Directory -Force -Path $engineRoot | Out-Null

function Find-Python {
    $direct = @(
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python313\python.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\python.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python311\python.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python310\python.exe")
    )

    foreach ($path in $direct) {
        if (Test-Path $path) { return @($path) }
    }

    foreach ($version in @("3.13", "3.12", "3.11", "3.10")) {
        try {
            & py "-$version" --version *> $null
            if ($LASTEXITCODE -eq 0) { return @("py", "-$version") }
        } catch {}
    }

    try {
        & python --version *> $null
        if ($LASTEXITCODE -eq 0) { return @("python") }
    } catch {}

    return $null
}

$python = Find-Python

if (-not $python) {
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Host "Python was not found. Installing Python 3.12..." -ForegroundColor Yellow
        & winget install --id Python.Python.3.12 -e --silent --accept-package-agreements --accept-source-agreements
        $python = Find-Python
    }
}

if (-not $python) {
    throw "Python 3.10+ was not found and automatic installation was unavailable."
}

Write-Host "Creating private ArtistStudio AI environment..." -ForegroundColor Cyan
if ($python.Count -eq 2) {
    & $python[0] $python[1] -m venv $venv
} else {
    & $python[0] -m venv $venv
}

if (-not (Test-Path $managedPython)) {
    throw "ArtistStudio private Python environment could not be created."
}

Write-Host "Installing Demucs 4.1.0..." -ForegroundColor Cyan
& $managedPython -m pip install --upgrade pip
& $managedPython -m pip install demucs==4.1.0
& $managedPython -m demucs --help *> $null

if ($LASTEXITCODE -ne 0) {
    throw "Demucs installation check failed."
}

Write-Host "ArtistStudio local AI engine is ready." -ForegroundColor Green
Write-Host "Open Stem Lab. The AI Engine button should show Ready." -ForegroundColor Green
