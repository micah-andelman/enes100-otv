// OTV main sketch: Team H2Woah! (ENES100, Water Sampling)
// setup() and loop() live here. Subsystem code lives in drive.ino,
// mission.ino, nav.ino, tests.ino. Pins/limits/tunables live in config.h.
//
// Build target: -DSIM=1 (default, browser simulator) or -DSIM=0 (robot).
// Bench tests: -DSIM=0 -DTEST_MODE=1..6 (see config.h).
#include <Enes100.h>
#include "config.h"
#include "otv.h"
#if SIM
#include <Tank.h>
#endif

bool gVisionStarted = false;
bool gRunDone = false;  // global, not a static local (the simulator resets those)

void setup() {
#if !SIM
  // The simulator has no Serial.
  Serial.begin(SERIAL_BAUD);
  // Prints when this code was compiled, so you can tell
  // which version is on the board.
  Serial.print("OTV build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("Target: ROBOT");
  Serial.print("TEST_MODE: ");
  Serial.println(TEST_MODE);
  Serial.print("MS5_TASK: ");
  Serial.println(MS5_TASK);
#endif

  driveSetup();
  missionSetup();
  navSetup();

  // Only start vision when it is needed: hardware bench tests must run
  // without the vision system. Robot MS5 demos (2, 3, 10) don't use vision.
#if (TEST_MODE == TEST_VISION) || ((TEST_MODE == TEST_NONE) && (SIM || (MS5_TASK == MS5_NONE)))
  Enes100.begin(TEAM_NAME, TEAM_TYPE, MARKER_ID, ROOM_NUMBER, VISION_WIFI_TX, VISION_WIFI_RX);
  gVisionStarted = true;
  if (!waitForConnection(WIFI_CONNECT_MS)) logMsg("Vision system not connected, continuing");
#endif
#if SIM
  logMsg("Target: SIMULATOR");
  if (!simSpeedOk()) {
    // Stop here: nothing steers correctly at this speed.
    // (No commas: the simulator console drops the space after them.)
    logMsg("STOPPED: simulator speed is too slow.");
    logMsg("1. Click Stop (top-left square).");
    logMsg("2. Click the speed button (2nd from the left) until its tooltip says instant.");
    logMsg("3. Click Run and then Continue again.");
    while (true) {
      stopDrive();
      delay(1000);
    }
  }
#endif
}

void loop() {
#if MS5_TASK != MS5_NONE
  runMs5();
#elif TEST_MODE == TEST_NONE
  if (!gRunDone) {
    runFullRun();
    gRunDone = true;
  }
  stopDrive();
  delay(LOOP_MS);
#else
  runTests();
#endif
}
