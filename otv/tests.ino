// Bench tests: robot only. Build with -DSIM=0 -DTEST_MODE=n.
// All output goes to Serial at SERIAL_BAUD.
//   1 Drive   2 Pump   3 Arm   4 Ultrasonic   5 Color   6 Vision   7 IR

#if !SIM

static void testDriveStep(const char *name, int left, int right) {
  Serial.println(name);
  setDrive(left, right);
  delay(TEST_STEP_MS);
  stopDrive();
  delay(TEST_GAP_MS);
}

static void testDrive() {
  testDriveStep("DRIVE forward", DRIVE_PWM, DRIVE_PWM);
  testDriveStep("DRIVE reverse", -DRIVE_PWM, -DRIVE_PWM);
  testDriveStep("DRIVE spin left", -DRIVE_PWM, DRIVE_PWM);
  testDriveStep("DRIVE spin right", DRIVE_PWM, -DRIVE_PWM);
  Serial.println("DRIVE done");
}

// Measure the actual mL collected to calibrate PUMP_ML_PER_S.
static void testPump() {
  Serial.println("PUMP START");
  pumpDispenseML(SAMPLE_ML);
  Serial.println("PUMP STOP");
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

// Checks the KY-032 polarity (IR_OBSTACLE_LEVEL is assumed LOW = obstacle).
// Hold a hand in front and watch both columns change.
static void testIR() {
  Serial.print("IR pin=");
  Serial.print(digitalRead(PIN_IR) == HIGH ? "HIGH" : "LOW");
  Serial.println(obstacleAhead() ? "  obstacle" : "  clear");
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
    case TEST_IR:
      testIR();
      break;
  }
}

#endif
