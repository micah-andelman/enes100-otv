// Navigation: vision-based moves and the full mission route.
// Only calls setDrive()/stopDrive() for motion. Never touches motor
// pins or Tank directly.
// Heading convention (vision frame): 0 = +x, positive = counter-clockwise.

void navSetup() {
#if !SIM
  pinMode(PIN_IR, INPUT);
#endif
}

// ---------------- Helpers ----------------

// Enes100.println only works once the vision system has started.
// The simulator has no Serial.
void logMsg(const char *msg) {
#if !SIM
  Serial.println(msg);
#endif
  if (gVisionStarted) Enes100.println(msg);
}

void logVal(const char *label, float value) {
#if !SIM
  Serial.print(label);
  Serial.println(value);
#endif
  if (gVisionStarted) {
    Enes100.print(label);
    Enes100.println(value);
  }
}

float wrapAngle(float a) {
  while (a > PI) a -= TWO_PI;
  while (a < -PI) a += TWO_PI;
  return a;
}

// Holds the motors stopped while the marker is not visible.
// Returns false if it stays hidden for timeoutMs.
bool waitForVision(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!Enes100.isVisible()) {
    stopDrive();
    if (millis() - start >= timeoutMs) return false;
    delay(LOOP_MS);
  }
  return true;
}

bool obstacleAhead() {
#if SIM
  float d = Tank.readDistanceSensor(SIM_OBSTACLE_SENSOR_ID);
  return d > 0 && d < SIM_OBSTACLE_THRESH_M;
#else
  return digitalRead(PIN_IR) == IR_OBSTACLE_LEVEL;
#endif
}

// ---------------- Moves ----------------

// Turn in place to an absolute heading. PWM is proportional to the
// heading error, clamped to MIN_TURN_PWM..MAX_PWM.
// Positive error = counter-clockwise = left wheel back, right forward.
bool turnTo(float heading) {
  unsigned long start = millis();
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("turnTo: vision lost");
      return false;
    }
    if (millis() - start >= MOVE_TIMEOUT_MS) {
      stopDrive();
      logMsg("turnTo: timeout");
      return false;
    }

    float err = wrapAngle(heading - Enes100.getTheta());
    if (fabs(err) <= TURN_TOL_RAD) {
      stopDrive();
      return true;
    }

    int pwm = constrain((int)(TURN_GAIN * fabs(err)), MIN_TURN_PWM, MAX_PWM);
    if (err > 0) setDrive(-pwm, pwm);
    else setDrive(pwm, -pwm);
    delay(LOOP_MS);
  }
}

// Heading-corrected drive toward (x, y). Stops within stopDistance.
// Large heading errors stop and turn in place first.
bool driveTo(float x, float y, float stopDistance) {
  unsigned long start = millis();
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("driveTo: vision lost");
      return false;
    }
    if (millis() - start >= MOVE_TIMEOUT_MS) {
      stopDrive();
      logMsg("driveTo: timeout");
      return false;
    }

    float dx = x - Enes100.getX();
    float dy = y - Enes100.getY();
    if (sqrt(dx * dx + dy * dy) <= stopDistance) {
      stopDrive();
      return true;
    }

    float target = atan2(dy, dx);
    float err = wrapAngle(target - Enes100.getTheta());
    if (fabs(err) > REALIGN_RAD) {
      stopDrive();
      if (!turnTo(target)) {
        stopDrive();
        return false;
      }
      continue;
    }

    int correction = (int)(HEADING_GAIN * err);
    setDrive(DRIVE_PWM - correction, DRIVE_PWM + correction);
    delay(LOOP_MS);
  }
}

// Get past one obstacle column. Tries each lane in order: stop before
// the column facing +x, check for an obstacle, and drive through the
// first clear lane.
bool passColumn(float columnX) {
  for (int i = 0; i < NUM_LANES; i++) {
    float laneY = LANE_Y[i];
    if (!driveTo(columnX - APPROACH_DIST, laneY, POS_TOL_M)) {
      logVal("passColumn: approach failed, lane y = ", laneY);
    }
    turnTo(0.0);
    stopDrive();
    delay(OBSTACLE_PAUSE_MS);

    if (obstacleAhead()) {
      logVal("Lane blocked, y = ", laneY);
      continue;
    }

    logVal("Lane clear, y = ", laneY);
    return driveTo(columnX + CLEARANCE_DIST, laneY, POS_TOL_M);
  }
  logVal("All lanes blocked at column x = ", columnX);
  return false;
}

// ---------------- Full run ----------------
// Each leg continues even if the previous one failed, to salvage
// partial points.
void runFullRun() {
  logMsg("Run start");
  if (!waitForVision(VISION_START_MS)) {
    stopDrive();
    logMsg("No vision at start, aborting run");
    return;
  }

  // The mission site is the square we did NOT start in.
  bool startTop = Enes100.getY() > ARENA_MID_Y;
  float siteX = startTop ? MISSION_BOT_X : MISSION_TOP_X;
  float siteY = startTop ? MISSION_BOT_Y : MISSION_TOP_Y;
  logMsg(startTop ? "Started in top square, mission site is bottom"
                  : "Started in bottom square, mission site is top");

  if (!driveTo(siteX, siteY, MISSION_STOP_DIST)) {
    logMsg("Mission site approach failed, running mission anyway");
  }
  stopDrive();
  runMission();

  setDrive(-DRIVE_PWM, -DRIVE_PWM);
  delay(BACKUP_MS);
  stopDrive();

  if (!passColumn(COLUMN1_X)) logMsg("Column 1 failed, continuing");
  if (!passColumn(COLUMN2_X)) logMsg("Column 2 failed, continuing");

  // Under the limbo (top half), never over the log.
  driveTo(COLUMN2_X + CLEARANCE_DIST, LIMBO_LANE_Y, POS_TOL_M);
  turnTo(0.0);
  driveTo(GOAL_X, LIMBO_LANE_Y, POS_TOL_M);
  stopDrive();
  logMsg("Run complete");
}
