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

// Drive +x along the top wall at WALL_HUG_Y until x >= xEnd, passing
// every obstacle through the obstacle-to-wall gap. Steers back toward
// the hug line in proportion to how far off it the OTV is.
bool hugWall(float xEnd) {
  unsigned long start = millis();
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("hugWall: vision lost");
      return false;
    }
    if (millis() - start >= HUG_TIMEOUT_MS) {
      stopDrive();
      logMsg("hugWall: timeout");
      return false;
    }

    if (Enes100.getX() >= xEnd) {
      stopDrive();
      return true;
    }

    float target = atan(WALL_HUG_GAIN * (WALL_HUG_Y - Enes100.getY()));
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

  // Avoid all obstacles: turn to the top wall and hug it past both columns.
  logMsg("Wall hug start");
  if (!driveTo(HUG_START_X, WALL_HUG_Y, POS_TOL_M)) logMsg("Hug line approach failed, continuing");
  turnTo(0.0);
  if (!hugWall(HUG_END_X)) logMsg("Wall hug failed, continuing");

  // Under the limbo (top half), never over the log.
  logMsg("Limbo");
  driveTo(LIMBO_ALIGN_X, LIMBO_LANE_Y, POS_TOL_M);
  turnTo(0.0);
  driveTo(GOAL_X, LIMBO_LANE_Y, POS_TOL_M);
  stopDrive();
  logMsg("Run complete");
}
