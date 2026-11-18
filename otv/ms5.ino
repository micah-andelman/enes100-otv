// Milestone 5 demos. Build with -DMS5_TASK=n (tools/otv.ps1: otv2..otv10).
// Numbers match the MS5 sub-tasks (7a = 71, 7b = 72). See config.h.
// runMs5() is called every loop(); one-shot demos run once, then idle.

bool gMs5Done = false;  // global: the simulator resets static locals

#if MS5_TASK != MS5_NONE

// ---------------- Robot demos 2, 3, 10 ----------------
// The robot runs these blind (no vision). With SIM=1 they are simulator dry
// runs of the same code: the sim lines up with vision first (the team does
// that by hand on the robot) and reports what vision saw afterwards.

static void ms5StartCountdown() {
  logVal("MS5 demo starts in ms: ", MS5_START_DELAY_MS);
  delay(MS5_START_DELAY_MS);
}

#if SIM
// Face +x from wherever Randomize put the Tank (the robot is placed that way).
static bool ms5SimLineUp() {
  if (!waitForVision(VISION_START_MS)) {
    logMsg("MS5: no vision, aborting");
    return false;
  }
  return turnTo(0.0);
}
#endif

// Sub-task 2: forward locomotion. Straight, open loop, longer than 3.4 m.
static void ms5Forward() {
#if SIM
  if (!ms5SimLineUp()) return;
  float x0 = Enes100.getX();
  float y0 = Enes100.getY();
  // The sim Tank is 3.5x faster: drive for the time that ends before the limbo.
  unsigned long ms = (unsigned long)(1000.0 * (MS5_SIM_FORWARD_END_X - x0) / SIM_DRIVE_MPS);
#else
  unsigned long ms = MS5_FORWARD_MS;
#endif
  ms5StartCountdown();
  logVal("MS5 2: driving forward for ms = ", ms);
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(ms);
  stopDrive();
  logMsg("MS5 2: stopped");
#if SIM
  delay(MS5_PRINT_MS);
  float dx = Enes100.getX() - x0;
  float dy = Enes100.getY() - y0;
  logPose("MS5 2: end pose");
  logVal("MS5 2: distance m = ", sqrt(dx * dx + dy * dy));
  logVal("MS5 2: sideways drift m = ", dy);
  logMsg("MS5 2: done");
#endif
}

// Sub-task 3: three 90 deg turns in place (counter-clockwise), timed.
static void ms5Turn() {
#if SIM
  if (!ms5SimLineUp()) return;
  unsigned long turnMs = MS5_SIM_TURN90_MS;
#else
  unsigned long turnMs = MS5_TURN90_MS;
#endif
  ms5StartCountdown();
  for (int i = 1; i <= 3; i++) {
#if SIM
    float th0 = Enes100.getTheta();
#endif
    logVal("MS5 3: turn ", i);
    setDrive(-DRIVE_PWM, DRIVE_PWM);
    delay(turnMs);
    stopDrive();
    delay(MS5_TURN_PAUSE_MS);
#if SIM
    logVal("MS5 3:   turned deg = ", wrapAngle(Enes100.getTheta() - th0) * 180.0 / PI);
#endif
  }
  logMsg("MS5 3: done");
}

// Sub-task 10: mission actuation. Robot: runs on any Serial key, repeatable.
// Sim (no Serial): runs once.
static void ms5ActuateOnce() {
  logMsg("MS5 10: arm down");
  armDown();
  delay(MS5_ARM_HOLD_MS);
  logMsg("MS5 10: pump on");
  pumpRunMs(TEST_PUMP_CHECK_MS);
  logMsg("MS5 10: pump off then arm stow");
  armStow();
}

#if !SIM
static void ms5Actuate() {
  if (!gMs5Done) {
    logMsg("MS5 10: press Enter to run arm down -> pump -> arm stow");
    gMs5Done = true;
  }
  if (!Serial.available()) return;
  while (Serial.available()) Serial.read();
  ms5ActuateOnce();
  logMsg("MS5 10: done. Press Enter to run again");
}
#endif

// ---------------- Physical Tank: sub-tasks 4 + 5, real vision system ----------------
// Upload over USB with the Tank power OFF, unplug, then power on (the Tank
// doesn't drive in these). Every line goes to the vision system's team
// console, which otvwatch prints in PowerShell; on USB it also goes to Serial.
#if TANK_HW
unsigned long gTankLastPrint = 0;
unsigned long gTankLastSend = 0;
int gTankSends = 0;
int gTankDepthMM = SIM_DEPTH_MM;
bool gTankPolluted = SIM_POLLUTED;
char gTankLine[8];
int gTankLen = 0;
char gTankOut[96];

// Throttled "still connecting" line (Serial only: no WiFi yet).
static bool tankConnected() {
  if (Enes100.isConnected()) return true;
  if (millis() - gTankLastPrint >= MS5_PRINT_MS * 2) {
    gTankLastPrint = millis();
    Serial.println("Vision system: not connected yet (WiFi)...");
  }
  return false;
}

// Sub-task 4: print what the Arduino receives from the vision system.
// dtostrf: AVR printf has no %f.
static void ms5TankReceive() {
  if (!tankConnected()) return;
  if (millis() - gTankLastPrint < MS5_PRINT_MS) return;
  gTankLastPrint = millis();
  if (!Enes100.isVisible()) {
    snprintf(gTankOut, sizeof(gTankOut), "Received: marker %d NOT VISIBLE", MARKER_ID);
    logMsg(gTankOut);
    return;
  }
  char xs[10], ys[10], ts[10], ds[10];
  float th = Enes100.getTheta();
  dtostrf(Enes100.getX(), 1, 3, xs);
  dtostrf(Enes100.getY(), 1, 3, ys);
  dtostrf(th, 1, 3, ts);
  dtostrf(th * 180.0 / PI, 1, 1, ds);
  snprintf(gTankOut, sizeof(gTankOut), "Received x=%s m  y=%s m  theta=%s rad (%s deg)", xs, ys, ts, ds);
  logMsg(gTankOut);
}

static void tankSendMission() {
  Enes100.mission(DEPTH, gTankDepthMM);
  Enes100.mission(WATER_TYPE, gTankPolluted ? FRESH_POLLUTED : FRESH_UNPOLLUTED);
  gTankSends++;
  gTankLastSend = millis();
  snprintf(gTankOut, sizeof(gTankOut), "Sent #%d: DEPTH = %d mm, WATER_TYPE = %s", gTankSends, gTankDepthMM,
           gTankPolluted ? "FRESH_POLLUTED" : "FRESH_UNPOLLUTED");
  logMsg(gTankOut);
}

// Sub-task 5: send the mission messages once connected, then every
// MS5_TANK_RESEND_MS (no keyboard once USB is unplugged). On USB:
// Enter = send now, 20/30/40 = depth, p/u = polluted/unpolluted.
static void ms5TankTransmit() {
  if (!tankConnected()) return;
  if (gTankSends == 0 || millis() - gTankLastSend >= MS5_TANK_RESEND_MS) {
    tankSendMission();
    return;
  }
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch != '\n' && ch != '\r') {
      if (ch != ' ' && gTankLen < (int)sizeof(gTankLine) - 1) gTankLine[gTankLen++] = ch;
      continue;
    }
    gTankLine[gTankLen] = 0;
    if (gTankLen > 0 && gTankLine[0] >= '0' && gTankLine[0] <= '9') gTankDepthMM = atoi(gTankLine);
    else if (gTankLine[0] == 'p') gTankPolluted = true;
    else if (gTankLine[0] == 'u') gTankPolluted = false;
    gTankLen = 0;
    tankSendMission();
  }
}
#endif  // TANK_HW

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
// sensor sees one: stop, move to the middle of the open space before that
// column, sidestep to the nearest untried lane, face +x, and continue until
// past both columns. Turns in place only happen in those open spaces
// (>= MS5_SIDESTEP_GAP_M from any column), so the body never sweeps a column.
int gLane = 1;          // 0 low, 1 mid, 2 high
int gLaneTried = 0;     // bit per lane tried at the current column
float gBlockX = -1;     // right edge of the column that blocked us (-1 = none)

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

// Nearest lane not tried yet at this column, or -1.
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
    unsigned long used = millis() - start;
    if (used >= MS5_DETOUR_TIMEOUT_MS) {
      stopDrive();
      logMsg("MS5 7b: timeout");
      return;
    }
    int r = followLine(laneY(gLane), MS5_DETOUR_END_X, FOLLOW_PWM, true, MS5_DETOUR_TIMEOUT_MS - used);
    if (r == FOLLOW_DONE) {
      logPose("MS5 7b: past the obstacles");
      return;
    }
    if (r == FOLLOW_FAILED) {
      logMsg("MS5 7b: stopped (see the line above)");
      return;
    }

    // Blocked. The column ahead is the first one whose left edge is past us.
    float x = Enes100.getX();
    logPose("MS5 7b: obstacle encountered");
    float colLeft = (x < OBST_COL1_X - OBST_HALF_W) ? OBST_COL1_X - OBST_HALF_W : OBST_COL2_X - OBST_HALF_W;
    float colRight = colLeft + 2 * OBST_HALF_W;
    if (gBlockX >= 0 && colRight > gBlockX + 0.01) gLaneTried = 0;  // a new column: every lane is open again
    gBlockX = colRight;
    gLaneTried |= (1 << gLane);
    int next = nextLane();
    if (next < 0) {
      logMsg("MS5 7b: every lane blocked, giving up");
      return;
    }
    float sidestepX = colLeft - MS5_SIDESTEP_GAP_M;
    moveToX(sidestepX);
    gLane = next;
    logVal("MS5 7b: detour to lane y = ", laneY(gLane));
    driveTo(sidestepX, laneY(gLane), POS_TOL_M);
    turnTo(0.0);
  }
}

// Sub-task 8 (sim version): the Tank's front distance sensor while creeping
// at the west wall, next to the true distance from vision. Stops when the
// reading drops under SIM_OBSTACLE_THRESH_M (the threshold the nav uses).
unsigned long gSensePrintMs = 0;

static void ms5Sense() {
  logMsg("MS5 8: obstacle sensor vs true distance from vision");
  if (!waitForVision(VISION_START_MS)) {
    logMsg("MS5 8: no vision, aborting");
    return;
  }
  driveTo(MS5_SENSE_START_X, ARENA_MID_Y, POS_TOL_M);
  turnTo(PI);
  logVal("MS5 8: stop threshold m = ", SIM_OBSTACLE_THRESH_M);
  unsigned long start = millis();
  gSensePrintMs = 0;
  while (millis() - start < MS5_SENSE_TIMEOUT_MS) {
    float d = Tank.readDistanceSensor(SIM_OBSTACLE_SENSOR_ID);
    // Wall at x = 0, sensor SIM_SENSOR_FRONT_M ahead of the marker.
    float c = -cos(Enes100.getTheta());
    float truth = (c > 0.5) ? Enes100.getX() / c - SIM_SENSOR_FRONT_M : -1;
    bool stopNow = d > 0 && d < SIM_OBSTACLE_THRESH_M;
    if (stopNow || millis() - gSensePrintMs >= MS5_PRINT_MS) {
      gSensePrintMs = millis();
      if (d > 0) logVal("MS5 8: sensor m = ", d); else logMsg("MS5 8: sensor m = none - no echo beyond 1 m");
      logVal("MS5 8:  vision m = ", truth);
    }
    if (stopNow) {
      stopDrive();
      logMsg("MS5 8: obstacle -> stopped");
      return;
    }
    setDrive(MS5_SENSE_PWM, MS5_SENSE_PWM);
    delay(LOOP_MS);
  }
  stopDrive();
  logMsg("MS5 8: timeout");
}

#endif  // SIM

void runMs5() {
#if (MS5_TASK == MS5_ACTUATE) && !SIM
  ms5Actuate();
#elif TANK_HW && (MS5_TASK == MS5_RECEIVE)
  ms5TankReceive();
#elif TANK_HW && (MS5_TASK == MS5_TRANSMIT)
  ms5TankTransmit();
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
#elif MS5_TASK == MS5_SENSE
  ms5Sense();
#elif MS5_TASK == MS5_ACTUATE
  ms5ActuateOnce();
  logMsg("MS5 10: done");
#endif
  stopDrive();
  // Clear end marker: afterwards loop() just idles here with the motors off.
  logMsg("MS5 demo DONE - robot stopped");
#endif  // MS5_ACTUATE && !SIM
}

#endif  // MS5_TASK != MS5_NONE
