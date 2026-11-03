# OTV Code: Team H2Woah!

Code for our ENES100 OTV (Arduino UNO R4 WiFi).

## How teammates contribute

1. Write and test your code in the **Arduino IDE** as its own sketch (e.g. `Color_sensor.ino`).
2. Put the `.ino` file in the team Google Drive folder **ENES 100-H2Woah**.
   Use a clear name, and tell Micah when there's a new version.
3. Micah pulls it into VS Code, ports it into the OTV code, and tests it on the robot.

You don't need GitHub or any install beyond the Arduino IDE.

## Rules for your sketch

- **Use the pins from the pin map** (below). Don't reuse a pin another subsystem owns.
- **Keep the motor PWM at or below 153** (60%). The 6 V motors run on a 9.6 V pack.
- **No `pinMode()` on servo pins** after `attach()`. It kills the servo signal.
- Put the numbers you measured (angles, thresholds, flow rate) at the **top of the file**, with a comment saying how you measured them.

| Pin | Function | Pin | Function |
|---|---|---|---|
| D2 | IR sensor | D10 | Pump MOSFET gate |
| D3 / D5 | Left motor IN1 / IN2 | D11 | Servo 1 |
| D6 / D9 | Right motor IN1 / IN2 | A2 | Servo 2 |
| D7 / D8 | Ultrasonic TRIG / ECHO | A0 / A1 | Color S0 / S1 |
| A3 | Color OUT | D12 / D13 | Color S2 / S3 |
| A5 | Color LED | | |

## Where things are (for Micah)

| Path | What's in it |
|---|---|
| `otv/config.h` | Every pin, limit, and tunable. Build flags `SIM` and `TEST_MODE`. |
| `otv/otv.ino` | `setup()` and `loop()` |
| `otv/drive.ino` | Motors (simulator: Tank, robot: DRV8871) |
| `otv/mission.ino` | Arm, pump, depth, pollution |
| `otv/nav.ino` | Vision navigation (wall hug, limbo) |
| `otv/tests.ino` | Bench tests (TEST_MODE 1–7) |
| `teammates/` | Teammate sketches as downloaded from Drive (read-only) |
| `docs/bench-tests.md` | Bench test checklist |
| `docs/aruco-marker-80.svg` | Printable vision marker |
| `sim/otv_sim/otv_sim.ino` | Generated single file for the browser simulator |

## Uploading (Micah)

**Pull teammate code:** run the VS Code task **Pull Teammate Code**. It copies new or changed `.ino` files from the Drive folder into `teammates/`.

VS Code: **Ctrl+Shift+B** builds the robot target, asks for TEST_MODE and the COM port, then uploads.
Find the port with `arduino-cli board list`. Close the Serial Monitor before uploading.
After changing `otv/`, run the **Make Sim File** task to regenerate the simulator file.
