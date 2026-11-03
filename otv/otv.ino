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

void setup() {
  Serial.begin(SERIAL_BAUD);
  // Prints when this code was compiled, so you can tell
  // which version is on the board.
  Serial.print("OTV build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println(SIM ? "Target: SIMULATOR" : "Target: ROBOT");
  Serial.print("TEST_MODE: ");
  Serial.println(TEST_MODE);

  driveSetup();
  missionSetup();
  navSetup();

  // Only start vision when it is needed: hardware bench tests must run
  // without the vision system.
#if (TEST_MODE == TEST_NONE) || (TEST_MODE == TEST_VISION)
  Enes100.begin(TEAM_NAME, TEAM_TYPE, MARKER_ID, ROOM_NUMBER);
  gVisionStarted = true;
#endif
}

void loop() {
#if TEST_MODE == TEST_NONE
  static bool runDone = false;
  if (!runDone) {
    runFullRun();
    runDone = true;
  }
  stopDrive();
  delay(LOOP_MS);
#else
  runTests();
#endif
}
