# MS6 demo day checklist

MS6 = any 4 of 6 objectives on the real OTV, one at a time, unlimited
attempts. Every command below is also a key in the `otv` menu (desktop
shortcut **OTV**). Results and prints: `otvwatch` (menu `w`) in a second
PowerShell window, or the vision page (`otvvision`, room 1120, marker 72).

| Objective | Command / key | Covers |
|---|---|---|
| Nav 1: within 150 mm of the mission site | `otvsite` / `n` | Nav 1 + Mission I-III |
| Mission I-III (OTV placed at the pool) | `otvmission` / `i` | Mission I-III |
| Nav 2 + Nav 3: past the obstacles, into the goal | `otvobst` / `o` | Nav 2 + Nav 3 |

Mission I-III are not defined in the MS6 doc (probably pollution, depth,
>= 20 mL sample): confirm with the instructor.

## Before the session

1. `otvpull`: check for new teammate code.
2. `otvsimcheck -Only site,mission,obst` (menu `k`): sim dry runs must PASS.
3. Charge the battery. Pack: laptop + charger, USB cable, kill switch key,
   pool of known depth, clean + polluted water samples, measuring cup.

## Every attempt

The MS6 commands only build for the OTV: `SIM=0`, UNO R4 WiFi, marker 72,
room 1120. They refuse to upload if the board on USB is the Tank's Mega.

The vision connection is automatic. The R4 joins the vision system's WiFi
and registers as H2Woah! / marker 72 in `setup()` (code in otv.ino). Nothing
to do on the laptop for the robot to connect. The laptop only needs campus
WiFi for `otvwatch` and the vision page.

1. OTV on USB, kill switch OFF. Open the `otv` menu (desktop shortcut) and
   press the key (`n`, `i` or `o`), or type the command. It uploads and prints the steps.
2. Open a second PowerShell window and run `otvwatch` (menu `w`). It shows
   our live x / y / theta and every line the OTV prints.
3. Unplug USB. Put the OTV where the steps say, marker 72 facing up. Kill switch ON.
4. Within 30 s `otvwatch` shows our marker and `Vision system connected`, then
   the run. Prints end with `MS6: done`.
   - If the marker can't be seen at the start, it prints `No vision at start, aborting run`
     and doesn't move: check the marker is flat, uncovered and in the camera's view.
5. Retry: kill switch OFF, put it back, kill switch ON (no re-upload needed;
   power-on restarts the run).

## If something goes wrong

- `UPLOAD FAILED`: close the Serial monitor (Ctrl+C) and retry.
- `No board found`: `otvconnect`, try another USB port/cable.
- Never connects: bench test 6 (`otvrun robot -Test 6`, menu `b` then 6) on USB.

## Run log

Newest first. Date, what was run, what happened, what was changed.

| Date | Run | Result | Change made |
|---|---|---|---|
| 2026-11-18 | New ArUco marker | — | `MARKER_ID` 80 → 72 (robot builds) |
| 2026-11-18 | Set depth sensor height | Ruler, arm horizontal (servos unpowered) | `SENSOR_HEIGHT_MM` 0 → 95 |
| 2026-11-18 | Bench test 8, arm physically horizontal (battery out, servos unpowered: the 20° the code sent did not apply) | Arm ultrasonic 90.7 mm, steady over ~10 readings; ruler says ~95 mm (reads ~4 mm short). Front ultrasonic 160 mm while touching the pool: pressed against a surface it reads past it (HC-SR04 blind zone ~2-3 cm). | none |
