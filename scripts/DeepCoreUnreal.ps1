# DeepCoreUnreal.ps1 -- pull, build, play.
#
# Double-click the .cmd next to this file. It fetches whatever has been pushed since last
# time, rebuilds the game module if any source changed, and launches.
#
# WHY THE EDITOR BINARY RATHER THAN A PACKAGED GAME
# The materials are built as node graphs in C++ at startup (see DeepCoreMaterials.cpp), which
# needs the shader compiler. That ships with the editor, not with a cooked build. Running
# UnrealEditor.exe with -game gives a normal fullscreen game with no editor UI; it is the
# standard "standalone game" path, not a debug mode.

$ErrorActionPreference = 'Stop'

$Repo = Split-Path -Parent $PSScriptRoot
$Proj = Join-Path $Repo 'unreal\DeepCore\DeepCore.uproject'

function Find-UnrealEngine {
    # Prefer the newest installed 5.x. Hard-coding a version means this script rots the next
    # time the engine updates, and the failure mode is a confusing "file not found".
    $roots = @('C:\Program Files\Epic Games', 'D:\Program Files\Epic Games', 'C:\Epic Games')
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

Write-Host ''
Write-Host '  DeepCore' -ForegroundColor Cyan
Write-Host '  --------' -ForegroundColor DarkCyan

$ue = Find-UnrealEngine
if ($null -eq $ue) {
    Write-Host '  Unreal Engine 5 was not found.' -ForegroundColor Red
    Write-Host '  Install it from the Epic Games Launcher, then run this again.'
    Read-Host '  Press Enter to close'
    exit 1
}
Write-Host "  engine   Unreal $($ue.Version)"

# --- update ---------------------------------------------------------------------------
Push-Location $Repo
try {
    $before = (git rev-parse HEAD 2>$null)
    git fetch --quiet origin 2>$null
    if ($LASTEXITCODE -eq 0) {
        $branch = (git rev-parse --abbrev-ref HEAD).Trim()
        git merge --ff-only "origin/$branch" --quiet 2>$null
    }
    $after = (git rev-parse HEAD 2>$null)
    if ($before -ne $after) {
        Write-Host "  update   pulled $($after.Substring(0,7))" -ForegroundColor Green
    } else {
        Write-Host '  update   already current'
    }
} catch {
    # An offline machine should still be able to play what it already has.
    Write-Host '  update   skipped (no network)' -ForegroundColor DarkYellow
} finally {
    Pop-Location
}

# --- build ----------------------------------------------------------------------------
# Live Coding holds a lock on the module and makes the build fail with a message that does not
# mention which process is responsible, so it is cleared up front.
Get-Process -Name LiveCodingConsole -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Host '  build    compiling game module...' -NoNewline
$buildBat = Join-Path $ue.Root 'Engine\Build\BatchFiles\Build.bat'
$log = & $buildBat DeepCoreEditor Win64 Development -Project="$Proj" -WaitMutex 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host ' FAILED' -ForegroundColor Red
    $log | Select-String -Pattern 'error' | Select-Object -First 20 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    Read-Host '  Press Enter to close'
    exit 1
}
Write-Host ' ok' -ForegroundColor Green

# --- play -----------------------------------------------------------------------------
Write-Host '  launch   starting game'
Write-Host ''
Write-Host '  WASD / arrows  pan       Q E  rotate      wheel  zoom' -ForegroundColor DarkGray
Write-Host '  left click     send the nearest crew member there' -ForegroundColor DarkGray
Write-Host ''

& $ue.Exe $Proj -game -windowed -resx=1600 -resy=900
