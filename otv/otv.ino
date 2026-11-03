// OTV main sketch: Team H2Woah! (ENES100)
// setup() and loop() live here. Subsystem code lives in
// drive.ino, mission.ino, nav.ino. Pins/limits live in config.h.
#include "config.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  // Prints when this code was compiled, so you can tell
  // which version is on the board.
  Serial.print("OTV build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);

  driveSetup();
  missionSetup();
  navSetup();
}

void loop() {
}
