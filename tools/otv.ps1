# OTV shortcut commands. Loaded by the PowerShell profile, so these work in
# any PowerShell window (including the VS Code terminal):
#
#   otvhelp                 list the commands
#   otvconnect              find the plugged-in board (Tank Mega or robot R4) and its COM port
#   otvrun                  compile + upload the full mission to whatever is plugged in
#   otvrun robot -Test 1    robot bench test (TEST_MODE 1-7, see docs/bench-tests.md)
#   otvbuild [tank|robot]   compile only (no board needed)
#   otvvision               open the vision system page (room 1120)
#   otvmonitor              Serial monitor (robot only; SIM/Tank builds print nothing)
#   otvsim                  regenerate sim/otv_sim/otv_sim.ino for the browser simulator
#   otvpull                 pull teammate sketches from Google Drive
#
# Install once:  powershell -ExecutionPolicy Bypass -File tools/install_otv_commands.ps1

$script:OtvRoot = Split-Path -Parent $PSScriptRoot

$script:OtvTargets = @{
    tank  = @{ Fqbn = 'arduino:avr:mega';              Sim = 1; Name = 'Physical Tank (Mega, SIM=1, marker 54)' }
    robot = @{ Fqbn = 'arduino:renesas_uno:unor4wifi'; Sim = 0; Name = 'OTV robot (UNO R4 WiFi, SIM=0, marker 80)' }
}

$script:OtvVisionUrl = 'http://lx-enes100-4.eng.umd.edu:8080/'

function Write-Otv([string]$msg, [string]$color = 'Cyan') { Write-Host $msg -ForegroundColor $color }

# Returns @{ Port; Target } for the connected board, or $null.
function Find-OtvBoard([string]$Target) {
    $json = arduino-cli board list --format json | Out-String | ConvertFrom-Json
    $ports = @($json.detected_ports | Where-Object { $_.port.protocol -eq 'serial' })
    if ($ports.Count -eq 0) { return $null }

    $found = @()
    foreach ($p in $ports) {
        $fqbn = $null
        if ($p.matching_boards) { $fqbn = $p.matching_boards[0].fqbn }
        $t = $null
        foreach ($k in $script:OtvTargets.Keys) { if ($script:OtvTargets[$k].Fqbn -eq $fqbn) { $t = $k } }
        $found += @{ Port = $p.port.address; Target = $t; Label = $p.port.label; Fqbn = $fqbn }
    }

    # Prefer a board that matches the requested target, then any recognized board.
    $pick = $null
    if ($Target) { $pick = $found | Where-Object { $_.Target -eq $Target } | Select-Object -First 1 }
    if (-not $pick) { $pick = $found | Where-Object { $_.Target } | Select-Object -First 1 }
    if (-not $pick -and $found.Count -eq 1) { $pick = $found[0] }   # unrecognized clone board
    if (-not $pick) {
        Write-Otv "Several serial ports and none recognized:" Yellow
        $found | ForEach-Object { Write-Host "  $($_.Port)  $($_.Label)" }
        $port = Read-Host 'Which COM port?'
        $pick = @{ Port = $port; Target = $null }
    }
    return $pick
}

function Resolve-OtvTarget([string]$Target, $board) {
    if ($Target) { return $Target }
    if ($board -and $board.Target) { return $board.Target }
    Write-Otv "Can't tell what board this is ($($board.Port))." Yellow
    $ans = Read-Host 'Type tank or robot'
    return $ans.Trim().ToLower()
}

function otvhelp {
    Write-Otv "OTV commands (Team H2Woah!)"
    Write-Host @"
  otvconnect              find the plugged-in board and its COM port
  otvrun                  compile + upload full mission to whatever is plugged in
  otvrun tank             force the Tank build (SIM=1, Mega)
  otvrun robot            force the robot build (SIM=0, R4)
  otvrun robot -Test N    robot bench test: 1 drive, 2 pump, 3 arm, 4 ultrasonic,
                          5 color, 6 vision print, 7 IR
  otvbuild [tank|robot]   compile only, no board needed
  otvvision               open the vision system page (room 1120)
  otvmonitor              Serial monitor at 115200 (robot only)
  otvsim                  regenerate the browser simulator file
  otvpull                 pull teammate sketches from Google Drive

TANK: upload with Tank power OFF, unplug USB, THEN power on (power-on starts the run).
"@
}

function otvconnect {
    param([ValidateSet('tank', 'robot', '')][string]$Target = '')
    Write-Otv 'Looking for boards...'
    $b = Find-OtvBoard $Target
    if (-not $b) {
        Write-Otv 'No board found. Check the USB cable (and that the Tank power is OFF).' Red
        return
    }
    $t = $b.Target
    if ($t) {
        Write-Otv "Found $($script:OtvTargets[$t].Name) on $($b.Port)" Green
        Write-Host "Next: otvrun"
    } else {
        Write-Otv "Found an unrecognized board on $($b.Port) (probably a clone Mega = the Tank)." Yellow
        Write-Host "Next: otvrun tank"
    }
}

function otvbuild {
    param([ValidateSet('tank', 'robot')][string]$Target = 'robot', [int]$Test = 0)
    $cfg = $script:OtvTargets[$Target]
    if ($cfg.Sim -eq 1 -and $Test -ne 0) { Write-Otv 'Bench tests (-Test) are robot only.' Red; return }
    Write-Otv "Compiling for $($cfg.Name), TEST_MODE=$Test"
    $flags = "compiler.cpp.extra_flags=-DSIM=$($cfg.Sim) -DTEST_MODE=$Test"
    arduino-cli compile --fqbn $cfg.Fqbn --build-property $flags (Join-Path $script:OtvRoot 'otv') | Out-Host
    if ($LASTEXITCODE -ne 0) { Write-Otv 'COMPILE FAILED' Red; return }
    Write-Otv 'Compile OK' Green

}

function otvrun {
    param(
        [ValidateSet('tank', 'robot', '')][string]$Target = '',
        [int]$Test = 0,
        [string]$Port = ''
    )
    $b = $null
    if (-not $Port) {
        $b = Find-OtvBoard $Target
        if (-not $b) { Write-Otv 'No board found. Plug in USB (Tank power OFF) and try again.' Red; return }
        $Port = $b.Port
    }
    $Target = Resolve-OtvTarget $Target $b
    if (-not $script:OtvTargets.ContainsKey($Target)) { Write-Otv "Unknown target '$Target'." Red; return }
    $cfg = $script:OtvTargets[$Target]
    if ($cfg.Sim -eq 1 -and $Test -ne 0) { Write-Otv 'Bench tests (-Test) are robot only.' Red; return }

    if ($Target -eq 'tank') {
        Write-Otv '!! Make sure the Tank power switch is OFF while USB is plugged in.' Yellow
    }
    Write-Otv "Uploading to $($cfg.Name) on $Port, TEST_MODE=$Test"
    $flags = "compiler.cpp.extra_flags=-DSIM=$($cfg.Sim) -DTEST_MODE=$Test"
    arduino-cli compile --upload -p $Port --fqbn $cfg.Fqbn --build-property $flags (Join-Path $script:OtvRoot 'otv') | Out-Host
    if ($LASTEXITCODE -ne 0) { Write-Otv 'UPLOAD FAILED (wrong port? board busy? close any Serial monitor)' Red; return }

    Write-Otv 'Upload OK' Green
    if ($Target -eq 'tank') {
        Write-Host @"

Next:
  1. Unplug USB.
  2. Put the Tank in its start square.
  3. Power on (the run starts immediately).
  4. Watch the vision page (otvvision), marker 54. No Serial output on SIM builds.
"@
    } elseif ($Test -ne 0) {
        Write-Host "`nBench test loaded. Run 'otvmonitor' to see output (docs/bench-tests.md)."
    } else {
        Write-Host "`nFull mission loaded. Watch the vision page (otvvision), marker 80, or run 'otvmonitor' while on USB."
    }
}

function otvvision {
    Write-Otv "Opening vision system (room 1120): $script:OtvVisionUrl"
    Start-Process $script:OtvVisionUrl
}

function otvmonitor {
    param([string]$Port = '')
    if (-not $Port) {
        $b = Find-OtvBoard 'robot'
        if (-not $b) { Write-Otv 'No board found.' Red; return }
        $Port = $b.Port
    }
    Write-Otv "Serial monitor on $Port at 115200. Ctrl+C to quit."
    arduino-cli monitor -p $Port -c baudrate=115200
}

function otvsim {
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\make_sim.ps1')
}

function otvpull {
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\pull_teammates.ps1')
    git -C $script:OtvRoot status --short teammates
}
