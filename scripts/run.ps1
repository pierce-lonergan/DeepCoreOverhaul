# run.ps1 : build DeepCoreOverhaul and launch it against YOUR installation of the game.
#
# THIS SCRIPT NEVER DOWNLOADS ANYTHING. It does not fetch the game, it does not fetch
# assets, and it will not guess at a path. The only legitimate copy of the 1999 executable
# is one you already own and installed yourself. If it cannot find one, it says exactly
# what is missing and stops.
#
#   .\scripts\run.ps1                       # auto-detect an installation
#   .\scripts\run.ps1 -GamePath "D:\Games\RR"
#   .\scripts\run.ps1 -Config Debug
#   .\scripts\run.ps1 -DeployOnly           # build and copy, do not launch
#
# If you have no installation, you can still watch this project's systems run:
#   docker compose -f docker/docker-compose.yml run --rm view
# That is the sandbox. It is not the game.

[CmdletBinding()]
param(
    [string]$GamePath = "",
    [ValidateSet("Debug", "Release")] [string]$Config = "Release",
    [switch]$DeployOnly,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot

function Say($msg)  { Write-Host "  $msg" }
function Good($msg) { Write-Host "  $msg" -ForegroundColor Green }
function Bad($msg)  { Write-Host "  $msg" -ForegroundColor Red }
function Warn($msg) { Write-Host "  $msg" -ForegroundColor Yellow }

Write-Host ""
Write-Host "DeepCoreOverhaul launcher" -ForegroundColor Cyan
Write-Host "-------------------------"

# ---------------------------------------------------------------------------
# 1. Locate an installation. Detection only -- never acquisition.
# ---------------------------------------------------------------------------
function Find-GameInstall {
    param([string]$Explicit)

    if ($Explicit) {
        if (Test-Path (Join-Path $Explicit "LegoRR.exe")) { return $Explicit }
        Bad "No LegoRR.exe under the path you gave: $Explicit"
        return $null
    }

    # Only places a legitimate install actually lives. No network, no archives.
    $candidates = @(
        "$env:ProgramFiles\LEGO Media\Games\Rock Raiders",
        "${env:ProgramFiles(x86)}\LEGO Media\Games\Rock Raiders",
        "$env:ProgramFiles\Rock Raiders",
        "${env:ProgramFiles(x86)}\Rock Raiders",
        "C:\Rock Raiders",
        "$env:USERPROFILE\Games\Rock Raiders"
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path (Join-Path $c "LegoRR.exe"))) { return $c }
    }
    return $null
}

$install = Find-GameInstall -Explicit $GamePath

if (-not $install) {
    Write-Host ""
    Bad "No installation of the game was found."
    Write-Host ""
    Say "This project is a modification. It loads alongside the original 1999"
    Say "executable and cannot run without it. That file is not distributed here"
    Say "and this script will not download it -- the only legitimate copy is one"
    Say "you already own."
    Write-Host ""
    Say "If you have it installed somewhere else:"
    Say "    .\scripts\run.ps1 -GamePath `"D:\path\to\the\game`""
    Write-Host ""
    Say "If you do not have it, you can still watch this project's own systems run,"
    Say "on a generated cavern, with no game required:"
    Write-Host ""
    Good "    docker compose -f docker/docker-compose.yml run --rm view"
    Write-Host ""
    Say "That is the SANDBOX. It runs the wave director, the spawn rules and the"
    Say "threat-audio decisions. It is not the game and does not claim to be."
    Write-Host ""
    exit 1
}

Good "Found installation: $install"

# Verify rather than assume. Read-only, always -- this script never writes to a
# game folder outside the two deploy targets below, and never modifies Lego.cfg.
$exe = Join-Path $install "LegoRR.exe"
$exeInfo = Get-Item $exe
Say ("LegoRR.exe   {0:N0} bytes" -f $exeInfo.Length)

$dataDir = Join-Path $install "Data"
if (-not (Test-Path $dataDir)) {
    Warn "No Data\ directory under the install. The game may not be fully installed."
}

# ---------------------------------------------------------------------------
# 2. Build
# ---------------------------------------------------------------------------
if (-not $SkipBuild) {
    Write-Host ""
    Say "Building $Config|x86 ..."

    $msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
        -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" 2>$null | Select-Object -First 1
    if (-not $msbuild) {
        $fallback = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path $fallback) { $msbuild = $fallback }
    }
    if (-not $msbuild) {
        Bad "MSBuild not found. Install Visual Studio Build Tools with the C++ workload."
        exit 1
    }

    & $msbuild (Join-Path $Repo "openlrr.sln") /t:openlrr /p:Configuration=$Config /p:Platform=x86 /v:minimal
    if ($LASTEXITCODE -ne 0) { Bad "Build failed."; exit 1 }
    Good "Build succeeded."
}

$dll = Join-Path $Repo ("bin\openlrr" + $(if ($Config -eq "Debug") { "-d" } else { "" }) + ".dll")
if (-not (Test-Path $dll)) { Bad "Expected DLL not found: $dll"; exit 1 }

# ---------------------------------------------------------------------------
# 3. Deploy settings and generated audio
# ---------------------------------------------------------------------------
Write-Host ""
Say "Deploying..."

$settingsDir = Join-Path $install "Settings"
New-Item -ItemType Directory -Force -Path $settingsDir | Out-Null
$cfgSrc = Join-Path $Repo "data\Settings\DeepCore.cfg"
$cfgDst = Join-Path $settingsDir "DeepCore.cfg"
if (Test-Path $cfgDst) {
    Warn "DeepCore.cfg already present; leaving your copy alone."
    Say  "  (delete it and re-run to take the shipped defaults)"
} else {
    Copy-Item $cfgSrc $cfgDst
    Good "DeepCore.cfg -> $cfgDst"
}

$audioSrc = Join-Path $Repo "assets\audio\threat"
$audioDst = Join-Path $install "Data\DeepCore"
if (Test-Path $audioSrc) {
    New-Item -ItemType Directory -Force -Path $audioDst | Out-Null
    Copy-Item (Join-Path $audioSrc "*.wav") $audioDst -Force
    $n = (Get-ChildItem $audioDst -Filter *.wav).Count
    Good "$n generated cue(s) -> $audioDst"
    Say  "  (DeepCore registers these itself; you do not edit Lego.cfg)"
}

if ($DeployOnly) { Write-Host ""; Good "Deploy complete. Not launching (-DeployOnly)."; exit 0 }

# ---------------------------------------------------------------------------
# 4. Launch
# ---------------------------------------------------------------------------
Write-Host ""
Say "Launching..."

$injector = Join-Path $Repo ("bin\OpenLRR" + $(if ($Config -eq "Debug") { "-d" } else { "" }) + ".exe")
if (-not (Test-Path $injector)) {
    Bad "Injector not found: $injector"
    Say "Build the openlrr-injector project, or generate a launcher with OpenLRR-MakeExe."
    exit 1
}

$log = Join-Path $install "DeepCore.log"
if (Test-Path $log) { Remove-Item $log -Force }

Push-Location $install
try {
    Start-Process -FilePath $injector -WorkingDirectory $install
    Good "Launched. Watching $log ..."
    Write-Host ""

    # The log is what makes a first run diagnosable at all. Several of this project's
    # most important checks fail SILENTLY -- an unresolved species name, a cue that never
    # registered -- so tailing it is not optional.
    $waited = 0
    while (-not (Test-Path $log) -and $waited -lt 20) { Start-Sleep -Milliseconds 500; $waited++ }
    if (Test-Path $log) { Get-Content $log -Wait -Tail 100 }
    else {
        Warn "No DeepCore.log appeared after 10s."
        Say "That usually means the DLL was not injected. Check that $dll is next to the injector."
    }
}
finally { Pop-Location }
