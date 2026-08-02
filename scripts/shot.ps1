# shot.ps1 -- build, run, capture a frame, report. The inner loop for visual work.
#
# Looking at a rendered frame is the only reliable way to know what this game looks like. Two
# separate builds during the Unreal port compiled clean, logged clean, and drew a black screen;
# both were caught by a screenshot and neither could have been caught by reading code. So the
# capture path is a first-class tool rather than something reconstructed each time it is needed.
#
#   .\shot.ps1                        build, capture, copy to scratch
#   .\shot.ps1 -Boom 700              close in on a unit
#   .\shot.ps1 -Delay 20              let Lumen and shaders settle longer
#   .\shot.ps1 -Name wide             name the output, so captures can be compared
#   .\shot.ps1 -NoBuild               re-run without recompiling

[CmdletBinding()]
param(
    [double]$Boom  = 0,           # 0 = use the game's own default
    [double]$Delay = 12,
    [string]$Name  = 'shot',
    [string]$Tune  = '',            # -DeepCoreTune payload, e.g. 'ev=3,amb=0.1'
    [switch]$NoBuild,
    [int]$ResX = 1600,
    [int]$ResY = 900,
    [string]$OutDir = "$env:TEMP\deepcore-shots"
)

$ErrorActionPreference = 'Stop'

$Repo = Split-Path -Parent $PSScriptRoot
$Proj = Join-Path $Repo 'unreal\DeepCore\DeepCore.uproject'
$Saved = Join-Path $Repo 'unreal\DeepCore\Saved'
$LogFile = Join-Path $Saved 'Logs\DeepCore.log'

function Find-UnrealEngine {
    $roots = @('C:\Program Files\Epic Games', 'D:\Program Files\Epic Games')
    $best = $null
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        Get-ChildItem $root -Directory -Filter 'UE_*' -ErrorAction SilentlyContinue | ForEach-Object {
            $exe = Join-Path $_.FullName 'Engine\Binaries\Win64\UnrealEditor.exe'
            if (Test-Path $exe) {
                $v = [version]($_.Name -replace '^UE_', '')
                if ($null -eq $best -or $v -gt $best.Version) {
                    $best = [pscustomobject]@{ Version = $v; Root = $_.FullName; Exe = $exe }
                }
            }
        }
    }
    return $best
}

$ue = Find-UnrealEngine
if ($null -eq $ue) { throw 'Unreal Engine 5 not found.' }

# A leftover editor or Live Coding session holds the module lock, and the build failure message
# does not name the process responsible.
Get-Process -Name 'UnrealEditor', 'LiveCodingConsole' -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

if (-not $NoBuild) {
    Write-Host 'build  ' -NoNewline
    $out = & (Join-Path $ue.Root 'Engine\Build\BatchFiles\Build.bat') `
              DeepCoreEditor Win64 Development -Project="$Proj" -WaitMutex 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'FAILED' -ForegroundColor Red
        # Compiler diagnostics carry a file(line,col) prefix; the surrounding UBT chatter does not.
        $out | Select-String -Pattern '(error|warning) [A-Z]+\d+' |
               Select-Object -First 25 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
        exit 1
    }
    $warn = @($out | Select-String -Pattern 'warning [A-Z]+\d+')
    if ($warn.Count -gt 0) {
        Write-Host "ok ($($warn.Count) warnings)" -ForegroundColor Yellow
        $warn | Select-Object -First 10 | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkYellow }
    } else {
        Write-Host 'ok (0 warnings)' -ForegroundColor Green
    }
}

if (Test-Path $LogFile) { Remove-Item $LogFile -Force -ErrorAction SilentlyContinue }

# Delete the previous capture BEFORE the run. Without this a run that fails to capture leaves the
# last run's PNG in place, and the copy below silently publishes it under the new name -- which is
# how a sweep produces two "different" variants that are byte-identical files. Any comparison drawn
# from those is worthless, and nothing in the output says so.
$ShotFile = Join-Path $Saved 'Screenshots\WindowsEditor\DeepCore.png'
if (Test-Path $ShotFile) { Remove-Item $ShotFile -Force -ErrorAction SilentlyContinue }

$runArgs = @("`"$Proj`"", '-game', '-windowed', "-resx=$ResX", "-resy=$ResY", '-log', "-DeepCoreShot=$Delay")
if ($Boom -gt 0) { $runArgs += "-DeepCoreBoom=$Boom" }
if ($Tune)       { $runArgs += "-DeepCoreTune=$Tune" }

Write-Host "run    capturing at t+${Delay}s..." -NoNewline
$p = Start-Process -FilePath $ue.Exe -ArgumentList $runArgs -PassThru
# The game exits itself 4s after the capture; the wait is a backstop for a hang, not the plan.
$null = $p.WaitForExit([int](($Delay + 45) * 1000))
if (-not $p.HasExited) { $p | Stop-Process -Force; Write-Host ' (killed)' -ForegroundColor Yellow }
else { Write-Host ' done' -ForegroundColor Green }

New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$src = Join-Path $Saved 'Screenshots\WindowsEditor\DeepCore.png'
$dst = Join-Path $OutDir "$Name.png"
if (Test-Path $src) {
    Copy-Item $src $dst -Force
    Write-Host "shot   $dst" -ForegroundColor Cyan
} else {
    Write-Host 'shot   NO IMAGE PRODUCED' -ForegroundColor Red
}

if (Test-Path $LogFile) {
    Write-Host 'log'
    Select-String -Path $LogFile -Pattern 'DeepCore: ' |
        ForEach-Object { '  ' + ($_.Line -replace '^.*LogTemp: \w+: ', '') }

    # Scoped to our own log lines plus genuine engine failures. A bare "LogTemp: Error" also
    # matches the engine's UnifiedErrorTest self-test, which fires every boot and is not a fault.
    # "Failed to compile Material" is logged at WARNING level, not Error, and the engine then
    # silently swaps in the Default Material -- a frame that renders perfectly while showing none
    # of this project's shading work. That exact failure went unnoticed across several sessions of
    # look tuning because nothing here matched it. It is the single most expensive thing this log
    # can say, so it is scanned for explicitly.
    $errs = @(Select-String -Path $LogFile -Pattern 'DeepCore: .*(Error|FAIL)|Fatal error|Ensure condition failed|LogMaterial: Error|LogProceduralMesh: Error|Failed to compile Material|function definition is not allowed here')
    if ($errs.Count -gt 0) {
        Write-Host 'errors' -ForegroundColor Red
        $errs | Select-Object -First 10 | ForEach-Object { Write-Host "  $($_.Line)" -ForegroundColor Red }
    }
}
