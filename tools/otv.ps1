# OTV shortcut commands. Loaded by the PowerShell profile, so these work in
# any PowerShell window (including the VS Code terminal):
#
#   otv                     menu: pick a task by key (also the OTV desktop shortcut)
#   otvhelp                 list the commands
#   otvconnect              find the plugged-in board (Tank Mega or robot R4) and its COM port
#   otvrun                  compile + upload the full mission to whatever is plugged in
#   otvrun robot -Test 1    robot bench test (TEST_MODE 1-8, see docs/bench-tests.md)
#   otvarm                  live arm angle + depth calibration (bench test 8, type commands)
#   otvbuild [tank|robot]   compile only (no board needed)
#   otvvision               open the vision system page (room 1120)
#   otvmonitor              Serial monitor (robot only; SIM/Tank builds print nothing)
#   otvsim                  regenerate sim/otv_sim/otv_sim.ino for the browser simulator
#   otvsimcheck [-Runs 5]   run every sim demo N times and check each finishes inside the course
#   otvpull                 pull teammate sketches from Google Drive
#   otv2 .. otv10           MS5 demos (otv7a / otv7b for sub-task 7), see otvhelp
#   otv2sim otv3sim otv8sim otv10sim   sim dry runs of the robot demos
#   otv4tank otv5tank       MS5 4/5 on the physical Tank + real vision system, numbers in this terminal
#   otvwatch                live pose + Arduino prints from the vision system, in this terminal
#   otvsite otvmission otvobst   MS6 demos on the robot (+ ...sim dry runs), see otvhelp
#
# Install once:  powershell -ExecutionPolicy Bypass -File tools/install_otv_commands.ps1

$script:OtvRoot = Split-Path -Parent $PSScriptRoot

$script:OtvTargets = @{
    tank  = @{ Fqbn = 'arduino:avr:mega';              Sim = 1; Name = 'Physical Tank (Mega, SIM=1, marker 54)' }
    robot = @{ Fqbn = 'arduino:renesas_uno:unor4wifi'; Sim = 0; Name = 'OTV robot (UNO R4 WiFi, SIM=0, marker 72)' }
}

$script:OtvVisionUrl = 'http://lx-enes100-4.eng.umd.edu:8080/'

# A number from otv/config.h (e.g. ARM_DOWN_DEG), so help text shows the calibrated value.
function Get-OtvConfigValue([string]$Name) {
    $m = Select-String -Path (Join-Path $script:OtvRoot 'otv\config.h') -Pattern "\b$Name\s*=\s*(-?[0-9.]+)\s*;" | Select-Object -First 1
    if ($m) { return $m.Matches[0].Groups[1].Value }
    return '?'
}

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
                          5 color, 6 vision print, 7 front ultrasonic, 8 arm cal
  otvarm                  robot: live arm + depth calibration. down/stow, or an angle (150), +/-,
                          d/s to keep down/stow, e (empty bowl) or k30 (30 mm water)
                          for SENSOR_HEIGHT_MM, p pump, ? for the config.h lines
  otvbuild [tank|robot]   compile only, no board needed
  otvvision               open the vision system page (room 1120)
  otvmonitor              Serial monitor at 115200 (robot only)
  otvsim                  regenerate the browser simulator file
  otvsimcheck [-Runs 5]   run every sim demo (4-7b, the dry runs, full mission) N times and
                          check each finishes, the Tank (as drawn) never leaves the arena,
                          never touches a solid obstacle, and passes the limbo opening.
                          -Only otv7a,otv7b runs just those (~1 min per moving run)
  otvpull                 pull teammate sketches from Google Drive

MS6 demos (any 4 of 6 objectives, one at a time, on the robot; vision system on):
  otvvisioncheck          OTV on USB in the arena: prints x/y/theta for marker 72 (do this first)
  otvsite                 Nav 1 + Mission I-III: start square -> pool -> measure, pump, send, stop
  otvmission              Mission I-III only: OTV placed at the pool by hand
  otvobst                 Nav 2 + Nav 3: landing zone -> wall hug past the obstacles -> limbo -> goal
  otvsitesim otvmissionsim otvobstsim   the same three as simulator dry runs

MS5 demos (numbers = MS5 sub-tasks):
  otv2 [-Seconds 45]      robot: drive straight ~3.4 m, open loop (time = distance / DRIVE_SPEED_MPS)
  otv3 [-TurnMs 950]      robot: three timed 90 deg turns
  otv4                    sim: wireless receive (prints x, y, theta)
  otv5                    sim: wireless transmit (mission messages)
  otv4tank [-Marker 54]   TANK + vision system: prints received x, y, theta here
  otv5tank [-Marker 54]   TANK + vision system: sends depth + water type, prints what it sent
                          (upload on USB with Tank power OFF, unplug, power on; output prints here)
  otvwatch [-Team name]   live pose + Arduino prints for a team from the vision system, in this window
  otv6                    sim: start -> mission site -> before the limbo
  otv7a                   sim: obstacle avoidance, wall hug (full route)
  otv7b                   sim: obstacle avoidance, encounter + detour
  otv8                    robot: front ultrasonic readout (obstacle sensing)
  otv9                    robot: arm ultrasonic readout (mission sensing)
  otv10                   robot: arm down, pump, arm stow (mission actuation)
  otv2sim otv3sim         sim dry runs of robot demos 2, 3, 10 (same code, sim-speed times)
  otv10sim otv8sim        otv8sim: the sim's front distance sensor vs vision (no otv9sim: no water in the sim)
  Sim commands run by themselves in their own Chrome window and print progress here.

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
        Write-Host "`nFull mission loaded. Watch the vision page (otvvision), marker 72, or run 'otvmonitor' while on USB."
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

# Runs every moving sim demo $Runs times with the footprint checker
# (tools/sim_autorun.js --check) and prints a PASS/FAIL table.
# Every simulator demo. otvsimcheck runs these with the footprint checker.
$script:OtvSimCases = @(
    @{ Name = 'otv4';     Task = 4;  File = 'sim\ms5\task4_receive.ino';      Buttons = 'Randomize' }
    @{ Name = 'otv5';     Task = 5;  File = 'sim\ms5\task5_transmit.ino';     Buttons = 'Randomize' }
    @{ Name = 'otv6';     Task = 6;  File = 'sim\ms5\task6_nav.ino';          Buttons = 'Randomize,Remove Obstacles' }
    @{ Name = 'otv7a';    Task = 71; File = 'sim\ms5\task7a_wallhug.ino';     Buttons = 'Randomize' }
    @{ Name = 'otv7b';    Task = 72; File = 'sim\ms5\task7b_detour.ino';      Buttons = 'Randomize' }
    @{ Name = 'otv2sim';  Task = 2;  File = 'sim\ms5\task2_forward_sim.ino';  Buttons = 'Randomize,Remove Obstacles' }
    @{ Name = 'otv3sim';  Task = 3;  File = 'sim\ms5\task3_turns_sim.ino';    Buttons = 'Randomize,Remove Obstacles' }
    @{ Name = 'otv8sim';  Task = 8;  File = 'sim\ms5\task8_sensor_sim.ino';   Buttons = 'Randomize,Remove Obstacles' }
    @{ Name = 'otv10sim'; Task = 10; File = 'sim\ms5\task10_actuate_sim.ino'; Buttons = 'Randomize' }
    @{ Name = 'full';     Task = 0;  File = 'sim\otv_sim\otv_sim.ino';        Buttons = 'Randomize' }
    @{ Name = 'site';     Ms6 = 1;   File = 'sim\ms6\site_sim.ino';           Buttons = 'Randomize' }
    @{ Name = 'mission';  Ms6 = 2;   File = 'sim\ms6\mission_sim.ino';        Buttons = 'Randomize' }
    @{ Name = 'obst';     Ms6 = 3;   File = 'sim\ms6\obstacles_sim.ino';      Buttons = 'Randomize' }
)

function otvsimcheck {
    # -Only otv7a,otv7b runs just those.
    param([int]$Runs = 5, [string[]]$Only)
    if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Otv 'Node.js not found.' Red; return }
    $mk = Join-Path $script:OtvRoot 'tools\make_sim.ps1'
    powershell -ExecutionPolicy Bypass -File $mk | Out-Null
    $cases = $script:OtvSimCases
    if ($Only) { $cases = @($cases | Where-Object { $Only -contains $_.Name }) }
    foreach ($c in $cases) {
        if ($c.Task) { powershell -ExecutionPolicy Bypass -File $mk -Task $c.Task -Out (Join-Path $script:OtvRoot $c.File) | Out-Null }
        if ($c.Ms6) { powershell -ExecutionPolicy Bypass -File $mk -Ms6Task $c.Ms6 -Out (Join-Path $script:OtvRoot $c.File) | Out-Null }
    }
    $js = Join-Path $script:OtvRoot 'tools\sim_autorun.js'
    $tmp = Join-Path $env:TEMP 'otv_simcheck.txt'
    $results = @()
    Write-Otv "Sim check: $Runs run(s) each of $(($cases | ForEach-Object { $_.Name }) -join ', ') (random starts)"
    foreach ($c in $cases) {
        for ($i = 1; $i -le $Runs; $i++) {
            # Output goes to a file, not a pipe: a Chrome window started by
            # node would otherwise hold the pipe open.
            $file = Join-Path $script:OtvRoot $c.File
            $argStr = "--experimental-websocket --no-warnings `"$js`" `"$file`" `"$($c.Buttons)`" 240 --check --quiet"
            $p = Start-Process node -ArgumentList $argStr -RedirectStandardOutput $tmp -NoNewWindow -PassThru
            $p.WaitForExit()
            $out = Get-Content $tmp
            $check = $out | Where-Object { $_ -like 'CHECK *' } | Select-Object -First 1
            $end = $out | Where-Object { $_ -like 'END *' } | Select-Object -First 1
            $why = $out | Where-Object { $_ -like '  *' -or $_ -like 'SIM AUTORUN FAILED*' }
            $ok = $check -like 'CHECK PASS*'
            Write-Host ("  {0,-8} run {1}: {2}" -f $c.Name, $i, $(if ($ok) { 'PASS' } else { 'FAIL' })) -ForegroundColor $(if ($ok) { 'Green' } else { 'Red' })
            Write-Host "      $end"
            Write-Host "      $check"
            foreach ($w in $why) { Write-Host "      $w" -ForegroundColor Red }
            $results += [pscustomobject]@{ Demo = $c.Name; Pass = $ok }
        }
    }
    $fails = @($results | Where-Object { -not $_.Pass }).Count
    Write-Host ''
    $results | Group-Object Demo | ForEach-Object {
        Write-Host ("  {0,-8} {1}/{2} passed" -f $_.Name, @($_.Group | Where-Object Pass).Count, $_.Count)
    }
    if ($fails) { Write-Otv "$fails run(s) FAILED (details above)" Red } else { Write-Otv 'All runs finished and stayed inside the course.' Green }
}

function otvpull {
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\pull_teammates.ps1')
    git -C $script:OtvRoot status --short teammates
}

# ---------------- MS5 demos (Milestone 5: Systems Performance) ----------------
# Numbers match the MS5 sub-tasks. Vision tasks run in the browser simulator
# (no vision system in the room for MS5); hardware tasks upload to the robot.

$script:OtvSimUrl = 'https://umdenes100.github.io/enes100-simulator/'

# Runs a sim sketch in the browser simulator with no clicks: tools/sim_autorun.js
# opens its own Chrome window, loads the code, sets speed to instant, clicks
# $Buttons, Run and Continue, then prints the pose + sim console here.
# Falls back to the clipboard + manual steps if Node is missing or it fails.
function Start-OtvSimAuto([string]$File, [string]$Buttons) {
    $node = Get-Command node -ErrorAction SilentlyContinue
    if ($node) {
        $js = Join-Path $script:OtvRoot 'tools\sim_autorun.js'
        & node --experimental-websocket --no-warnings $js $File $Buttons 180
        if ($LASTEXITCODE -eq 0) { return }
        Write-Otv 'Automatic run failed (message above). Falling back to manual steps.' Yellow
    } else {
        Write-Otv 'Node.js not found, so the simulator cannot be driven automatically.' Yellow
    }
    Get-Content -Raw $File | Set-Clipboard
    Start-Process $script:OtvSimUrl
    Write-Host @"

Code copied to the clipboard. In the simulator:
  1. 2nd button from the left: click until its tooltip says "instant".
  2. Click in the code editor, Ctrl+A, Ctrl+V.
  3. Click: $($Buttons -replace ',', ', then ')
  4. Click Run, then Continue (Run starts paused).
"@
}

# Builds a single-file sim sketch for MS5_TASK=$Task and runs it.
function Invoke-OtvSimTask([int]$Task, [string]$Name, [string]$Title, [string]$Buttons, [string]$Note) {
    $out = Join-Path $script:OtvRoot "sim\ms5\$Name.ino"
    Write-Otv "MS5 $Title (simulator)"
    Write-Host $Note
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\make_sim.ps1') -Task $Task -Out $out
    if ($LASTEXITCODE -ne 0) { Write-Otv 'SIM BUILD FAILED' Red; return }
    Start-OtvSimAuto $out $Buttons
}

# Compiles + uploads a robot demo. $Flags are extra -D flags.
function Invoke-OtvRobotTask([string]$Title, [string]$Flags, [string]$Next, [switch]$Monitor, [switch]$Watch, [string]$Label = 'MS5') {
    $b = Find-OtvBoard 'robot'
    if (-not $b) { Write-Otv 'No board found. Plug in the OTV (R4) over USB.' Red; return }
    if ($b.Target -and $b.Target -ne 'robot') { Write-Otv "That's the Tank ($($b.Port)), not the OTV R4." Red; return }
    $cfg = $script:OtvTargets['robot']
    Write-Otv "$Label $Title (robot, $($b.Port))"
    arduino-cli compile --upload -p $b.Port --fqbn $cfg.Fqbn --build-property "compiler.cpp.extra_flags=-DSIM=0 $Flags" (Join-Path $script:OtvRoot 'otv') | Out-Host
    if ($LASTEXITCODE -ne 0) { Write-Otv 'UPLOAD FAILED (close any Serial monitor and retry)' Red; return }
    Write-Otv 'Upload OK' Green
    Write-Host "`n$Next"
    if ($Monitor) { otvmonitor -Port $b.Port }
    if ($Watch) {
        Read-Host "`nPress Enter to watch the run here (live pose + OTV prints; Ctrl+C to stop)" | Out-Null
        otvwatch
    }
}

function otv2 {
    param([int]$Seconds = 45)
    Invoke-OtvRobotTask "2 forward locomotion ($Seconds s)" "-DMS5_TASK=2 -DMS5_FORWARD_MS=$($Seconds * 1000)" @"
Next:
  1. Unplug USB. Set the OTV in the landing zone facing +x (toward the limbo).
  2. Flip the kill switch and step back. It starts 5 s later.
  3. It drives straight for $Seconds s (about $([math]::Round($Seconds * 0.075, 1)) m, more than 3.4 m). Catch it.
"@
}

function otv3 {
    param([int]$TurnMs = 950)
    Invoke-OtvRobotTask "3 turning (3 x 90 deg, $TurnMs ms each)" "-DMS5_TASK=3 -DMS5_TURN90_MS=$TurnMs" @"
Next:
  1. Unplug USB. Set the OTV on the arena floor.
  2. Flip the kill switch and step back. It starts 5 s later.
  3. Three counter-clockwise turns, 2 s apart.
  Short of 90 deg: otv3 -TurnMs 1050. Past 90 deg: otv3 -TurnMs 850.
  Then put the final value in config.h (MS5_TURN90_MS).
"@
}

function otv4  { Invoke-OtvSimTask 4  'task4_receive'  '4 wireless receive'  'Randomize' 'Prints x, y, theta 20 times (spinning for the last 10).' }
function otv5  { Invoke-OtvSimTask 5  'task5_transmit' '5 wireless transmit' 'Randomize' 'Sends DEPTH and WATER_TYPE once each.' }
function otv6  { Invoke-OtvSimTask 6  'task6_nav'      '6 navigation, no obstacles' 'Randomize,Remove Obstacles' 'Random start, no obstacles: mission site, then stops just before the limbo.' }
function otv7a { Invoke-OtvSimTask 71 'task7a_wallhug' '7a obstacle avoidance (wall hug)' 'Randomize' 'Random start + obstacles. Full route: mission site, wall hug past the obstacles, limbo, goal.' }
function otv7b { Invoke-OtvSimTask 72 'task7b_detour'  '7b obstacle avoidance (encounter + detour)' 'Randomize' 'Random obstacles. Drives down a lane, sees an obstacle, backs up, changes lane, stops past both columns.' }

# Simulator dry runs of the robot demos: same code as otv2/otv3/otv10, so a
# logic mistake shows up here before demo day instead of on the arena floor.
# (The sim Tank is 3.5x faster, so the times are scaled to it; the robot
# numbers still have to be tuned on the robot.) otv9 has no sim version:
# the sim has no water, so its depth reading is a fixed stand-in.
function otv2sim  { Invoke-OtvSimTask 2  'task2_forward_sim'  '2 forward locomotion (sim dry run)' 'Randomize,Remove Obstacles' 'Faces +x, then the open-loop forward drive; stops before the limbo and prints the distance.' }
function otv3sim  { Invoke-OtvSimTask 3  'task3_turns_sim'    '3 three 90 deg turns (sim dry run)' 'Randomize,Remove Obstacles' 'Three timed turns in place; prints how many degrees vision saw for each.' }
function otv8sim  { Invoke-OtvSimTask 8  'task8_sensor_sim'   '8 obstacle sensor (sim)' 'Randomize,Remove Obstacles' 'Creeps at the west wall printing the distance sensor next to the true distance from vision; stops at the threshold.' }
function otv10sim { Invoke-OtvSimTask 10 'task10_actuate_sim' '10 arm + pump (sim dry run)' 'Randomize' 'Runs arm down -> pump -> arm stow once (the sim has no servos: sequence and messages only).' }

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

# Bench test 8: set the arm angles and the depth sensor height live.
function otvarm {
    Invoke-OtvRobotTask 'arm + depth calibration (bench test 8)' '-DTEST_MODE=8' @"
Kill switch ON. Type a command + Enter:
  down = arm to mission height ($(Get-OtvConfigValue 'ARM_DOWN_DEG') deg)   stow = arm folded for driving ($(Get-OtvConfigValue 'ARM_STOW_DEG') deg)
  150 = move arm to 150 deg   + / - = 1 deg (+5 / -5)
  d = keep this angle as ARM_DOWN_DEG (color sensor above water, tube in it)
  s = keep this angle as ARM_STOW_DEG
  e = empty bowl under the arm -> SENSOR_HEIGHT_MM    k30 = water is 30 mm deep
  p = pump 2 s (p7 = 7 s)    ? = print the lines to paste into otv/config.h
Live line every second: arm angle, arm ultrasonic, depth, front ultrasonic.
"@ -Monitor
}

# MS5 4 + 5 on the physical Tank with the real vision system (room 1120).
# Upload with the Tank power OFF, unplug USB, power the Tank on (it doesn't
# drive in these). Its WiFi module only works on Tank power. The Tank prints
# to the vision system, and otvwatch shows that in this PowerShell window.
function Invoke-OtvTankTask([string]$Title, [int]$Task, [string]$Next, [int]$Marker = 54) {
    $b = Find-OtvBoard 'tank'
    if (-not $b) { Write-Otv 'No board found. Plug in the Tank (Mega) over USB, Tank power OFF.' Red; return }
    if ($b.Target -eq 'robot') { Write-Otv "That's the OTV R4 ($($b.Port)), not the Tank's Mega." Red; return }
    Write-Otv '!! Tank power switch must be OFF while USB is plugged in.' Yellow
    Write-Otv "MS5 $Title (physical Tank, marker $Marker, $($b.Port))"
    $cfg = $script:OtvTargets['tank']
    arduino-cli compile --upload -p $b.Port --fqbn $cfg.Fqbn --build-property "compiler.cpp.extra_flags=-DSIM=1 -DMS5_TASK=$Task -DMARKER_OVERRIDE=$Marker" (Join-Path $script:OtvRoot 'otv') | Out-Host
    if ($LASTEXITCODE -ne 0) { Write-Otv 'UPLOAD FAILED (close any Serial monitor and retry)' Red; return }
    Write-Otv 'Upload OK' Green
    Write-Host @"

Now:
  1. Unplug the USB cable from the Tank.
  2. Tank in the arena, marker $Marker facing up.
  3. Power the Tank ON. It connects to the vision system in up to 30 s.
$Next
"@
    Read-Host 'Press Enter once the Tank is unplugged and powered on' | Out-Null
    otvwatch
}

# Live team data from the vision system, printed in this window: pose
# (marker, x, y, theta) and every line the Arduino prints. Ctrl+C to quit.
function otvwatch {
    param([string]$Team = 'H2Woah!')
    if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Otv 'Node.js not found. Use otvvision (the web page) instead.' Red; return }
    & node --experimental-websocket --no-warnings (Join-Path $script:OtvRoot 'tools/vision_watch.js') --team $Team
}

function otv4tank {
    param([int]$Marker = 54)
    Invoke-OtvTankTask '4 wireless receive' 4 -Marker $Marker -Next @"
Here you'll see two kinds of lines, twice a second:
  ARDUINO Received x=... y=... theta=...   what the Tank's Arduino received
  VISION  marker $Marker x = ... y = ... theta = ...   what the vision system sent
Move or turn the Tank by hand and both follow. Ctrl+C to quit.
"@
}

function otv5tank {
    param([int]$Marker = 54)
    Invoke-OtvTankTask '5 wireless transmit' 5 -Marker $Marker -Next @"
It sends DEPTH 30 mm and WATER_TYPE unpolluted once connected, then
again every 15 s. Here you'll see 'ARDUINO Sent #n: ...' lines; the
vision computer shows the mission messages (team H2Woah!). Ctrl+C to quit.
"@
}

function otv10 {
    Invoke-OtvRobotTask '10 mission actuation (arm + pump)' '-DMS5_TASK=10' @"
Kill switch ON (servos and pump need battery power). Press Enter in the
monitor to run: arm down, pump 2 s, arm stow. Repeatable. Ctrl+C to quit.
"@ -Monitor
}

# ---------------- MS6 demos (Milestone 6: Systems Integration) ----------------
# Any 4 of 6 objectives, shown one at a time on the real OTV with unlimited
# attempts. Each demo runs part of the full mission (MS6_TASK, otv/ms6.ino)
# so a failed attempt is quick to retry. The vision system must be running.

# Vision check: the OTV stays on USB and prints what the vision system sends.
function otvvisioncheck {
    Invoke-OtvRobotTask 'vision check (bench test 6, marker 72)' '-DTEST_MODE=6' -Label 'MS6' @"
WHAT TO DO
  1. Keep USB plugged in (kill switch can stay OFF). OTV in the arena, marker 72 flat
     and facing up, nothing covering it.
  2. Wait up to 30 s. Lines like this start printing:
       x=0.52 y=1.48 theta=-1.57 visible=yes
  3. Move the OTV by hand: x grows toward the limbo, y grows toward the top wall,
     theta = 0 facing the limbo, +1.57 facing the top wall.
GOOD:  visible=yes and the numbers follow the OTV.
BAD:   visible=no -> marker covered, tilted or out of camera view.
       Zeros that never change -> not connected (room 1120? vision page up?).
SEND CLAUDE: one line with the OTV still, and whether the numbers followed it.
Ctrl+C to stop.
"@ -Monitor
}

function otvsite {
    Invoke-OtvRobotTask 'Nav 1 + mission (start square -> pool -> mission)' '-DMS6_TASK=1' -Label 'MS6' -Watch @"
OBJECTIVES: Nav 1 (within 150 mm of the pool) + mission (depth, sample).
WHAT TO DO
  1. Empty the sample bottle. Unplug USB. Put the OTV in either start square,
     marker 72 up, any heading.
  2. Kill switch ON, step back. Within 30 s it connects, drives to the other
     square's pool, stops, lowers the arm, measures depth, pumps, stows the arm,
     sends the depth, and stops for good. Last line: MS6: done
WATCH FOR (below, after you press Enter)
  MS6: marker to pool center (m) = ...    Mission: raw depth mm = ...    Mission sent: depth mm = ...
RETRY: kill switch OFF, put it back, kill switch ON (no upload needed).
SEND CLAUDE: how far the OTV's front ended from the pool edge (mm), the depth it
sent vs the real depth, mL in the bottle, and anything odd (wobble, overshoot, bumps).
"@
}

function otvmission {
    Invoke-OtvRobotTask 'mission only (OTV placed at the pool)' '-DMS6_TASK=2' -Label 'MS6' -Watch @"
OBJECTIVES: mission (depth, sample), no driving.
WHAT TO DO
  1. Empty the sample bottle. Unplug USB.
  2. Place the OTV at the pool by hand so the arm will be over the water when lowered.
  3. Kill switch ON, hands clear of the arm. Within 30 s it connects, lowers the arm
     to horizontal, measures depth, pumps the sample, stows, sends the depth.
     Last line: MS6: done
WATCH FOR (below, after you press Enter)
  Mission: raw depth mm = ...    Mission sent: depth mm = ...
RETRY: kill switch OFF, empty the bottle, kill switch ON (no upload needed).
SEND CLAUDE: the depth it sent vs the real depth, and how many mL are in the bottle.
"@
}

function otvobst {
    Invoke-OtvRobotTask 'Nav 2 + Nav 3 (landing zone -> past the obstacles -> goal)' '-DMS6_TASK=3' -Label 'MS6' -Watch @"
OBJECTIVES: Nav 2 (completely past the 3 obstacles) + Nav 3 (completely in the goal zone).
WHAT TO DO
  1. Unplug USB. Arm stowed. Put the OTV anywhere in the landing zone (x < 0.8 m),
     clear of the pools, marker 72 up, any heading.
  2. Kill switch ON, step back. It drives to x = 0.8, up toward the top wall, hugs
     the top wall past the obstacles, stops and lowers the arm, goes under the
     limbo, and stops in the goal zone. Last line: MS6: done
  Be ready to hit the kill switch if it is about to hit a wall, obstacle or the limbo.
WATCH FOR (below, after you press Enter)
  Pose at ... lines after each leg; "Obstacle ahead: backing up" if the front sensor stopped it.
RETRY: kill switch OFF, put it back, kill switch ON (no upload needed).
SEND CLAUDE: where it went wrong (which leg, what it hit or how close), or that it made it.
"@
}

# Sim dry runs of the MS6 demos: same code, browser simulator (mission is a stub).
function Invoke-OtvMs6Sim([int]$Ms6Task, [string]$Name, [string]$Title, [string]$Note) {
    $out = Join-Path $script:OtvRoot "sim\ms6\$Name.ino"
    Write-Otv "MS6 $Title (simulator dry run)"
    Write-Host $Note
    powershell -ExecutionPolicy Bypass -File (Join-Path $script:OtvRoot 'tools\make_sim.ps1') -Ms6Task $Ms6Task -Out $out
    if ($LASTEXITCODE -ne 0) { Write-Otv 'SIM BUILD FAILED' Red; return }
    Start-OtvSimAuto $out 'Randomize'
}
function otvsitesim    { Invoke-OtvMs6Sim 1 'site_sim'      'Nav 1 + mission' 'Random start: drives to the mission site, runs the (stub) mission, stops.' }
function otvmissionsim { Invoke-OtvMs6Sim 2 'mission_sim'   'mission only'    'Runs the (stub) mission where it starts and sends the results.' }
function otvobstsim    { Invoke-OtvMs6Sim 3 'obstacles_sim' 'Nav 2 + Nav 3'   'Random start + obstacles: wall hug past the obstacles, under the limbo, into the goal.' }

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

# Main menu: MS6 only. 'x' switches to everything else ($OtvMenuMore) and back.
$script:OtvMenu = @(
    @{ Section = '1. BEFORE THE RUNS   (OTV on USB)' }
    @{ Key = 'c';  Title = 'Find the OTV';            When = 'is it plugged in, which COM port';                           Run = { otvconnect } }
    @{ Key = 'v';  Title = 'Vision check';            When = 'in the arena: x/y/theta for marker 72 (do this first)';      Run = { otvvisioncheck } }
    @{ Key = 'a';  Title = 'Arm, depth + pump check'; When = 'kill switch ON: type down / stow, p10 = pump 10 s into a cup'; Run = { otvarm } }
    @{ Section = '2. MS6 RUNS   (uploads; then unplug, place the OTV, kill switch ON, watch here)' }
    @{ Key = 'n';  Title = 'Nav 1 + mission';         When = 'start square -> pool -> depth + sample';                     Run = { otvsite } }
    @{ Key = 'i';  Title = 'Mission only';            When = 'OTV placed at the pool by hand -> depth + sample';           Run = { otvmission } }
    @{ Key = 'o';  Title = 'Nav 2 + Nav 3';           When = 'landing zone -> past the obstacles -> under the limbo -> goal'; Run = { otvobst } }
    @{ Key = 'w';  Title = 'Watch the vision system'; When = 'live pose + OTV prints (again, or in a 2nd window)';         Run = { otvwatch } }
    @{ Section = '3. OTHER' }
    @{ Key = 'd';  Title = 'Dry run in the simulator'; When = 'the same n / i / o code in the browser sim, no robot';      Run = {
        $w = (Read-Host 'Which one: n (Nav 1 + mission), i (mission only), o (Nav 2 + 3)').Trim().ToLower()
        switch ($w) { 'n' { otvsitesim } 'i' { otvmissionsim } 'o' { otvobstsim } default { Write-Otv 'Pick n, i or o.' Yellow } } } }
    @{ Key = 'm';  Title = 'Serial monitor';          When = 'OTV on USB';                                                 Run = { otvmonitor } }
    @{ Key = 'x';  Title = 'More...';                 When = 'MS5 demos, Tank, bench tests, full mission, sim tools';      Run = { } }
)

$script:OtvMenuMore = @(
    @{ Section = 'FULL MISSION + BENCH TESTS' }
    @{ Key = 'r';  Title = 'Full mission (robot)';    When = 'needs the vision system in the room';           Run = { otvrun robot } }
    @{ Key = 'b';  Title = 'Bench test';              When = '1 drive 2 pump 3 arm 4 depth 5 color 6 vision 7 front 8 arm cal'; Run = {
        $n = Read-OtvNumber 'Bench test number (1-8)' 1
        if ($n -lt 1 -or $n -gt 8) { Write-Otv 'Bench tests are 1-8.' Red; return }
        otvrun robot -Test $n
        otvmonitor } }
    @{ Key = 't';  Title = 'Full mission (Tank)';     When = 'Tank power OFF while on USB';                   Run = { otvrun tank } }
    @{ Section = 'MS5 ON THE ROBOT' }
    @{ Key = '2';  Title = 'Forward 3.4 m';           When = 'uploads, then unplug + kill switch; catch it';  Run = { otv2 -Seconds (Read-OtvNumber 'Seconds to drive' 45) } }
    @{ Key = '3';  Title = '3 x 90 deg turns';        When = 'uploads, then unplug + kill switch';            Run = { otv3 -TurnMs (Read-OtvNumber 'ms per 90 deg turn' 950) } }
    @{ Key = '8';  Title = 'Obstacle sensor';         When = 'stays on USB; live front ultrasonic readout';   Run = { otv8 } }
    @{ Key = '9';  Title = 'Mission (depth) sensor';  When = 'stays on USB; live arm ultrasonic readout';     Run = { otv9 } }
    @{ Key = '10'; Title = 'Arm + pump';              When = 'USB + kill switch ON; Enter runs it';           Run = { otv10 } }
    @{ Section = 'MS5 IN THE SIMULATOR / ON THE TANK' }
    @{ Key = '4';  Title = 'Wireless receive';        When = 'sim: prints x, y, theta';                       Run = { otv4 } }
    @{ Key = '5';  Title = 'Wireless transmit';       When = 'sim: sends depth + water type';                 Run = { otv5 } }
    @{ Key = '6';  Title = 'Navigation, no obstacles'; When = 'sim: start -> mission site -> before the limbo'; Run = { otv6 } }
    @{ Key = '7a'; Title = 'Obstacles: wall hug';     When = 'sim: our real route, start to goal';            Run = { otv7a } }
    @{ Key = '7b'; Title = 'Obstacles: detour';       When = 'sim: sees an obstacle, backs up, changes lane'; Run = { otv7b } }
    @{ Key = '4t'; Title = 'Wireless receive (Tank)'; When = 'real vision system';                            Run = { otv4tank -Marker (Read-OtvNumber 'Marker number on the Tank' 54) } }
    @{ Key = '5t'; Title = 'Wireless transmit (Tank)'; When = 'real vision system';                           Run = { otv5tank -Marker (Read-OtvNumber 'Marker number on the Tank' 54) } }
    @{ Key = '2s'; Title = 'Forward (sim)';           When = 'dry run of otv2';                               Run = { otv2sim } }
    @{ Key = '3s'; Title = '3 x 90 deg turns (sim)';  When = 'dry run of otv3';                               Run = { otv3sim } }
    @{ Key = '8s'; Title = 'Obstacle sensor (sim)';   When = 'dry run of otv8';                               Run = { otv8sim } }
    @{ Key = '10s';Title = 'Arm + pump (sim)';        When = 'dry run of otv10';                              Run = { otv10sim } }
    @{ Section = 'TOOLS' }
    @{ Key = 's';  Title = 'Full mission in the sim'; When = 'runs by itself in its own Chrome window';       Run = {
        otvsim
        if ($LASTEXITCODE -ne 0) { Write-Otv 'SIM BUILD FAILED' Red; return }
        Start-OtvSimAuto (Join-Path $script:OtvRoot 'sim\otv_sim\otv_sim.ino') 'Randomize' } }
    @{ Key = 'k';  Title = 'Check sim paths';         When = 'every sim demo x5: must finish inside the course'; Run = { otvsimcheck } }
    @{ Key = 'vp'; Title = 'Vision system page';      When = 'room 1120, in the browser';                     Run = { otvvision } }
    @{ Key = 'p';  Title = 'Pull teammate code';      When = 'from Google Drive';                             Run = { otvpull } }
    @{ Key = 'h';  Title = 'All typed commands';      When = 'otvhelp';                                       Run = { otvhelp } }
    @{ Key = 'x';  Title = 'Back';                    When = 'to the MS6 menu';                               Run = { } }
)

function Show-OtvMenu($Items, [string]$Title) {
    Clear-Host
    Write-Otv "  OTV  -  Team H2Woah!  -  $Title"
    foreach ($item in $Items) {
        if ($item.Section) { Write-Host ''; Write-Host "  $($item.Section)" -ForegroundColor Yellow; continue }
        Write-Host ('   {0,-3}' -f $item.Key) -ForegroundColor Green -NoNewline
        Write-Host (' {0,-26}' -f $item.Title) -NoNewline
        Write-Host $item.When -ForegroundColor DarkGray
    }
    Write-Host ''
    Write-Host '   q   quit     (Ctrl+C in a monitor or watch drops you to the prompt; type otv to come back)' -ForegroundColor DarkGray
    Write-Host ''
}

function otv {
    $more = $false
    while ($true) {
        if ($more) { Show-OtvMenu $script:OtvMenuMore 'more' } else { Show-OtvMenu $script:OtvMenu 'MS6' }
        $key = (Read-Host '  Pick').Trim().ToLower()
        if ($key -eq 'q') { return }
        if (-not $key) { continue }
        if ($key -eq 'x') { $more = -not $more; continue }
        $items = if ($more) { $script:OtvMenuMore } else { $script:OtvMenu }
        $item = $items | Where-Object { $_.Key -eq $key } | Select-Object -First 1
        if (-not $item) { continue }
        Write-Host ''
        & $item.Run
        Write-Host ''
        $again = Read-Host 'Enter = back to the menu, q = quit'
        if ($again.Trim().ToLower() -eq 'q') { return }
    }
}
