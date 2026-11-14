# OTV shortcut commands. Loaded by the PowerShell profile, so these work in
# any PowerShell window (including the VS Code terminal):
#
#   otv                     menu: pick a task by key (also the OTV desktop shortcut)
#   otvhelp                 list the commands
#   otvconnect              find the plugged-in board (Tank Mega or robot R4) and its COM port
#   otvrun                  compile + upload the full mission to whatever is plugged in
#   otvrun robot -Test 1    robot bench test (TEST_MODE 1-7, see docs/bench-tests.md)
#   otvbuild [tank|robot]   compile only (no board needed)
#   otvvision               open the vision system page (room 1120)
#   otvmonitor              Serial monitor (robot only; SIM/Tank builds print nothing)
#   otvsim                  regenerate sim/otv_sim/otv_sim.ino for the browser simulator
#   otvpull                 pull teammate sketches from Google Drive
#   otv2 .. otv10           MS5 demos (otv7a / otv7b for sub-task 7), see otvhelp
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
  otv                     menu: pick any task below by key
  otvconnect              find the plugged-in board and its COM port
  otvrun                  compile + upload full mission to whatever is plugged in
  otvrun tank             force the Tank build (SIM=1, Mega)
  otvrun robot            force the robot build (SIM=0, R4)
  otvrun robot -Test N    robot bench test: 1 drive, 2 pump, 3 arm, 4 ultrasonic,
                          5 color, 6 vision print, 7 front ultrasonic
  otvbuild [tank|robot]   compile only, no board needed
  otvvision               open the vision system page (room 1120)
  otvmonitor              Serial monitor at 115200 (robot only)
  otvsim                  regenerate the browser simulator file
  otvpull                 pull teammate sketches from Google Drive

MS5 demos (numbers = MS5 sub-tasks):
  otv2 [-Seconds 60]      robot: drive straight, open loop (overshoots 3.4 m)
  otv3 [-TurnMs 1900]     robot: three timed 90 deg turns
  otv4                    sim: wireless receive (prints x, y, theta)
  otv5                    sim: wireless transmit (mission messages)
  otv6                    sim: start -> mission site -> before the limbo
  otv7a                   sim: obstacle avoidance, wall hug (full route)
  otv7b                   sim: obstacle avoidance, encounter + detour
  otv8                    robot: front ultrasonic readout (obstacle sensing)
  otv9                    robot: arm ultrasonic readout (mission sensing)
  otv10                   robot: arm down, pump, arm stow (mission actuation)
  Sim commands copy the code to the clipboard and open the simulator.

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

# ---------------- MS5 demos (Milestone 5: Systems Performance) ----------------
# Numbers match the MS5 sub-tasks. Vision tasks run in the browser simulator
# (no vision system in the room for MS5); hardware tasks upload to the robot.

$script:OtvSimUrl = 'https://umdenes100.github.io/enes100-simulator/'

# Builds a single-file sim sketch for MS5_TASK=$Task, copies it to the
# clipboard and opens the simulator.
function Invoke-OtvSimTask([int]$Task, [string]$Name, [string]$Title, [string]$Setup) {
    $out = Join-Path $script:OtvRoot "sim\ms5\$Name.ino"
    Write-Otv "MS5 $Title (simulator)"
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\make_sim.ps1') -Task $Task -Out $out
    if ($LASTEXITCODE -ne 0) { Write-Otv 'SIM BUILD FAILED' Red; return }
    Get-Content -Raw $out | Set-Clipboard
    Start-Process $script:OtvSimUrl
    Write-Otv 'Code copied to the clipboard. Simulator opened.' Green
    Write-Host @"

In the simulator:
  1. Click in the code editor, Ctrl+A, Ctrl+V.
  2. Speed: hover the 2nd button from the left (next to Run). Its tooltip
     must say "instant code execution speed". If it says normal or slow,
     click it until it says instant (the browser remembers this).
  3. $Setup
  4. Click Run (top-left triangle). The program starts PAUSED on line 1.
  5. Click Continue (the next button, triangle with a bar) to let it run.
  6. Keep the browser window in front while it runs (Chrome pauses the
     physics in hidden or covered windows).
  7. Watch the console under the arena for the printed messages.
(File: $out)
"@
}

# Compiles + uploads a robot demo. $Flags are extra -D flags.
function Invoke-OtvRobotTask([string]$Title, [string]$Flags, [string]$Next, [switch]$Monitor) {
    $b = Find-OtvBoard 'robot'
    if (-not $b) { Write-Otv 'No board found. Plug in the OTV (R4) over USB.' Red; return }
    if ($b.Target -and $b.Target -ne 'robot') { Write-Otv "That's the Tank ($($b.Port)), not the OTV R4." Red; return }
    $cfg = $script:OtvTargets['robot']
    Write-Otv "MS5 $Title (robot, $($b.Port))"
    arduino-cli compile --upload -p $b.Port --fqbn $cfg.Fqbn --build-property "compiler.cpp.extra_flags=-DSIM=0 $Flags" (Join-Path $script:OtvRoot 'otv') | Out-Host
    if ($LASTEXITCODE -ne 0) { Write-Otv 'UPLOAD FAILED (close any Serial monitor and retry)' Red; return }
    Write-Otv 'Upload OK' Green
    Write-Host "`n$Next"
    if ($Monitor) { otvmonitor -Port $b.Port }
}

function otv2 {
    param([int]$Seconds = 60)
    Invoke-OtvRobotTask "2 forward locomotion ($Seconds s)" "-DMS5_TASK=2 -DMS5_FORWARD_MS=$($Seconds * 1000)" @"
Next:
  1. Unplug USB. Set the OTV in the landing zone facing +x (toward the limbo).
  2. Flip the kill switch and step back. It starts 5 s later.
  3. It drives straight for $Seconds s (about $([math]::Round($Seconds * 0.075, 1)) m, more than 3.4 m). Catch it.
"@
}

function otv3 {
    param([int]$TurnMs = 1900)
    Invoke-OtvRobotTask "3 turning (3 x 90 deg, $TurnMs ms each)" "-DMS5_TASK=3 -DMS5_TURN90_MS=$TurnMs" @"
Next:
  1. Unplug USB. Set the OTV on the arena floor.
  2. Flip the kill switch and step back. It starts 5 s later.
  3. Three counter-clockwise turns, 2 s apart.
  Short of 90 deg: otv3 -TurnMs 2100. Past 90 deg: otv3 -TurnMs 1700.
  Then put the final value in config.h (MS5_TURN90_MS).
"@
}

function otv4  { Invoke-OtvSimTask 4  'task4_receive'  '4 wireless receive'  'Any start position. It prints x, y, theta 20 times (spinning for the last 10).' }
function otv5  { Invoke-OtvSimTask 5  'task5_transmit' '5 wireless transmit' 'Any start position. It sends DEPTH and WATER_TYPE once each.' }
function otv6  { Invoke-OtvSimTask 6  'task6_nav'      '6 navigation, no obstacles' 'Click Randomize, THEN Remove Obstacles (Randomize puts them back). It goes to the mission site, then stops just before the limbo.' }
function otv7a { Invoke-OtvSimTask 71 'task7a_wallhug' '7a obstacle avoidance (wall hug)' 'Click Randomize (random start + obstacles). Full route: mission site, wall hug past the obstacles, limbo, goal.' }
function otv7b { Invoke-OtvSimTask 72 'task7b_detour'  '7b obstacle avoidance (encounter + detour)' 'Click Randomize (random start + obstacles). It drives down a lane, sees an obstacle, backs up, changes lane, and stops past both columns.' }

function otv8 {
    Invoke-OtvRobotTask '8 obstacle sensing (front ultrasonic)' '-DTEST_MODE=7' @"
Front ultrasonic prints mm and obstacle/clear. Move a box in and out:
it should switch to 'obstacle' at FRONT_OBSTACLE_MM (config.h). Ctrl+C to quit.
"@ -Monitor
}

function otv9 {
    Invoke-OtvRobotTask '9 mission sensing (arm ultrasonic depth)' '-DTEST_MODE=4' @"
Arm ultrasonic prints distance in mm. Hold it over a ruler or the water at
known heights and compare. Ctrl+C to quit.
"@ -Monitor
}

function otv10 {
    Invoke-OtvRobotTask '10 mission actuation (arm + pump)' '-DMS5_TASK=10' @"
Kill switch ON (servos and pump need battery power). Press Enter in the
monitor to run: arm down, pump 2 s, arm stow. Repeatable. Ctrl+C to quit.
"@ -Monitor
}

# ---------------- otv: interactive menu ----------------
# Type 'otv' (or double-click the "OTV" desktop shortcut) for a menu.
# Keys match the commands: 2 = otv2, 7a = otv7a, r = full mission, etc.

function Read-OtvNumber([string]$Prompt, [int]$Default) {
    $ans = Read-Host "$Prompt [Enter = $Default]"
    if (-not $ans.Trim()) { return $Default }
    $n = 0
    if ([int]::TryParse($ans.Trim(), [ref]$n)) { return $n }
    Write-Otv "Not a number, using $Default." Yellow
    return $Default
}

$script:OtvMenu = @(
    @{ Section = 'MS5 ON THE ROBOT   (plug the OTV in over USB first)' }
    @{ Key = '2';  Title = 'Forward 3.4 m';         When = 'uploads, then unplug + kill switch; catch it';  Run = { otv2 -Seconds (Read-OtvNumber 'Seconds to drive' 60) } }
    @{ Key = '3';  Title = '3 x 90 deg turns';      When = 'uploads, then unplug + kill switch';            Run = { otv3 -TurnMs (Read-OtvNumber 'ms per 90 deg turn' 1900) } }
    @{ Key = '8';  Title = 'Obstacle sensor';       When = 'stays on USB; live front ultrasonic readout';   Run = { otv8 } }
    @{ Key = '9';  Title = 'Mission (depth) sensor';When = 'stays on USB; live arm ultrasonic readout';     Run = { otv9 } }
    @{ Key = '10'; Title = 'Arm + pump';            When = 'USB + kill switch ON; Enter runs it';           Run = { otv10 } }
    @{ Section = 'MS5 IN THE SIMULATOR   (opens the browser; speed = instant, Ctrl+A, Ctrl+V, Run, Continue)' }
    @{ Key = '4';  Title = 'Wireless receive';      When = 'prints x, y, theta';                            Run = { otv4 } }
    @{ Key = '5';  Title = 'Wireless transmit';     When = 'sends depth + water type';                      Run = { otv5 } }
    @{ Key = '6';  Title = 'Navigation, no obstacles'; When = 'start -> mission site -> before the limbo';  Run = { otv6 } }
    @{ Key = '7a'; Title = 'Obstacles: wall hug';   When = 'our real route, start to goal';                 Run = { otv7a } }
    @{ Key = '7b'; Title = 'Obstacles: detour';     When = 'sees an obstacle, backs up, changes lane';      Run = { otv7b } }
    @{ Section = 'FULL MISSION + BENCH TESTS' }
    @{ Key = 'r';  Title = 'Full mission (robot)';  When = 'needs the vision system in the room';           Run = { otvrun robot } }
    @{ Key = 't';  Title = 'Full mission (Tank)';   When = 'Tank power OFF while on USB';                   Run = { otvrun tank } }
    @{ Key = 'b';  Title = 'Bench test';            When = '1 drive 2 pump 3 arm 4 depth 5 color 6 vision 7 front'; Run = {
        $n = Read-OtvNumber 'Bench test number (1-7)' 1
        if ($n -lt 1 -or $n -gt 7) { Write-Otv 'Bench tests are 1-7.' Red; return }
        otvrun robot -Test $n
        otvmonitor } }
    @{ Section = 'TOOLS' }
    @{ Key = 'm';  Title = 'Serial monitor';        When = 'robot on USB';                                  Run = { otvmonitor } }
    @{ Key = 'c';  Title = 'Find the board';        When = 'which board + COM port is plugged in';          Run = { otvconnect } }
    @{ Key = 'v';  Title = 'Vision system page';    When = 'room 1120';                                     Run = { otvvision } }
    @{ Key = 's';  Title = 'Open the simulator';    When = 'full mission sim file, copied to clipboard';    Run = {
        otvsim
        Get-Content -Raw (Join-Path $script:OtvRoot 'sim\otv_sim\otv_sim.ino') | Set-Clipboard
        Start-Process $script:OtvSimUrl
        Write-Otv 'Full mission copied to the clipboard. In the simulator: speed button = instant, Ctrl+A, Ctrl+V, Run, then Continue (Run starts paused).' Green } }
    @{ Key = 'p';  Title = 'Pull teammate code';    When = 'from Google Drive';                             Run = { otvpull } }
    @{ Key = 'h';  Title = 'All typed commands';    When = 'otvhelp';                                       Run = { otvhelp } }
)

function Show-OtvMenu {
    Clear-Host
    Write-Otv '  OTV  -  Team H2Woah!'
    foreach ($item in $script:OtvMenu) {
        if ($item.Section) { Write-Host ''; Write-Host "  $($item.Section)" -ForegroundColor Yellow; continue }
        Write-Host ('   {0,-3}' -f $item.Key) -ForegroundColor Green -NoNewline
        Write-Host (' {0,-27}' -f $item.Title) -NoNewline
        Write-Host $item.When -ForegroundColor DarkGray
    }
    Write-Host ''
    Write-Host '   q   quit' -ForegroundColor DarkGray
    Write-Host '  (Ctrl+C in a monitor drops you to the prompt; type otv to come back.)' -ForegroundColor DarkGray
    Write-Host ''
}

function otv {
    while ($true) {
        Show-OtvMenu
        $key = (Read-Host '  Pick').Trim().ToLower()
        if ($key -eq 'q') { return }
        if (-not $key) { continue }
        $item = $script:OtvMenu | Where-Object { $_.Key -eq $key } | Select-Object -First 1
        if (-not $item) { continue }
        Write-Host ''
        & $item.Run
        Write-Host ''
        $again = Read-Host 'Enter = back to the menu, q = quit'
        if ($again.Trim().ToLower() -eq 'q') { return }
    }
}
