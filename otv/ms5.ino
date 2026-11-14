// Milestone 5 demos. Build with -DMS5_TASK=n (tools/otv.ps1: otv2..otv10).
// Numbers match the MS5 sub-tasks (7a = 71, 7b = 72). See config.h.
// runMs5() is called every loop(); one-shot demos run once, then idle.

bool gMs5Done = false;  // global: the simulator resets static locals

#if MS5_TASK != MS5_NONE

// ---------------- Robot demos (no vision) ----------------
#if !SIM

static void ms5StartCountdown() {
  logVal("MS5 demo starts in ms: ", MS5_START_DELAY_MS);
  delay(MS5_START_DELAY_MS);
}

// Sub-task 2: forward locomotion. Straight, open loop, longer than 3.4 m.
static void ms5Forward() {
  ms5StartCountdown();
  logVal("MS5 2: driving forward, ms = ", MS5_FORWARD_MS);
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(MS5_FORWARD_MS);
  stopDrive();
  logMsg("MS5 2: stopped");
}

// Sub-task 3: three 90 deg turns in place (counter-clockwise), timed.
static void ms5Turn() {
  ms5StartCountdown();
  for (int i = 1; i <= 3; i++) {
    logVal("MS5 3: turn ", i);
    setDrive(-DRIVE_PWM, DRIVE_PWM);
    delay(MS5_TURN90_MS);
    stopDrive();
    delay(MS5_TURN_PAUSE_MS);
  }
  logMsg("MS5 3: done");
}

// Sub-task 10: mission actuation. Runs on any Serial key, repeatable.
static void ms5Actuate() {
  if (!gMs5Done) {
    logMsg("MS5 10: press Enter to run arm down -> pump -> arm stow");
    gMs5Done = true;
  }
  if (!Serial.available()) return;
  while (Serial.available()) Serial.read();
  logMsg("MS5 10: arm down");
  armDown();
  delay(MS5_ARM_HOLD_MS);
  logMsg("MS5 10: pump on");
  pumpRunMs(TEST_PUMP_CHECK_MS);
  logMsg("MS5 10: pump off, arm stow");
  armStow();
  logMsg("MS5 10: done. Press Enter to run again");
}

#endif  // !SIM

// ---------------- Simulator demos (vision) ----------------
#if SIM

// Sub-task 4: wireless receive. Prints pose while spinning slowly so
// theta visibly changes.
static void ms5Receive() {
  logMsg("MS5 4: wireless receive");
  if (!waitForVision(VISION_START_MS)) logMsg("MS5 4: marker not visible yet");
  for (int i = 0; i < MS5_RECEIVE_PRINTS; i++) {
    if (i == MS5_RECEIVE_PRINTS / 2) {
      logMsg("MS5 4: spinning so the heading changes");
      setDrive(-MIN_TURN_PWM, MIN_TURN_PWM);
    }
    logPose("Received from vision:");
    delay(MS5_PRINT_MS);
  }
  stopDrive();
  logMsg("MS5 4: done");
}

// Sub-task 5: wireless transmit. Sends both mission messages once.
static void ms5Transmit() {
  logMsg("MS5 5: sending mission messages");
  Enes100.mission(DEPTH, SIM_DEPTH_MM);
  Enes100.mission(WATER_TYPE, SIM_POLLUTED ? FRESH_POLLUTED : FRESH_UNPOLLUTED);
  logVal("MS5 5: sent depth mm = ", SIM_DEPTH_MM);
  logMsg(SIM_POLLUTED ? "MS5 5: sent polluted" : "MS5 5: sent unpolluted");
}

// Sub-task 7b: drive down a lane at the obstacles. When the distance
// sensor sees one: stop, back up, sidestep to the nearest untried lane,
// face +x, and continue until past both columns.
int gLane = 1;          // 0 low, 1 mid, 2 high
int gLaneTried = 0;     // bit per lane tried in the current column
float gBlockX = -1;     // x where the last block happened (-1 = none)

static float laneY(int lane) {
  if (lane == 0) return MS5_LANE_LOW_Y;
  if (lane == 1) return MS5_LANE_MID_Y;
  return MS5_LANE_HIGH_Y;
}

static int nearestLane(float y) {
  int best = 0;
  for (int i = 1; i < 3; i++) {
    if (fabs(laneY(i) - y) < fabs(laneY(best) - y)) best = i;
  }
  return best;
}

// Nearest lane not tried yet in this column, or -1.
static int nextLane() {
  int best = -1;
  for (int i = 0; i < 3; i++) {
    if (gLaneTried & (1 << i)) continue;
    if (best < 0 || abs(i - gLane) < abs(best - gLane)) best = i;
  }
  return best;
}

static void ms5Detour() {
  logMsg("MS5 7b: obstacle encounter and detour");
  if (!waitForVision(VISION_START_MS)) {
    logMsg("MS5 7b: no vision, aborting");
    return;
  }
  gLane = nearestLane(Enes100.getY());
  gLaneTried = 0;
  gBlockX = -1;
  driveTo(MS5_DETOUR_START_X, laneY(gLane), POS_TOL_M);
  turnTo(0.0);
  logPose("MS5 7b: lined up on lane");

  unsigned long start = millis();
  while (true) {
    if (!waitForVision(VISION_LOST_MS)) {
      stopDrive();
      logMsg("MS5 7b: vision lost");
      return;
    }
    if (millis() - start >= MS5_DETOUR_TIMEOUT_MS) {
      stopDrive();
      logMsg("MS5 7b: timeout");
      return;
    }
    float x = Enes100.getX();
    if (x >= MS5_DETOUR_END_X) {
      stopDrive();
      logPose("MS5 7b: past the obstacles");
      return;
    }
    if (gBlockX >= 0 && x > gBlockX + MS5_LANE_CLEAR_M) {
      gLaneTried = 0;  // past that column: any lane may be blocked differently ahead
      gBlockX = -1;
    }

    if (obstacleAhead()) {
      stopDrive();
      logPose("MS5 7b: obstacle encountered");
      setDrive(-DRIVE_PWM, -DRIVE_PWM);
      delay(OBSTACLE_BACKUP_MS);
      stopDrive();
      gLaneTried |= (1 << gLane);
      gBlockX = x;
      int next = nextLane();
      if (next < 0) {
        logMsg("MS5 7b: every lane blocked, giving up");
        return;
      }
      gLane = next;
      logVal("MS5 7b: detour to lane y = ", laneY(gLane));
      waitForVision(VISION_LOST_MS);
      driveTo(Enes100.getX(), laneY(gLane), POS_TOL_M);
      turnTo(0.0);
      continue;
    }

    // Follow the lane, steering back toward it like hugWall does.
    float target = atan(WALL_HUG_GAIN * (laneY(gLane) - Enes100.getY()));
    float err = wrapAngle(target - Enes100.getTheta());
    if (fabs(err) > REALIGN_RAD) {
      stopDrive();
      turnTo(target);
      continue;
    }
    int correction = (int)(HEADING_GAIN * err);
    setDrive(DRIVE_PWM - correction, DRIVE_PWM + correction);
    delay(LOOP_MS);
  }
}

#endif  // SIM

void runMs5() {
#if MS5_TASK == MS5_ACTUATE
  ms5Actuate();
#else
  if (gMs5Done) {
    stopDrive();
    delay(LOOP_MS);
    return;
  }
  gMs5Done = true;
#if MS5_TASK == MS5_FORWARD
  ms5Forward();
#elif MS5_TASK == MS5_TURN
  ms5Turn();
#elif MS5_TASK == MS5_RECEIVE
  ms5Receive();
#elif MS5_TASK == MS5_TRANSMIT
  ms5Transmit();
#elif MS5_TASK == MS5_NAV
  runRoute(false);
#elif MS5_TASK == MS5_WALLHUG
  runFullRun();
#elif MS5_TASK == MS5_DETOUR
  ms5Detour();
#endif
  stopDrive();
#endif  // MS5_ACTUATE
}

#endif  // MS5_TASK != MS5_NONE
