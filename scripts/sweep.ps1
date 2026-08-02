# sweep.ps1 -- capture a whole matrix of look variants in one go.
#
# The previous lighting attempt failed because each change was judged alone, against a frame
# that had several unrelated faults in it at once. Comparing a variant to a memory of the last
# one is hopeless. This captures every combination back to back so they can be laid side by
# side and ranked, which turns the question from an argument into a measurement.
#
# Builds ONCE, then runs each variant with -NoBuild.

[CmdletBinding()]
param(
    [string]$File   = "$PSScriptRoot\sweep.txt",   # one "name<TAB>tune" per line
    [double]$Boom   = 2600,
    [double]$Delay  = 14,
    [string]$OutDir = "$env:TEMP\deepcore-sweep",
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

$variants = @()
foreach ($line in Get-Content $File) {
    $t = $line.Trim()
    if (-not $t -or $t.StartsWith('#')) { continue }
    $parts = $t -split '\s+', 2
    $variants += [pscustomobject]@{ Name = $parts[0]; Tune = $(if ($parts.Count -gt 1) { $parts[1] } else { '' }) }
}

Write-Host "sweep  $($variants.Count) variants -> $OutDir" -ForegroundColor Cyan

$first = $true
foreach ($v in $variants) {
    $skipBuild = $NoBuild -or (-not $first)
    $first = $false
    Write-Host ("  {0,-22} {1}" -f $v.Name, $v.Tune) -ForegroundColor DarkGray
    # Hashtable, not array: array splatting binds POSITIONALLY, which sent '-Name' into $Boom.
    $a = @{ Name = $v.Name; Boom = $Boom; Delay = $Delay; OutDir = $OutDir; Tune = $v.Tune }
    if ($skipBuild) { $a['NoBuild'] = $true }
    & "$PSScriptRoot\shot.ps1" @a | Out-Null
}

Write-Host 'sweep complete' -ForegroundColor Green
Get-ChildItem $OutDir -Filter *.png | Select-Object Name, Length | Format-Table -AutoSize
