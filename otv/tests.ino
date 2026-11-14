// Bench tests: robot only. Build with -DSIM=0 -DTEST_MODE=n.
// All output goes to Serial at SERIAL_BAUD.
//   1 Drive   2 Pump   3 Arm   4 Arm ultrasonic   5 Color   6 Vision   7 Front ultrasonic

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
  }
}

#endif
