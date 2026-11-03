# OTV Code: Team H2Woah!

Code for our ENES100 OTV (Arduino UNO R4 WiFi).
**You don't need to install anything to contribute.** Everything happens on github.com.

## Where things are

| File | What's in it |
|---|---|
| `otv/config.h` | Every pin number and hardware limit. **Don't change without asking Micah.** |
| `otv/otv.ino` | `setup()` and `loop()`, which call into the files below |
| `otv/drive.ino` | Motors |
| `otv/mission.ino` | Depth, pollution, water sample, arm |
| `otv/nav.ino` | Navigation |

## How to make a change (about 2 minutes)

1. Open the file you want to change on github.com and click the **pencil icon** (top right of the file).
   - Editing several files? Press the `.` key on the repo page instead. It opens a VS Code editor in your browser.
2. Make your edit.
3. Click **Commit changes...**
4. Write one short line saying what you changed (e.g. "add ultrasonic read function").
5. Select **"Create a new branch for this commit and start a pull request"**, then click **Propose changes** and **Create pull request**.
6. Wait about 2 minutes and look at the bottom of the pull request:
   - ✅ **green check**: it compiles. Micah will test it on the OTV and merge it.
   - ❌ **red X**: it doesn't compile. Click **Details** to see the error, fix the file **on the same branch**, and commit again. The check reruns automatically.

## Rules

- **One pull request per task.** Small and focused is best.
- **Never type raw pin numbers or PWM values.** Use the names in `config.h` (e.g. `PIN_PUMP_GATE`, `MAX_PWM`). Going over `MAX_PWM` can overdrive the 6 V motors.
- You **can't** push straight to `main`. All changes go through a pull request, and that's intentional.
- Nothing reaches the robot until Micah uploads it. GitHub doesn't talk to the Arduino.

## For Micah (uploading)

VS Code tasks (Ctrl+Shift+B = Upload). Upload compiles first, then asks for the COM port.
Find the port with `arduino-cli board list`. Close the Serial Monitor before uploading.
