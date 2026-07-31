# DeepCoreSandbox.ps1 : the double-clickable launcher.
#
# Pulls the latest committed code, rebuilds only if something actually changed, and runs
# the sandbox. Designed so that the moment a commit lands on origin/main, the next
# double-click is running it.
#
# WHAT THIS RUNS, PRECISELY
# This launches the SANDBOX, not the game. The sandbox executes the subsystems this
# project owns -- the wave director, the spawn fairness rules, the threat-audio decisions --
# against a procedurally generated cavern. It needs no installation of the 1999 game and
# contains no copyrighted content.
#
# To run the actual game with these modifications you need your own installed copy; use
# scripts\run.ps1 for that. This launcher deliberately does not pretend otherwise.
#
# SAFETY
#   - Never force-pulls. If the working tree is dirty or the branch has diverged, it says
#     so and runs the build you already have rather than destroying local work.
#   - Never downloads game content. It only pulls this repository.
#   - Works offline: no network just means "run what is already built".

param(
    [switch]$NoUpdate,        # skip the git pull
    [switch]$Sandbox,         # run the headless simulation viewer instead of the game
    [switch]$Headless,        # text summary only
    [int]$Seed = 0,
    [int]$Seconds = 600
)

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot

function Line($t) { Write-Host $t }
function Good($t) { Write-Host $t -ForegroundColor Green }
function Warn($t) { Write-Host $t -ForegroundColor Yellow }
function Bad($t)  { Write-Host $t -ForegroundColor Red }
function Dim($t)  { Write-Host $t -ForegroundColor DarkGray }

$host.UI.RawUI.WindowTitle = "DeepCoreOverhaul Sandbox"

Write-Host ""
Write-Host "  DeepCore" -ForegroundColor Cyan
Write-Host "  ========" -ForegroundColor Cyan
Dim   "  A subterranean mining game built on this project's own systems."
Dim   "  NOT LEGO Rock Raiders -- that is a 1999 commercial game this cannot"
Dim   "  contain or reproduce. For a faithful free remake, play Manic Miners."
Write-Host ""

Push-Location $Repo
try {
    # -----------------------------------------------------------------------
    # 1. Update
    # -----------------------------------------------------------------------
    $rebuild = $false
    $headBefore = (& git rev-parse HEAD 2>$null)

    if (-not $NoUpdate) {
        Line "  Checking for updates..."

        $dirty = (& git status --porcelain 2>$null)
        if ($dirty) {
            Warn "  You have uncommitted local changes. Skipping update so nothing is lost."
            Dim  "    (commit or stash them, then relaunch, to pick up the latest)"
        }
        else {
            & git fetch --quiet origin 2>$null
            if ($LASTEXITCODE -ne 0) {
                Warn "  Could not reach GitHub. Running the version you already have."
            }
            else {
                $local  = (& git rev-parse '@' 2>$null)
                $remote = (& git rev-parse '@{u}' 2>$null)
                $base   = (& git merge-base '@' '@{u}' 2>$null)

                if ($local -eq $remote) {
                    Good "  Already up to date."
                }
                elseif ($local -eq $base) {
                    $n = (& git rev-list --count "$local..$remote" 2>$null)
                    Line "  $n new commit(s). Updating..."
                    & git merge --ff-only '@{u}' --quiet 2>$null
                    if ($LASTEXITCODE -eq 0) {
                        Good "  Updated to $((& git log -1 --format='%h %s'))"
                        $rebuild = $true
                    } else {
                        Warn "  Fast-forward failed; running the version you have."
                    }
                }
                else {
                    Warn "  Your branch has diverged from origin. Not touching it."
                }
            }
        }
    }

    # -----------------------------------------------------------------------
    # 2. Build, but only when needed
    # -----------------------------------------------------------------------
    # The GAME is the default. The sandbox viewer is still there behind -Sandbox, because
    # it is what CI asserts on and what makes the director's decisions inspectable.
    $useGame = -not ($Sandbox -or $Headless)
    $exe   = Join-Path $Repo $(if ($useGame) { "bin\DeepCoreGame.exe" } else { "bin\sandbox.exe" })
    $proj  = Join-Path $Repo $(if ($useGame) { "src\game\deepcoregame.vcxproj" } else { "src\sandbox\sandbox.vcxproj" })
    $stamp = Join-Path $Repo $(if ($useGame) { "bin\.game-built-at" } else { "bin\.sandbox-built-at" })

    if (-not (Test-Path $exe)) { $rebuild = $true }
    elseif (Test-Path $stamp) {
        $builtFrom = (Get-Content $stamp -ErrorAction SilentlyContinue | Select-Object -First 1)
        $now = (& git rev-parse HEAD 2>$null)
        if ($builtFrom -ne $now) { $rebuild = $true }
    }
    else { $rebuild = $true }

    if ($rebuild) {
        Line "  Building..."

        $msbuild = $null
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" 2>$null | Select-Object -First 1
        }
        if (-not $msbuild) {
            $fallback = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path $fallback) { $msbuild = $fallback }
        }

        if (-not $msbuild) {
            Bad  "  MSBuild not found, so the sandbox cannot be rebuilt here."
            if (Test-Path $exe) { Warn "  Running the existing build instead." }
            else {
                Bad "  And there is no existing build to fall back on."
                Dim "  Install Visual Studio Build Tools with the C++ workload, then relaunch."
                Read-Host "`n  Press Enter to close"
                exit 1
            }
        }
        else {
            # No /p:SolutionDir here, deliberately. Passing a path that ends in a backslash
            # to a native command from PowerShell mangles it -- the trailing \ escapes the
            # closing quote and swallows the following arguments, which produced a genuinely
            # baffling "Illegal characters in path" from MSBuild. sandbox.vcxproj now derives
            # its output directory from $(MSBuildProjectDirectory), so the variable that
            # caused the problem is simply not needed.
            $buildArgs = @(
                $proj,
                "/p:Configuration=Release",
                "/p:Platform=x86",
                "/v:minimal",
                "/nologo"
            )

            $buildLog = & $msbuild @buildArgs 2>&1
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) {
                Bad "  Build failed."
                Write-Host ""
                # Show the actual reason. A launcher that reports failure without the cause
                # just moves the problem somewhere less convenient.
                $buildLog | Select-Object -Last 25 | ForEach-Object { Dim "    $_" }
                Write-Host ""
                Read-Host "  Press Enter to close"
                exit 1
            }
            (& git rev-parse HEAD) | Set-Content $stamp
            Good "  Built."
        }
    }
    else {
        Dim "  Build is current."
    }

    # -----------------------------------------------------------------------
    # 3. Run
    # -----------------------------------------------------------------------
    if ($Seed -le 0) { $Seed = Get-Random -Minimum 1 -Maximum 100000 }

    Write-Host ""
    Dim "  commit  $((& git log -1 --format='%h %s'))"
    Dim "  seed    $Seed"
    Write-Host ""
    Start-Sleep -Milliseconds 700

    if ($useGame) {
        Good "  Launching the game. Close its window to return here."
        Write-Host ""
        & $exe | Out-Null
    }
    else {
        $args = @()
        if ($Headless) { $args += "--seed"; $args += "$Seed" }
        else           { $args += "--view"; $args += "--seed"; $args += "$Seed"; $args += "--seconds"; $args += "$Seconds" }
        & $exe @args
    }

    Write-Host ""
    Good "  Run complete."
    Dim  "  Every launch pulls the latest commit and rebuilds if anything changed."
    Dim  "  This was the sandbox. To run the real game you need your own copy:"
    Dim  "    .\scripts\run.ps1"
    Write-Host ""
    Read-Host "  Press Enter to close"
}
finally {
    Pop-Location
}
