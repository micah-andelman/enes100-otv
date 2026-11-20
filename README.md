# ENES100 OTV: Team H2Woah! (Water Sampling)

Firmware for our autonomous robot (OTV) in ENES100 at the University of Maryland, Fall 2026. The OTV starts in a random corner of a 4 m × 2 m arena, finds its way to a pool using an overhead camera and an ArUco marker, lowers an arm to measure the water depth and pump a 30 mL sample, then gets past three obstacles and under a limbo bar into the goal zone.

I'm the electrical lead on an eight-person team and own the electronics and the code. Write-up with diagrams and a 3D tour: **[meandelman.com/otv](https://meandelman.com/otv)**.

## Hardware

- Arduino UNO R4 WiFi, with position and heading from the course's overhead vision system over WiFi
- Two 6 V gear motors on DRV8871 drivers, powered by a 9.6 V pack (PWM capped at 60%)
- Servo arm with an ultrasonic depth sensor and the pump hose
- Water pump switched by a MOSFET
- Front ultrasonic sensor for obstacle detection

## How the code works

| File | What it does |
|---|---|
| `otv/config.h` | Every pin, limit and tunable in one place, with plain-English notes on which way to adjust each one |
| `otv/otv.ino` | `setup()` and `loop()` |
| `otv/drive.ino` | The only file that touches motors. One build flag switches between the real robot and the simulator |
| `otv/nav.ino` | Heading control (`turnTo`, `driveTo`), wall hugging past the obstacles, limbo alignment, and the full route as separate legs |
| `otv/mission.ino` | Arm, pump, depth measurement (snapped to the known 20/30/40 mm pool depths) |
| `otv/tests.ino` | Bench tests for each subsystem, chosen with `TEST_MODE` |
| `otv/ms5.ino`, `otv/ms6.ino` | Milestone demos, each runnable on its own so a failed attempt is quick to retry |
| `sim/` | Single-file builds for the [ENES100 browser simulator](https://umdenes100.github.io/enes100-simulator/) |
| `tools/` | PowerShell scripts: build and upload, generate the sim file, and run the simulator automatically over many random starts |
| `docs/` | Bench-test checklist and demo-day run logs |

Navigation was developed in the browser simulator first, then tuned on the robot. An automated sim harness (`tools/otv.ps1`) runs dozens of randomized courses and checks that the robot never leaves the arena; the last full check passed 40/40. GitHub Actions compiles the simulator and robot targets on every push.

## How it was built

I designed the electronics, picked the parts, wired and tested the robot, and decided how the software should behave: the route strategy, the build-flag structure, which sensors to trust, and what to measure on the bench. Teammates wrote and calibrated subsystem sketches (pump, servo, color, ultrasonic) in the Arduino IDE, and I ported them into this codebase. Their original sketches aren't included here.

Built with Arduino UNO R4 WiFi, C++, arduino-cli, GitHub Actions and [Claude Code](https://claude.com/claude-code).

This is a public copy of our team's working repo, which stays private.
