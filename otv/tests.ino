// Bench tests: robot only. Build with -DSIM=0 -DTEST_MODE=n.
// All output goes to Serial at SERIAL_BAUD.
//   1 Drive   2 Pump   3 Arm   4 Arm ultrasonic   5 Color   6 Vision   7 Front ultrasonic
//   8 Arm + depth calibration (interactive: type commands in the monitor)

#if !SIM

static void testDriveStep(const char *name, int left, int right) {
  Serial.println(name);
  setDrive(left, right);
  delay(TEST_STEP_MS);
  stopDrive();
  delay(TEST_GAP_MS);
}

static void testDrive() {
  // Each wheel alone first, so a swapped or backward motor is obvious.
  testDriveStep("DRIVE left wheel forward", DRIVE_PWM, 0);
  testDriveStep("DRIVE left wheel back", -DRIVE_PWM, 0);
  testDriveStep("DRIVE right wheel forward", 0, DRIVE_PWM);
  testDriveStep("DRIVE right wheel back", 0, -DRIVE_PWM);
  testDriveStep("DRIVE forward", DRIVE_PWM, DRIVE_PWM);
  testDriveStep("DRIVE reverse", -DRIVE_PWM, -DRIVE_PWM);
  testDriveStep("DRIVE spin left", -DRIVE_PWM, DRIVE_PWM);
  testDriveStep("DRIVE spin right", DRIVE_PWM, -DRIVE_PWM);
  Serial.println("DRIVE done");
}

// 2 s on/off check, then a fixed 10 s run: measure the mL from that run
// to calibrate PUMP_ML_PER_S (fixed time, so the math doesn't depend on
// the old placeholder rate).
static void testPump() {
  Serial.println("PUMP check: on 2 s");
  pumpRunMs(TEST_PUMP_CHECK_MS);
  Serial.println("PUMP check: off");
  Serial.println("Empty the measuring cup. Calibration run starts in 5 s...");
  delay(TEST_PUMP_PAUSE_MS);
  Serial.println("PUMP calibration: on 10 s");
  pumpRunMs(TEST_PUMP_CAL_MS);
  Serial.println("PUMP calibration: off. PUMP_ML_PER_S = measured mL / 10");
}

static void testArm() {
  Serial.println("ARM down");
  armDown();
  delay(TEST_ARM_HOLD_MS);
  Serial.println("ARM stow");
  armStow();
  Serial.println("ARM done");
}

static void testUltrasonic() {
  float d = readDistanceMM();
  if (d < 0) {
    Serial.println("Distance: FAILED (no echo), not data");
  } else {
    Serial.print("Distance: ");
    Serial.print(d);
    Serial.println(" mm");
  }
  delay(TEST_PRINT_MS);
}

static void testColor() {
  int r, g, b;
  readColorRaw(&r, &g, &b);
  Serial.print("R=");
  Serial.print(r);
  Serial.print(" G=");
  Serial.print(g);
  Serial.print(" B=");
  Serial.print(b);
  // 0 = that channel FAILED (timed out): don't use this line as calibration data.
  if (r == 0 || g == 0 || b == 0) {
    Serial.println("  FAILED (0 = no reading), not data");
  } else {
    // Same decision rule as isPolluted(), on this reading.
    Serial.println(colorIsPolluted(r, g, b) ? "  polluted" : "  not polluted");
  }
  delay(TEST_PRINT_MS);
}

static void testVision() {
  Serial.print("x=");
  Serial.print(Enes100.getX());
  Serial.print(" y=");
  Serial.print(Enes100.getY());
  Serial.print(" theta=");
  Serial.print(Enes100.getTheta());
  Serial.print(" visible=");
  Serial.println(Enes100.isVisible() ? "yes" : "no");
  delay(TEST_VISION_MS);
}

// Front ultrasonic (obstacle stop). Move a hand in and out and check the
// obstacle/clear switch happens near FRONT_OBSTACLE_MM.
static void testFrontUltrasonic() {
  float mm = pingMM(PIN_FRONT_US_TRIG, PIN_FRONT_US_ECHO);
  if (mm < 0) {
    Serial.println("Front: no echo (nothing in range, or FAILED if something is close)  clear");
  } else {
    Serial.print("Front: ");
    Serial.print(mm);
    Serial.println(mm < FRONT_OBSTACLE_MM ? " mm  obstacle" : " mm  clear");
  }
  delay(TEST_PRINT_MS);
}

bool gTestDone = false;

// ---- 8: live arm + depth calibration ----
// Type a command + Enter in otvmonitor. Every second it prints the arm
// angle, the arm ultrasonic, the depth it would compute, and the front
// ultrasonic. Nothing is saved on the board: '?' prints the lines to
// copy into config.h.
int gCalDownDeg = ARM_DOWN_DEG;
int gCalStowDeg = ARM_STOW_DEG;
float gCalHeightMM = SENSOR_HEIGHT_MM;
char gCalLine[16];
int gCalLen = 0;
unsigned long gCalLastPrint = 0;

static void calHelp() {
  Serial.println("ARM CAL commands (type, then Enter):");
  Serial.println("  150      move the arm to 150 deg (any 0-180)");
  Serial.println("  + / -    nudge 1 deg   (+5 / -5 = 5 deg)");
  Serial.println("  d        use this angle as ARM_DOWN_DEG (color sensor above water, tube in it)");
  Serial.println("  s        use this angle as ARM_STOW_DEG (folded flat)");
  Serial.println("  e        EMPTY bowl under the arm: reading becomes SENSOR_HEIGHT_MM");
  Serial.println("  k30      water is known to be 30 mm deep: SENSOR_HEIGHT_MM = reading + 30");
  Serial.println("  p        pump 2 s (p7 = 7 s; kill switch ON)");
  Serial.println("  ?        this help + the values to paste into config.h");
}

static void calSummary() {
  Serial.println("---- paste into otv/config.h ----");
  Serial.print("const int   ARM_STOW_DEG     = "); Serial.print(gCalStowDeg); Serial.println(";");
  Serial.print("const int   ARM_DOWN_DEG     = "); Serial.print(gCalDownDeg); Serial.println(";");
  Serial.print("const float SENSOR_HEIGHT_MM = "); Serial.print(gCalHeightMM, 1); Serial.println(";");
  Serial.println("(calibrate SENSOR_HEIGHT_MM with the arm at ARM_DOWN_DEG)");
  Serial.println("---------------------------------");
}

// Full 25-ping median for saving a value. -1 = failed.
static float calSaveReading() {
  Serial.println("Measuring (25 pings, hold still)...");
  float mm = readDistanceMM();
  if (mm < 0) Serial.println("FAILED: no echo. Nothing saved.");
  return mm;
}

static void calCommand(const char *cmd) {
  char c = cmd[0];
  if (c >= '0' && c <= '9') {
    armMoveTo(atoi(cmd));
  } else if (c == '+' || c == '-') {
    int step = (cmd[1] >= '0' && cmd[1] <= '9') ? atoi(cmd + 1) : 1;
    armMoveTo(armGetAngle() + (c == '+' ? step : -step));
  } else if (c == 'd') {
    gCalDownDeg = armGetAngle();
    Serial.print("ARM_DOWN_DEG = "); Serial.println(gCalDownDeg);
    if (gCalHeightMM > 0) Serial.println("Re-do 'e' or 'k' at this angle (the height changes with the angle).");
  } else if (c == 's') {
    gCalStowDeg = armGetAngle();
    Serial.print("ARM_STOW_DEG = "); Serial.println(gCalStowDeg);
  } else if (c == 'e') {
    float mm = calSaveReading();
    if (mm >= 0) { gCalHeightMM = mm; Serial.print("SENSOR_HEIGHT_MM = "); Serial.println(mm, 1); }
  } else if (c == 'k') {
    int known = atoi(cmd + 1);
    if (known <= 0) { Serial.println("Give the depth: k20, k30 or k40"); return; }
    float mm = calSaveReading();
    if (mm >= 0) { gCalHeightMM = mm + known; Serial.print("SENSOR_HEIGHT_MM = "); Serial.println(gCalHeightMM, 1); }
  } else if (c == 'p') {
    int secs = atoi(cmd + 1);
    unsigned long ms = (secs > 0) ? (unsigned long)min(secs, CAL_PUMP_MAX_S) * 1000UL : TEST_PUMP_CHECK_MS;
    Serial.print("Pump on "); Serial.print(ms / 1000); Serial.println(" s");
    pumpRunMs(ms);
    Serial.println("Pump off");
  } else if (c == '?' || c == 'h') {
    calHelp();
    calSummary();
  } else {
    Serial.print("Unknown command: "); Serial.println(cmd);
    calHelp();
  }
  gCalLastPrint = 0;  // print a live line right away
}

static void calLivePrint() {
  Serial.print("arm "); Serial.print(armGetAngle());
  Serial.print(" deg | arm US ");
  float mm = readDistanceSamplesMM(CAL_LIVE_SAMPLES);
  if (mm < 0) Serial.print("no echo");
  else { Serial.print(mm, 1); Serial.print(" mm"); }
  if (gCalHeightMM > 0 && mm >= 0) {
    int raw = (int)round(gCalHeightMM - mm);
    Serial.print(" | depth "); Serial.print(raw);
    Serial.print(" mm -> sends "); Serial.print(snapDepthMM(raw));
  }
  delay(US_PING_GAP_MS);
  float front = pingMM(PIN_FRONT_US_TRIG, PIN_FRONT_US_ECHO);
  Serial.print(" | front ");
  if (front < 0) Serial.println("no echo");
  else { Serial.print(front, 0); Serial.println(" mm"); }
}

static void testArmCal() {
  if (!gTestDone) {
    gTestDone = true;
    calHelp();
  }
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (gCalLen > 0) {
        gCalLine[gCalLen] = 0;
        gCalLen = 0;
        calCommand(gCalLine);
      }
    } else if (ch != ' ' && gCalLen < (int)sizeof(gCalLine) - 1) {
      gCalLine[gCalLen++] = ch;
    }
  }
  if (gCalLastPrint == 0 || millis() - gCalLastPrint >= CAL_PRINT_MS) {
    gCalLastPrint = millis();
    calLivePrint();
  }
}

// Called every loop(). One-shot tests run once, then idle with motors
// and pump off. Print tests repeat forever.
void runTests() {
  switch (TEST_MODE) {
    case TEST_DRIVE:
      if (!gTestDone) testDrive();
      gTestDone = true;
      break;
    case TEST_PUMP:
      if (!gTestDone) testPump();
      gTestDone = true;
      break;
    case TEST_ARM:
      if (!gTestDone) testArm();
      gTestDone = true;
      break;
    case TEST_ULTRASONIC:
      testUltrasonic();
      break;
    case TEST_COLOR:
      testColor();
      break;
    case TEST_VISION:
      testVision();
      break;
    case TEST_FRONT_US:
      testFrontUltrasonic();
      break;
    case TEST_ARM_CAL:
      testArmCal();
      break;
  }
}

#endif
