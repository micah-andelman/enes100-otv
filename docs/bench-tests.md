# OTV Bench Tests

Run these on the real robot before the first full run. Each test sets one or
more values in `otv/config.h`. Update the value, then re-run the test to confirm.

## Before you start

1. Battery charged, kill switch reachable, fuse in.
2. Plug the Arduino in over USB and check it's found (any PowerShell window):
   ```
   otvconnect
   ```
3. Build and upload test `N` (port is found automatically):
   ```
   otvrun robot -Test N
   ```
   Or in VS Code: **Ctrl+Shift+B**, pick the TEST_MODE, then type the port.
4. Watch the output (Ctrl+C to close it before the next upload):
   ```
   otvmonitor
   ```
   The first lines show the build time, `Target: ROBOT`, and `TEST_MODE`. Check
   they match what you just uploaded.

## Order

Do them in this order. Drive first, because a backwards motor is the most
likely thing to damage something later.

| # | Test | Needs | Sets in `config.h` |
|---|---|---|---|
| 1 | Drive | Wheels off the ground | `LEFT_INVERT`, `RIGHT_INVERT`, `MIN_TURN_PWM` |
| 7 | Front ultrasonic | Your hand | `FRONT_OBSTACLE_MM` |
| 3 | Arm | Arm built, clear space around it | `ARM_STOW_DEG`, `ARM_DOWN_DEG`, `SERVO2_MIRRORED` |
| 2 | Pump | Cup of water, measuring cup or syringe | `PUMP_ML_PER_S` |
| 4 | Ultrasonic | Arm down over a bowl, ruler | `SENSOR_HEIGHT_MM` |
| 8 | Arm + depth calibration | Bowl, water, ruler | `ARM_DOWN_DEG`, `ARM_STOW_DEG`, `SENSOR_HEIGHT_MM` (replaces 3 + 4) |
| 5 | Color | Clean water + the plastic debris | `COLOR_POLLUTED_THRESH` |
| 6 | Vision | Lab 1120 arena, marker 80 on top | nothing (confirms WiFi + marker) |

---

## 1. Drive (`TEST_MODE=1`)

Runs once, about 1 s each: left wheel forward, left wheel back, right wheel
forward, right wheel back, then both forward, reverse, spin left, spin right.

- [ ] Wheels off the ground (robot on a box).
- [ ] **Left wheel / right wheel alone:** the named wheel moves, the other stays still.
      Wrong wheel moves = left and right motors are swapped in the wiring.
- [ ] **Forward:** both wheels roll so the robot would move forward.
      If one wheel turns backward, flip that side's `LEFT_INVERT` / `RIGHT_INVERT`.
- [ ] **Spin left:** left wheel backward, right wheel forward (counter-clockwise from above).
      If it's mirrored, the two motors are swapped in the wiring. Fix the wiring, not the code.
- [ ] Nothing gets hot; the 7.5 A fuse holds.
- [ ] **Min turn PWM (on the floor):** lower `MIN_TURN_PWM` until the robot just
      barely turns in place, then add about 10. Current value: 70.

## 7. Front ultrasonic (`TEST_MODE=7`)

Prints the front distance (mm) and `obstacle` / `clear` every 0.5 s.
Wiring: TRIG D2, ECHO A4.

- [ ] Nothing in front: `no echo` or a large distance, `clear`.
- [ ] Hand closer than `FRONT_OBSTACLE_MM` (150 mm): `obstacle`.
- [ ] Robot parallel to a wall 1–2 cm away (as in the wall hug), and angled
      about 35° toward it from ~30 cm: should stay `clear`. If not, lower
      `FRONT_OBSTACLE_MM`. The beam is ~30° wide and the hug lane is tight.

## 3. Arm (`TEST_MODE=3`)

Runs once: sweep down, hold 2 s, sweep back to stow.

- [ ] At power-up the arm jumps to stow (normal).
- [ ] **Stow (20°):** arm lies flat over the chassis and the robot is under 18.4 cm tall (limbo).
- [ ] **Down (96°):** arm ultrasonic parallel to the water, pump tube in the water.
- [ ] Both servos move the same way. If one fights the other, set `SERVO2_MIRRORED = true`.
- [ ] Arduino doesn't reset during the sweep (5 V rail OK).

## 2. Pump (`TEST_MODE=2`)

Runs once: pump on 2 s (check), off, a 5 s pause, then on for exactly 10 s (calibration).

- [ ] Inlet tube in a cup of water, outlet into a measuring cup.
- [ ] **Check (2 s):** the pump runs, then stops. Water moves the right way.
- [ ] Empty the measuring cup during the 5 s pause.
- [ ] **Calibration (10 s):** measure the mL collected: ______ mL
- [ ] New `PUMP_ML_PER_S` = measured mL ÷ 10 = ______
- [ ] Optional: run the full mission on the bench and confirm at least 20 mL (target 30).

## 4. Ultrasonic (`TEST_MODE=4`)

Arm sensor (TRIG D7, ECHO D8). Prints the median of 25 pings in mm every ~2 s.
`FAILED (no echo)` lines are not data.

- [ ] Arm down over an **empty** bowl. Reading = sensor height above the bowl
      bottom. Set `SENSOR_HEIGHT_MM` to it: ______ mm
- [ ] Fill to 20, 30, 40 mm. Check that `SENSOR_HEIGHT_MM − reading` is within
      4 mm of each depth:
  - 20 mm → ______
  - 30 mm → ______
  - 40 mm → ______
- [ ] Readings are steady. `FAILED (no echo)` lines are not data.

## 8. Arm + depth calibration (`TEST_MODE=8`, `otvarm`, menu `a`)

Interactive. Type a command + Enter in the monitor. Every second it prints:
`arm 150 deg | arm US 87.3 mm | depth 31 mm -> sends 30 | front 405 mm`
(depth only once the sensor height is set).

| Command | Does |
|---|---|
| `150` | sweep the arm to 150° (0–180) |
| `+` / `-`, `+5` / `-5` | nudge 1° / 5° |
| `d` | keep this angle as `ARM_DOWN_DEG` |
| `s` | keep this angle as `ARM_STOW_DEG` |
| `e` | empty bowl under the sensor: reading → `SENSOR_HEIGHT_MM` |
| `k30` | water known to be 30 mm deep: `SENSOR_HEIGHT_MM` = reading + 30 |
| `p`, `p7` | pump 2 s / 7 s (max 30) |
| `?` | help + the three lines to paste into `otv/config.h` |

Nothing is stored on the board. Paste the `?` lines into `config.h`.

- [ ] Kill switch ON (servos need the 5 V rail).
- [ ] Stow: nudge until the arm lies flat, `s`. Check the OTV is under 18.4 cm tall.
- [ ] Down: robot at the pool edge as in the mission, deepest water (40 mm). Nudge until
      the pump tube is in the water and the color sensor is clearly above it, `d`.
      Check at 20 mm water that the tube still reaches. If no angle does both, the tube
      must hang lower than the color sensor (mechanical fix).
- [ ] At that down angle: `e` over the empty bowl (or `k30` with 30 mm of water).
- [ ] Fill 20 / 30 / 40 mm: the live `depth` is within 4 mm and `sends` is right.
- [ ] `p` with the tube in water: water flows.
- [ ] `?`, paste the lines into `config.h`, re-run `otv10` to check.

## 5. Color (`TEST_MODE=5`)

Prints raw R/G/B pulse widths (lower = more of that color) and the decision every 0.5 s.

- [ ] LED on the sensor is lit.
- [ ] Clean water, no debris. R/G/B: ______ / ______ / ______ → should say `not polluted`.
- [ ] Each piece of plastic debris under the sensor. Lowest channel: ______ → should say `polluted`.
- [ ] Set `COLOR_POLLUTED_THRESH` between the two (above the debris reading, below the clean reading).
- [ ] A `FAILED` line means a channel got no pulse (sensor unplugged or OUT not on D4). Don't use it as data; check the wiring.

## 6. Vision (`TEST_MODE=6`)

Connects to the vision system, then prints x, y, theta, visible every 0.25 s.

- [ ] In room 1120, arena WiFi up, marker 80 on top of the OTV.
- [ ] Gets past connecting within 30 s.
- [ ] `visible=yes` anywhere on the arena.
- [ ] Place the OTV on square A and square B: about (0.5, 1.5) and (0.5, 0.5).
- [ ] Facing +x (toward the goal), theta ≈ 0; facing up, ≈ +1.57.
- [ ] Also record, to tune later:
  - OTV front touching the pool edge at the mission site: x ______ y ______
  - OTV centered in the limbo opening: x ______ y ______

## After the bench tests

Full run (`TEST_MODE=0`) in the arena. Values to tune there: `MISSION_STOP_DIST`
(front within 150 mm of the pool), `HUG_STAGE_X`/`HUG_STAGE_Y`, `HUG_END_X`, `LIMBO_ALIGN_X`,
`DRIVE_SPEED_MPS`.
