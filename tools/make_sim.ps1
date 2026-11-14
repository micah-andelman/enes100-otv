# Builds the single-file simulator sketch: sim/otv_sim/otv_sim.ino
# Paste that file into the ENES100 browser simulator.
# Run from the repo root:
#   powershell -ExecutionPolicy Bypass -File tools/make_sim.ps1
#
# The simulator (JSCPP interpreter) cannot handle #if/#ifdef/#ifndef,
# so the sources are run through the Arduino core's own C preprocessor
# with SIM=1. The output has no conditionals and no #defines left.
#
# MS5 demos: -Task N builds MS5_TASK=N into -Out (otv.ps1 otv4..otv7b use this).

param([int]$Task = 0, [string]$Out = '')

$ErrorActionPreference = 'Stop'

# Without -Out a demo build would overwrite the full-mission otv_sim.ino.
if ($Task -ne 0 -and -not $Out) { throw '-Task needs -Out (use otv4..otv7b, which pass it)' }

$root   = Split-Path -Parent $PSScriptRoot
$srcDir = Join-Path $root 'otv'
if ($Out) {
    $out    = $Out
    $outDir = Split-Path -Parent $out
} else {
    $outDir = Join-Path $root 'sim\otv_sim'
    $out    = Join-Path $outDir 'otv_sim.ino'
}

# Concatenation order matters: config and prototypes first.
$files = @('config.h', 'otv.h', 'otv.ino', 'drive.ino', 'mission.ino', 'nav.ino', 'tests.ino', 'ms5.ino')

# Lines removed before preprocessing (the header below adds the library
# includes once, in the order the simulator needs).
$strip = '^\s*#\s*(pragma\s+once\b|include\s*[<"](Arduino\.h|config\.h|otv\.h|Enes100\.h|Tank\.h)[>"])'

# arm-none-eabi-g++ ships with the arduino:renesas_uno core.
$gccRoot = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\arduino\tools\arm-none-eabi-gcc'
$gpp = Get-ChildItem -Path $gccRoot -Recurse -Filter 'arm-none-eabi-g++.exe' -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $gpp) { throw "arm-none-eabi-g++ not found under $gccRoot (install the arduino:renesas_uno core)" }

$src = New-Object System.Collections.Generic.List[string]
foreach ($f in $files) {
    $path = Join-Path $srcDir $f
    if (-not (Test-Path $path)) { throw "Missing source file: $path" }
    $src.Add("// ===================== otv/$f =====================")
    foreach ($line in Get-Content -Path $path) {
        if ($line -notmatch $strip) { $src.Add($line) }
    }
}

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) 'otv_sim_concat.cpp'
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($tmp, ($src -join "`n") + "`n", $utf8NoBom)

# -E preprocess only, -P no line markers, -undef no compiler-predefined
# macros. No -C: comments are stripped, so the code shown in the simulator
# has none (the commented source is otv/).
$pre = & $gpp.FullName -E -P -undef -x c++ -DSIM=1 -DTEST_MODE=0 "-DMS5_TASK=$Task" $tmp
if ($LASTEXITCODE -ne 0) { throw "Preprocessing failed (exit $LASTEXITCODE)" }
Remove-Item $tmp

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add('#include <cmath>')
$lines.Add('#include "Arduino.h"')
$lines.Add('#include "Enes100.h"')
$lines.Add('#include "Tank.h"')
$lines.Add('')

$blank = 0
foreach ($line in $pre) {
    if ($line -match '^\s*#\s*pragma\b') { continue }
    if ($line.Trim() -eq '') {
        $blank++
        if ($blank -gt 1) { continue }
    } else {
        $blank = 0
    }
    $lines.Add($line.TrimEnd())
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
[System.IO.File]::WriteAllText($out, (($lines -join "`r`n") + "`r`n"), $utf8NoBom)
Write-Host "Wrote $out ($($lines.Count) lines)"
