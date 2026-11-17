# MS5 demo day checklist

MS5 = sub-tasks 1 + 2 + any six of 3-10. Every command below is also a key in
the `otv` menu (desktop shortcut **OTV**). Run one thing at a time and let it
finish before starting the next.

## The night before

1. `otvpull`: check for new teammate code (port it if something changed).
2. `otvsimcheck -Runs 3` (menu `k`): every sim demo must PASS.
3. Charge the battery. Pack: laptop + charger, USB cable, kill switch key, cup
   of water + measuring cup (sub-task 10), a box (sub-task 8).

## Simulator demos (laptop only)

Each one opens its own Chrome window, sets everything up and runs by itself.
Progress prints in the PowerShell window; it says `Done.` at the end.
The window opens maximized with the code hidden (arena, pose and console
only); the sim console ends with `MS5 demo DONE - robot stopped`. Do not
switch it to fullscreen (F11): fullscreen runs hit obstacles in testing.

| Sub-task | Command / key | What the grader sees |
|---|---|---|
| 4 wireless receive | `otv4` / `4` | x, y, theta printed 20 times; theta changes while it spins |
| 5 wireless transmit | `otv5` / `5` | depth 30 mm + unpolluted sent once each |
| 6 navigation | `otv6` / `6` | random start, drives to the mission site, stops before the limbo |
| 7 obstacles (wall hug) | `otv7a` / `7a` | full route: mission site, past all obstacles, under the limbo, goal |
| 7 obstacles (detour) | `otv7b` / `7b` | drives at an obstacle, sees it, backs up, changes lane, gets past |

If a run says `Automatic run failed`, the code is already on the clipboard:
follow the 4 steps it prints (speed must be **instant**).

## Robot demos (OTV on USB)

Before the first robot demo: bench test 1 (`otvrun robot -Test 1`, menu `b`
then 1, wheels off the ground) to confirm both wheels spin the right way.
Flip `LEFT_INVERT` / `RIGHT_INVERT` in `config.h` if not.

| Sub-task | Command / key | Steps |
|---|---|---|
| 2 forward 3.4 m (required) | `otv2 -Seconds 45` / `2` | upload, unplug, OTV in the landing zone facing the limbo, kill switch, step back (starts 5 s later), catch it past 3.4 m. Too short or long: change the seconds (distance = seconds x speed). |
| 3 three 90° turns | `otv3 -TurnMs 950` / `3` | upload, unplug, kill switch. Under- or over-turning: scale TurnMs (e.g. 80° each -> 950 x 90/80). Tune it once before the demo. |
| 8 obstacle sensor | `otv8` / `8` | stays on USB; move a box toward the front sensor; mm readout and `obstacle` under 150 mm |
| 9 mission (depth) sensor | `otv9` / `9` | stays on USB; arm ultrasonic mm readout over a ruler / the bowl |
| 10 arm + pump | `otv10` / `10` | USB + kill switch ON; press Enter: arm down, pump 2 s, arm stow; repeatable |

Dry runs of 2, 3, 8 and 10 in the simulator (`otv2sim`, `otv3sim`, `otv8sim`,
`otv10sim`, menu `2s` `3s` `8s` `10s`) run the same code at the sim Tank's
speed. They check the logic, not the robot's timing.

## If something goes wrong

- `UPLOAD FAILED`: close the Serial monitor (Ctrl+C) and retry.
- `No board found`: `otvconnect`, try another USB port/cable.
- Sim says `another sim run is still going`: wait for it, or close that PowerShell window.
