// Navigation: vision-based moves and the full mission route.
// Only calls setDrive()/stopDrive() for motion. Never touches motor
// pins or Tank directly.
// Heading convention (vision frame): 0 = +x, positive = counter-clockwise.

// Last x seen by driveTo (global: the simulator resets static locals).
// Used to size the blind drive if the limbo hides the marker.
float gLastX = 0;

#if !SIM
// Front ultrasonic filter state (globals, not static locals).
unsigned long gFrontPingMs = 0;
int gFrontHits = 0;
#endif

void navSetup() {
#if !SIM
  pinMode(PIN_FRONT_US_TRIG, OUTPUT);
  digitalWrite(PIN_FRONT_US_TRIG, LOW);
  pinMode(PIN_FRONT_US_ECHO, INPUT);
#endif
}

// ---------------- Helpers ----------------

// Enes100.println only works once the vision system has started.
// The simulator has no Serial.
void logMsg(const char *msg) {
#if HAS_SERIAL
  Serial.println(msg);
#endif
  if (gVisionStarted) Enes100.println(msg);
}

void logVal(const char *label, float value) {
#if HAS_SERIAL
  Serial.print(label);
  Serial.println(value);
#endif
  if (gVisionStarted) {
    Enes100.print(label);
    Enes100.println(value);
  }
}

// Prints where vision thinks the OTV is (or that the marker is hidden).
void logPose(const char *label) {
  logMsg(label);
  if (!Enes100.isVisible()) {
    logMsg("  pose: marker not visible");
    return;
  }
  logVal("  x = ", Enes100.getX());
  logVal("  y = ", Enes100.getY());
  logVal("  theta = ", Enes100.getTheta());
}

#if SIM
// Loop counter for simSpeedOk (global: the simulator resets static locals).
int gSpeedLoops = 0;

// The browser simulator waits between source lines: ~4-15 ms at "instant",
// 50 ms at "normal", 100 ms at "slow". At normal/slow one control-loop pass
// takes over a second, so turnTo overshoots back and forth until it times
// out. Times a few trivial loop passes. (The physical Tank passes instantly.)
bool simSpeedOk() {
  unsigned long t0 = millis();
  gSpeedLoops = 0;
  while (gSpeedLoops < SIM_SPEED_CHECK_LOOPS) {
    gSpeedLoops++;
  }
  return (millis() - t0) <= SIM_SPEED_MAX_MS;
}
#endif

float wrapAngle(float a) {
  while (a > PI) a -= TWO_PI;
  while (a < -PI) a += TWO_PI;
  return a;
}

// On the R4, Enes100.begin() returns before WiFi and the vision server
// are connected. Wait here so the start-of-run vision timeout doesn't
// get used up by a slow connection.
bool waitForConnection(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!Enes100.isConnected()) {
    if (millis() - start >= timeoutMs) return false;
    delay(LOOP_MS);
  }
  return true;
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

#if SIM
// True if (x, y) is on or next to one of the two obstacle columns.
static bool inObstacleField(float x, float y) {
  if (y < OBST_Y_MIN - OBST_HIT_MARGIN_M || y > OBST_Y_MAX + OBST_HIT_MARGIN_M) return false;
  if (fabs(x - OBST_COL1_X) <= OBST_HALF_W + OBST_HIT_MARGIN_M) return true;
  if (fabs(x - OBST_COL2_X) <= OBST_HALF_W + OBST_HIT_MARGIN_M) return true;
  return false;
}
#endif

bool obstacleAhead() {
#if SIM
  float d = Tank.readDistanceSensor(SIM_OBSTACLE_SENSOR_ID);
  if (d <= 0 || d >= SIM_OBSTACLE_THRESH_M) return false;
  // The Tank sensor also sees the arena walls. Work out where the echo
  // came from (vision pose + heading) and only count hits on an obstacle
  // column, so the top wall next to the hug lane is not an "obstacle".
  if (!Enes100.isVisible()) return true;  // can't tell: be safe
  float th = Enes100.getTheta();
  float r = SIM_SENSOR_FRONT_M + d;
  return inObstacleField(Enes100.getX() + r * cos(th), Enes100.getY() + r * sin(th));
#else
  // Front HC-SR04: ping at most every US_PING_GAP_MS (old echoes), and
  // only report an obstacle after FRONT_HITS_NEEDED close pings in a row.
  // No echo (-1) = nothing in range = clear.
  if (millis() - gFrontPingMs < US_PING_GAP_MS) return false;
  gFrontPingMs = millis();
  float mm = pingMM(PIN_FRONT_US_TRIG, PIN_FRONT_US_ECHO);
  if (mm >= 0 && mm < FRONT_OBSTACLE_MM) gFrontHits++;
  else gFrontHits = 0;
  if (gFrontHits < FRONT_HITS_NEEDED) return false;
  gFrontHits = 0;  // consumed: the next stop needs fresh close pings
  return true;
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
// Timeout = MOVE_TIMEOUT_MS plus the expected travel time, so long legs
// on the slow real robot don't time out.
bool driveTo(float x, float y, float stopDistance) {
  unsigned long start = millis();
  unsigned long timeoutMs = MOVE_TIMEOUT_MS;
  if (Enes100.isVisible()) {
    float dx0 = x - Enes100.getX();
    float dy0 = y - Enes100.getY();
    timeoutMs += (unsigned long)(1000.0 * sqrt(dx0 * dx0 + dy0 * dy0) / DRIVE_SPEED_MPS);
  }
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("driveTo: vision lost");
      return false;
    }
    if (millis() - start >= timeoutMs) {
      stopDrive();
      logMsg("driveTo: timeout");
      return false;
    }

    gLastX = Enes100.getX();
    float dx = x - gLastX;
    float dy = y - Enes100.getY();
    float dist = sqrt(dx * dx + dy * dy);
    if (dist <= stopDistance) {
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

    // Slow down near the target so it stops where it should (see APPROACH_SLOW_M).
    int pwm = constrain((int)(DRIVE_PWM * (dist - stopDistance) / APPROACH_SLOW_M), APPROACH_MIN_PWM, DRIVE_PWM);
    int correction = (int)(HEADING_GAIN * err);
    setDrive(pwm - correction, pwm + correction);
    delay(LOOP_MS);
  }
}

// Expected time for a leg of distM meters, plus slack (sized for the slow robot).
unsigned long legTimeoutMs(float distM) {
  return MOVE_TIMEOUT_MS + (unsigned long)(1000.0 * fabs(distM) / DRIVE_SPEED_MPS);
}

// Drive +x along the horizontal line y = lineY until x >= xEnd.
// Steers back toward the line in proportion to how far off it the OTV is,
// but never more than MAX_MERGE_RAD off +x, and only turns in place if the
// heading is badly off (FOLLOW_REALIGN_RAD). That keeps every merge (onto the
// hug line, down to the limbo lane, between 7b lanes) a gentle curve instead
// of a spin next to a wall or column.
// watchObstacles: stop and return FOLLOW_BLOCKED when the front sensor sees
// an obstacle (only legs that pass the obstacle columns use it: the other
// legs head at the pool or a wall, which the sensor would also see).
int followLine(float lineY, float xEnd, int pwm, bool watchObstacles, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("followLine: vision lost");
      return FOLLOW_FAILED;
    }
    if (millis() - start >= timeoutMs) {
      stopDrive();
      logMsg("followLine: timeout");
      return FOLLOW_FAILED;
    }

    gLastX = Enes100.getX();
    if (gLastX >= xEnd) {
      stopDrive();
      return FOLLOW_DONE;
    }

    if (watchObstacles && obstacleAhead()) {
      stopDrive();
      return FOLLOW_BLOCKED;
    }

    float target = atan(WALL_HUG_GAIN * (lineY - Enes100.getY()));
    target = constrain(target, -MAX_MERGE_RAD, MAX_MERGE_RAD);
    float err = wrapAngle(target - Enes100.getTheta());
    if (fabs(err) > FOLLOW_REALIGN_RAD) {
      stopDrive();
      if (!turnTo(target)) {
        stopDrive();
        return FOLLOW_FAILED;
      }
      continue;
    }

    int correction = (int)(HEADING_GAIN * err);
    setDrive(pwm - correction, pwm + correction);
    delay(LOOP_MS);
  }
}

// Wall hug: follow WALL_HUG_Y through the obstacle-to-wall gap until
// x >= xEnd. This is the only full-route leg that passes the obstacle
// columns, so it is the only one that runs the MS2 front-sensor safety
// stop: if blocked, back up and re-aim from vision, giving up after
// MAX_OBSTACLE_HITS.
bool hugWall(float xEnd) {
  unsigned long start = millis();
  int hits = 0;
  while (true) {
    unsigned long used = millis() - start;
    if (used >= HUG_TIMEOUT_MS) {
      stopDrive();
      logMsg("hugWall: timeout");
      return false;
    }
    int r = followLine(WALL_HUG_Y, xEnd, FOLLOW_PWM, true, HUG_TIMEOUT_MS - used);
    if (r == FOLLOW_DONE) return true;
    if (r == FOLLOW_FAILED) return false;
    logMsg("Obstacle ahead: backing up");
    setDrive(-DRIVE_PWM, -DRIVE_PWM);
    delay(OBSTACLE_BACKUP_MS);
    stopDrive();
    hits++;
    if (hits >= MAX_OBSTACLE_HITS) {
      logMsg("hugWall: still blocked, giving up");
      return false;
    }
  }
}

// Drive straight forward or backward (holding heading +x) until the vision
// x is within POS_TOL_M of xTarget. Used by 7b to reach the middle of the
// open space before a column without turning around.
bool moveToX(float xTarget) {
  unsigned long start = millis();
  unsigned long timeoutMs = MOVE_TIMEOUT_MS;
  if (Enes100.isVisible()) timeoutMs = legTimeoutMs(xTarget - Enes100.getX());
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("moveToX: vision lost");
      return false;
    }
    if (millis() - start >= timeoutMs) {
      stopDrive();
      logMsg("moveToX: timeout");
      return false;
    }
    float dx = xTarget - Enes100.getX();
    if (fabs(dx) <= POS_TOL_M) {
      stopDrive();
      return true;
    }
    // The heading correction (right minus left) turns the same way
    // whether the wheels go forward or backward.
    int correction = (int)(HEADING_GAIN * wrapAngle(0.0 - Enes100.getTheta()));
    int pwm = dx > 0 ? DRIVE_PWM : -DRIVE_PWM;
    setDrive(pwm - correction, pwm + correction);
    delay(LOOP_MS);
  }
}

// Last leg fallback: the limbo rod can hide the marker, and driveTo stops
// when vision is lost. If that happened, drive straight (+x, already
// aligned) for the remaining distance from the last seen x.
static void finishBlindIfVisionLost() {
  if (Enes100.isVisible()) return;  // driveTo failed for another reason
  float remaining = GOAL_X - gLastX;
  if (remaining <= 0) return;
  unsigned long ms = (unsigned long)(1000.0 * remaining / DRIVE_SPEED_MPS);
  if (ms > LIMBO_BLIND_MAX_MS) ms = LIMBO_BLIND_MAX_MS;
  logVal("Vision lost on last leg, driving blind ms = ", ms);
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(ms);
  stopDrive();
}

// ---------------- Full run ----------------
// Each leg continues even if the previous one failed, to salvage
// partial points.
void runFullRun() { runRoute(true); }

// toGoal = false stops at the limbo align point, just before the limbo
// (MS5 sub-task 6: start -> mission site -> right before the limbo).
void runRoute(bool toGoal) {
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
  logPose("Pose at start");

  if (!driveTo(siteX, siteY, MISSION_STOP_DIST)) {
    logMsg("Mission site approach failed, running mission anyway");
  }
  stopDrive();
  logPose("Pose at mission site");
  runMission();

  setDrive(-DRIVE_PWM, -DRIVE_PWM);
  delay(BACKUP_MS);
  stopDrive();
  logPose("Pose after backup");

  // Avoid all obstacles: go to x = HUG_STAGE_X at mid height, drive up that
  // line to the staging point (clear of the top pool), turn to +x there,
  // then hugWall merges onto the hug line and follows it past both columns.
  logMsg("Wall hug start");
  if (!driveTo(HUG_STAGE_X, ARENA_MID_Y, POS_TOL_M)) logMsg("Hug approach failed, continuing");
  if (!driveTo(HUG_STAGE_X, HUG_STAGE_Y, POS_TOL_M)) logMsg("Hug staging approach failed, continuing");
  logPose("Pose at hug start");
  turnTo(0.0);
  logPose("Pose after turn to +x");
  if (!hugWall(HUG_END_X)) logMsg("Wall hug failed, continuing");
  logPose("Pose after wall hug");

  // Curve down onto the limbo lane (top half: under the limbo, never over
  // the log), then straight through the limbo to the goal.
  logMsg("Limbo");
  followLine(LIMBO_LANE_Y, LIMBO_ALIGN_X, FOLLOW_PWM, false, legTimeoutMs(LIMBO_ALIGN_X - HUG_END_X + WALL_HUG_Y - LIMBO_LANE_Y));
  logPose("Pose at limbo align");
  if (!toGoal) {
    stopDrive();
    logMsg("Stopped before the limbo");
    return;
  }
  if (followLine(LIMBO_LANE_Y, GOAL_X, FOLLOW_PWM, false, legTimeoutMs(GOAL_X - LIMBO_ALIGN_X)) != FOLLOW_DONE) {
    finishBlindIfVisionLost();
  }
  stopDrive();
  logPose("Pose at end");
  logMsg("Run complete");
}
