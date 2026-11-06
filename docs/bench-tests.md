# OTV Bench Tests

Run these on the real robot before the first full run. Each test sets one or
more values in `otv/config.h`. Update the value, then re-run the test to confirm.

## Before you start

1. Battery charged, kill switch reachable, fuse in.
2. Plug the Arduino in over USB and find the port:
   ```
   arduino-cli board list
   ```
3. Build and upload a test (replace `COMx` and `N`):
   ```
   arduino-cli compile --upload -p COMx --fqbn arduino:renesas_uno:unor4wifi --build-property "compiler.cpp.extra_flags=-DSIM=0 -DTEST_MODE=N" otv
   ```
   Or in VS Code: **Ctrl+Shift+B**, pick the TEST_MODE, then type the port.
4. Watch the output (close it before the next upload):
   ```
   arduino-cli monitor -p COMx -c baudrate=115200
   ```
   The first lines show the build time, `Target: ROBOT`, and `TEST_MODE`. Check
   they match what you just uploaded.

## Order

Do them in this order. Drive first, because a backwards motor is the most
likely thing to damage something later.

| # | Test | Needs | Sets in `config.h` |
|---|---|---|---|
| 1 | Drive | Wheels off the ground | `LEFT_INVERT`, `RIGHT_INVERT`, `MIN_TURN_PWM` |
| 7 | IR | Your hand | `IR_OBSTACLE_LEVEL` |
| 3 | Arm | Arm built, clear space around it | `ARM_STOW_DEG`, `ARM_DOWN_DEG`, `SERVO2_MIRRORED` |
| 2 | Pump | Cup of water, measuring cup or syringe | `PUMP_ML_PER_S` |
| 4 | Ultrasonic | Arm down over a bowl, ruler | `SENSOR_HEIGHT_MM` |
| 5 | Color | Clean water + the plastic debris | `COLOR_POLLUTED_THRESH` |
| 6 | Vision | Lab 1120 arena, marker 80 on top | nothing (confirms WiFi + marker) |

---

## 1. Drive (`TEST_MODE=1`)

Runs once: forward, reverse, spin left, spin right, about 1 s each.

- [ ] Wheels off the ground (robot on a box).
- [ ] **Forward:** both wheels roll so the robot would move forward.
      If one wheel turns backward, flip that side's `LEFT_INVERT` / `RIGHT_INVERT`.
- [ ] **Spin left:** left wheel backward, right wheel forward (counter-clockwise from above).
      If it's mirrored, the two motors are swapped in the wiring. Fix the wiring, not the code.
- [ ] Nothing gets hot; the 7.5 A fuse holds.
- [ ] **Min turn PWM (on the floor):** lower `MIN_TURN_PWM` until the robot just
      barely turns in place, then add about 10. Current value: 70.

## 7. IR (`TEST_MODE=7`)

Prints the IR pin level and `obstacle` / `clear` every 0.5 s.

- [ ] Nothing in front: should print `clear`.
- [ ] Hand about 10 cm in front: should print `obstacle`.
- [ ] If it's backwards, change `IR_OBSTACLE_LEVEL` to `HIGH`.
- [ ] Adjust the KY-032 range knob so it sees a hand at about 15–20 cm but **not**
      the side wall when the robot is parallel to it 1–2 cm away. It runs
      during the wall hug.

## 3. Arm (`TEST_MODE=3`)

Runs once: sweep down, hold 2 s, sweep back to stow.

- [ ] At power-up the arm jumps to stow (normal).
- [ ] **Stow (20°):** arm lies flat over the chassis and the robot is under 18.4 cm tall (limbo).
- [ ] **Down (160°):** sensors at the end of the arm hang over where the water will be.
- [ ] Both servos move the same way. If one fights the other, set `SERVO2_MIRRORED = true`.
- [ ] Arduino doesn't reset during the sweep (5 V rail OK).

## 2. Pump (`TEST_MODE=2`)

Runs once: prints `PUMP START`, runs for 30 mL at the configured rate (12 s at 2.5 mL/s), prints `PUMP STOP`.

- [ ] Inlet tube in a cup of water, outlet into a measuring cup.
- [ ] Measure the mL collected: ______ mL
- [ ] New `PUMP_ML_PER_S` = measured mL ÷ 12 s = ______
- [ ] Re-run and confirm you get at least 20 mL (target 30).

## 4. Ultrasonic (`TEST_MODE=4`)

Prints the averaged distance in mm every 0.5 s (`-1` = no echo).

- [ ] Arm down over an **empty** bowl. Reading = sensor height above the bowl
      bottom. Set `SENSOR_HEIGHT_MM` to it: ______ mm
- [ ] Fill to 20, 30, 40 mm. Check that `SENSOR_HEIGHT_MM − reading` is within
      4 mm of each depth:
  - 20 mm → ______
  - 30 mm → ______
  - 40 mm → ______
- [ ] Readings are steady (if they jump, tell Micah: the pings fire back to back).

## 5. Color (`TEST_MODE=5`)

Prints raw R/G/B pulse widths (lower = more of that color) and the decision every 0.5 s.

- [ ] LED on the sensor is lit.
- [ ] Clean water, no debris. R/G/B: ______ / ______ / ______ → should say `not polluted`.
- [ ] Each piece of plastic debris under the sensor. Lowest channel: ______ → should say `polluted`.
- [ ] Set `COLOR_POLLUTED_THRESH` between the two (above the debris reading, below the clean reading).
- [ ] A reading of 0 means no pulse (sensor unplugged or OUT not on A3). It reads as polluted, so check the wiring.

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
