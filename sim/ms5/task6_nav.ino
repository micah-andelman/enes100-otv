#include <cmath>
#include "Arduino.h"
#include "Enes100.h"
#include "Tank.h"

const int DRIVE_PWM = 140;
const int MIN_TURN_PWM = 70;
const float TURN_GAIN = 120.0;
const float HEADING_GAIN = 150.0;
const float TURN_TOL_RAD = 0.08;
const float POS_TOL_M = 0.05;
const float DRIVE_SPEED_MPS = 0.075;
const unsigned long BACKUP_MS = 800;
const int FRONT_OBSTACLE_MM = 150;
const float MISSION_STOP_DIST = 0.30;
const float HUG_STAGE_X = 0.8;
const float HUG_END_X = 2.55;
const float LIMBO_ALIGN_X = 3.0;
const unsigned long HUG_TIMEOUT_MS = 40000;
const float SAMPLE_ML = 30.0;
const float PUMP_ML_PER_S = 2.5;
const int ARM_STOW_DEG = 20;
const int ARM_DOWN_DEG = 96;
const unsigned long SERVO_STEP_MS = 15;
const unsigned long ARM_SETTLE_MS = 500;
const float SENSOR_HEIGHT_MM = 0;
const int COLOR_POLLUTED_THRESH = 400;

const char* const TEAM_NAME = "H2Woah!";
const byte TEAM_TYPE = WATER;
const int MARKER_ID = 54;
const int ROOM_NUMBER = 1120;
const int VISION_WIFI_TX = 52;
const int VISION_WIFI_RX = 50;
const long SERIAL_BAUD = 115200;
const int MAX_PWM = 153;
const bool LEFT_INVERT = false;
const bool RIGHT_INVERT = false;
const int FOLLOW_PWM = 70;
const float REALIGN_RAD = 0.50;
const float MAX_MERGE_RAD = 0.60;
const float FOLLOW_REALIGN_RAD = 1.20;
const float APPROACH_SLOW_M = 0.30;
const int APPROACH_MIN_PWM = 50;
const unsigned long LOOP_MS = 20;
const unsigned long VISION_LOST_MS = 6000;
const unsigned long WIFI_CONNECT_MS = 30000;
const unsigned long VISION_START_MS = 10000;
const unsigned long MOVE_TIMEOUT_MS = 15000;
const float OTV_HALF_WIDTH_M = 0.10;
const float OTV_HALF_LENGTH_M = 0.09;
const float OTV_TURN_RADIUS_M = 0.135;
const float WALL_TURN_MARGIN_M = 0.07;
const float ARENA_W = 4.0;
const float ARENA_H = 2.0;
const float MISSION_TOP_X = 0.5;
const float MISSION_TOP_Y = 1.5;
const float MISSION_BOT_X = 0.5;
const float MISSION_BOT_Y = 0.5;
const float ARENA_MID_Y = 1.0;
const float WALL_HUG_Y = 1.875;
const float WALL_HUG_GAIN = 5.0;
const float HUG_STAGE_Y = ARENA_H - OTV_TURN_RADIUS_M - WALL_TURN_MARGIN_M;
const float LIMBO_LANE_Y = 1.5;
const float GOAL_X = 3.7;
const unsigned long LIMBO_BLIND_MAX_MS = 6500;
const int SIM_OBSTACLE_SENSOR_ID = 1;
const float SIM_OBSTACLE_THRESH_M = 0.40;
const float SIM_SENSOR_FRONT_M = 0.075;
const float OBST_COL1_X = 1.5;
const float OBST_COL2_X = 2.3;
const float OBST_HALF_W = 0.1;
const float OBST_Y_MIN = 0.25;
const float OBST_Y_MAX = 1.75;
const float OBST_HIT_MARGIN_M = 0.05;
const unsigned long OBSTACLE_BACKUP_MS = 500;
const int MAX_OBSTACLE_HITS = 3;
const bool SERVO2_MIRRORED = true;
const int US_SAMPLES = 25;
const unsigned long US_TIMEOUT_US = 30000;
const unsigned long US_PING_GAP_MS = 60;
const float US_MM_PER_US = 0.343;
const unsigned long COLOR_SETTLE_MS = 200;
const int COLOR_SAMPLES = 5;
const unsigned long COLOR_TIMEOUT_US = 100000;
const int DEPTH_MIN_MM = 20;
const int DEPTH_MAX_MM = 40;
const int DEPTH_STEP_MM = 10;
const int DEPTH_FALLBACK_MM = 30;
const int SIM_DEPTH_MM = 30;
const int SIM_SPEED_CHECK_LOOPS = 10;
const unsigned long SIM_SPEED_MAX_MS = 800;
const bool SIM_POLLUTED = false;
const unsigned long SIM_ARM_MS = 500;
const unsigned long SIM_PUMP_MS = 1000;
const unsigned long TEST_STEP_MS = 1000;
const unsigned long TEST_PUMP_CHECK_MS = 2000;
const unsigned long TEST_PUMP_PAUSE_MS = 5000;
const unsigned long TEST_PUMP_CAL_MS = 10000;
const unsigned long TEST_GAP_MS = 500;
const unsigned long TEST_ARM_HOLD_MS = 2000;
const unsigned long TEST_PRINT_MS = 500;
const unsigned long TEST_VISION_MS = 250;
const unsigned long CAL_PRINT_MS = 1000;
const int CAL_PUMP_MAX_S = 30;
const int CAL_LIVE_SAMPLES = 5;
const unsigned long MS5_START_DELAY_MS = 1000;
const unsigned long MS5_TURN_PAUSE_MS = 2000;
const unsigned long MS5_PRINT_MS = 500;
const int MS5_RECEIVE_PRINTS = 20;
const unsigned long MS5_TANK_RESEND_MS = 15000;
const unsigned long MS5_ARM_HOLD_MS = 1000;
const float MS5_LANE_LOW_Y = 0.5;
const float MS5_LANE_MID_Y = 1.0;
const float MS5_LANE_HIGH_Y = 1.5;
const float MS5_DETOUR_START_X = 0.9;
const float MS5_DETOUR_END_X = 2.8;
const float MS5_SIDESTEP_GAP_M = 0.3;
const unsigned long MS5_DETOUR_TIMEOUT_MS = 90000;
const float SIM_DRIVE_MPS = DRIVE_PWM / 510.0;
const unsigned long MS5_SIM_TURN90_MS = (unsigned long)(1000.0 * (PI / 2) / (2.0 * DRIVE_PWM / 255.0));
const float MS5_SIM_FORWARD_END_X = 3.2;
const float MS5_SENSE_START_X = 1.2;
const int MS5_SENSE_PWM = 50;
const unsigned long MS5_SENSE_TIMEOUT_MS = 20000;
extern bool gVisionStarted;
void driveSetup();
void setDrive(int left, int right);
void stopDrive();
void missionSetup();
void armDown();
void armStow();
void pumpDispenseML(float ml);
float readDistanceMM();
int computeDepthMM();
void readColorRaw(int *r, int *g, int *b);
bool isPolluted();
int snapDepthMM(int rawMM);
void runMission();
void pumpRunMs(unsigned long ms);
void navSetup();
void logMsg(const char *msg);
void logVal(const char *label, float value);
float wrapAngle(float a);
bool waitForConnection(unsigned long timeoutMs);
bool waitForVision(unsigned long timeoutMs);
bool simSpeedOk();
bool obstacleAhead();
bool turnTo(float heading);
bool driveTo(float x, float y, float stopDistance);
bool hugWall(float xEnd);
const int FOLLOW_DONE = 0;
const int FOLLOW_BLOCKED = 1;
const int FOLLOW_FAILED = 2;
int followLine(float lineY, float xEnd, int pwm, bool watchObstacles, unsigned long timeoutMs);
bool moveToX(float xTarget);
unsigned long legTimeoutMs(float distM);
void logPose(const char *label);
void runFullRun();
void runRoute(bool toGoal);
extern float gSiteX;
extern float gSiteY;
bool routeStart();
void legMissionSite();
void legBackup();
void legObstacles();
void legLimboAlign();
void legGoal();
void runTests();
void runMs5();
void runMs6();
bool gVisionStarted = false;
bool gRunDone = false;
void setup() {
  driveSetup();
  missionSetup();
  navSetup();
  Enes100.begin(TEAM_NAME, TEAM_TYPE, MARKER_ID, ROOM_NUMBER, VISION_WIFI_TX, VISION_WIFI_RX);
  gVisionStarted = true;
  logMsg("Connecting to the vision system...");
  if (!waitForConnection(WIFI_CONNECT_MS)) logMsg("Vision system not connected, continuing");
  else logMsg("Vision system connected");
  logMsg("Target: SIMULATOR");
  if (!simSpeedOk()) {
    logMsg("STOPPED: simulator speed is too slow.");
    logMsg("1. Click Stop (top-left square).");
    logMsg("2. Click the speed button (2nd from the left) until its tooltip says instant.");
    logMsg("3. Click Run and then Continue again.");
    while (true) {
      stopDrive();
      delay(1000);
    }
  }
}
void loop() {
  runMs5();
}
void driveSetup() {
  Tank.begin();
  stopDrive();
}
void setDrive(int left, int right) {
  Tank.setLeftMotorPWM(constrain(left, -MAX_PWM, MAX_PWM));
  Tank.setRightMotorPWM(constrain(right, -MAX_PWM, MAX_PWM));
}
void stopDrive() {
  Tank.turnOffMotors();
}
void missionSetup() {}
void armDown() { delay(SIM_ARM_MS); }
void armStow() { delay(SIM_ARM_MS); }
void pumpDispenseML(float ml) { delay(SIM_PUMP_MS); }
void pumpRunMs(unsigned long ms) { delay(SIM_PUMP_MS); }
float readDistanceMM() { return -1; }
int computeDepthMM() { return SIM_DEPTH_MM; }
void readColorRaw(int *r, int *g, int *b) {
  *r = 0;
  *g = 0;
  *b = 0;
}
bool isPolluted() { return SIM_POLLUTED; }
int snapDepthMM(int rawMM) {
  if (rawMM < 0) return DEPTH_FALLBACK_MM;
  int d = constrain(rawMM, DEPTH_MIN_MM, DEPTH_MAX_MM);
  int steps = (d - DEPTH_MIN_MM + DEPTH_STEP_MM / 2) / DEPTH_STEP_MM;
  return DEPTH_MIN_MM + steps * DEPTH_STEP_MM;
}
void runMission() {
  logMsg("Mission: arm down");
  armDown();
  delay(ARM_SETTLE_MS);
  int rawDepthMM = computeDepthMM();
  if (rawDepthMM < 0) logMsg("Mission: depth reading FAILED, sending fallback (not data)");
  else logVal("Mission: raw depth mm = ", rawDepthMM);
  int depthMM = snapDepthMM(rawDepthMM);
  bool polluted = isPolluted();
  logMsg("Mission: pumping sample");
  pumpDispenseML(SAMPLE_ML);
  armStow();
  Enes100.mission(DEPTH, depthMM);
  Enes100.mission(WATER_TYPE, polluted ? FRESH_POLLUTED : FRESH_UNPOLLUTED);
  logVal("Mission sent: depth mm = ", depthMM);
  logMsg(polluted ? "Mission sent: polluted" : "Mission sent: unpolluted");
}
float gLastX = 0;
void navSetup() {
}
void logMsg(const char *msg) {
  if (gVisionStarted) Enes100.println(msg);
}
void logVal(const char *label, float value) {
  if (gVisionStarted) {
    Enes100.print(label);
    Enes100.println(value);
  }
}
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
int gSpeedLoops = 0;
bool simSpeedOk() {
  unsigned long t0 = millis();
  gSpeedLoops = 0;
  while (gSpeedLoops < SIM_SPEED_CHECK_LOOPS) {
    gSpeedLoops++;
  }
  return (millis() - t0) <= SIM_SPEED_MAX_MS;
}
float wrapAngle(float a) {
  while (a > PI) a -= TWO_PI;
  while (a < -PI) a += TWO_PI;
  return a;
}
bool waitForConnection(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!Enes100.isConnected()) {
    if (millis() - start >= timeoutMs) return false;
    delay(LOOP_MS);
  }
  return true;
}
bool waitForVision(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!Enes100.isVisible()) {
    stopDrive();
    if (millis() - start >= timeoutMs) return false;
    delay(LOOP_MS);
  }
  return true;
}
static bool inObstacleField(float x, float y) {
  if (y < OBST_Y_MIN - OBST_HIT_MARGIN_M || y > OBST_Y_MAX + OBST_HIT_MARGIN_M) return false;
  if (fabs(x - OBST_COL1_X) <= OBST_HALF_W + OBST_HIT_MARGIN_M) return true;
  if (fabs(x - OBST_COL2_X) <= OBST_HALF_W + OBST_HIT_MARGIN_M) return true;
  return false;
}
bool obstacleAhead() {
  float d = Tank.readDistanceSensor(SIM_OBSTACLE_SENSOR_ID);
  if (d <= 0 || d >= SIM_OBSTACLE_THRESH_M) return false;
  if (!Enes100.isVisible()) return true;
  float th = Enes100.getTheta();
  float r = SIM_SENSOR_FRONT_M + d;
  return inObstacleField(Enes100.getX() + r * cos(th), Enes100.getY() + r * sin(th));
}
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
    int pwm = constrain((int)(DRIVE_PWM * (dist - stopDistance) / APPROACH_SLOW_M), APPROACH_MIN_PWM, DRIVE_PWM);
    int correction = (int)(HEADING_GAIN * err);
    setDrive(pwm - correction, pwm + correction);
    delay(LOOP_MS);
  }
}
unsigned long legTimeoutMs(float distM) {
  return MOVE_TIMEOUT_MS + (unsigned long)(1000.0 * fabs(distM) / DRIVE_SPEED_MPS);
}
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
    int correction = (int)(HEADING_GAIN * wrapAngle(0.0 - Enes100.getTheta()));
    int pwm = dx > 0 ? DRIVE_PWM : -DRIVE_PWM;
    setDrive(pwm - correction, pwm + correction);
    delay(LOOP_MS);
  }
}
static void finishBlindIfVisionLost() {
  if (Enes100.isVisible()) return;
  float remaining = GOAL_X - gLastX;
  if (remaining <= 0) return;
  unsigned long ms = (unsigned long)(1000.0 * remaining / DRIVE_SPEED_MPS);
  if (ms > LIMBO_BLIND_MAX_MS) ms = LIMBO_BLIND_MAX_MS;
  logVal("Vision lost on last leg, driving blind ms = ", ms);
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(ms);
  stopDrive();
}
void runFullRun() { runRoute(true); }
void runRoute(bool toGoal) {
  if (!routeStart()) return;
  legMissionSite();
  runMission();
  legBackup();
  legObstacles();
  legLimboAlign();
  if (!toGoal) {
    stopDrive();
    logMsg("Stopped before the limbo");
    return;
  }
  legGoal();
  logMsg("Run complete");
}
float gSiteX = MISSION_TOP_X;
float gSiteY = MISSION_TOP_Y;
bool routeStart() {
  logMsg("Run start");
  if (!waitForVision(VISION_START_MS)) {
    stopDrive();
    logMsg("No vision at start, aborting run");
    return false;
  }
  return true;
}
void legMissionSite() {
  bool startTop = Enes100.getY() > ARENA_MID_Y;
  gSiteX = startTop ? MISSION_BOT_X : MISSION_TOP_X;
  gSiteY = startTop ? MISSION_BOT_Y : MISSION_TOP_Y;
  logMsg(startTop ? "Started in top square, mission site is bottom"
                  : "Started in bottom square, mission site is top");
  logPose("Pose at start");
  if (!driveTo(gSiteX, gSiteY, MISSION_STOP_DIST)) {
    logMsg("Mission site approach failed, running mission anyway");
  }
  stopDrive();
  logPose("Pose at mission site");
}
void legBackup() {
  setDrive(-DRIVE_PWM, -DRIVE_PWM);
  delay(BACKUP_MS);
  stopDrive();
  logPose("Pose after backup");
}
void legObstacles() {
  logMsg("Wall hug start");
  if (!driveTo(HUG_STAGE_X, ARENA_MID_Y, POS_TOL_M)) logMsg("Hug approach failed, continuing");
  if (!driveTo(HUG_STAGE_X, HUG_STAGE_Y, POS_TOL_M)) logMsg("Hug staging approach failed, continuing");
  logPose("Pose at hug start");
  turnTo(0.0);
  logPose("Pose after turn to +x");
  if (!hugWall(HUG_END_X)) logMsg("Wall hug failed, continuing");
  logPose("Pose after wall hug");
}
void legLimboAlign() {
  logMsg("Limbo");
  followLine(LIMBO_LANE_Y, LIMBO_ALIGN_X, FOLLOW_PWM, false, legTimeoutMs(LIMBO_ALIGN_X - HUG_END_X + WALL_HUG_Y - LIMBO_LANE_Y));
  logPose("Pose at limbo align");
}
void legGoal() {
  if (followLine(LIMBO_LANE_Y, GOAL_X, FOLLOW_PWM, false, legTimeoutMs(GOAL_X - LIMBO_ALIGN_X)) != FOLLOW_DONE) {
    finishBlindIfVisionLost();
  }
  stopDrive();
  logPose("Pose at end");
}
bool gMs5Done = false;
static void ms5StartCountdown() {
  logVal("MS5 demo starts in ms: ", MS5_START_DELAY_MS);
  delay(MS5_START_DELAY_MS);
}
static bool ms5SimLineUp() {
  if (!waitForVision(VISION_START_MS)) {
    logMsg("MS5: no vision, aborting");
    return false;
  }
  return turnTo(0.0);
}
static void ms5Forward() {
  if (!ms5SimLineUp()) return;
  float x0 = Enes100.getX();
  float y0 = Enes100.getY();
  unsigned long ms = (unsigned long)(1000.0 * (MS5_SIM_FORWARD_END_X - x0) / SIM_DRIVE_MPS);
  ms5StartCountdown();
  logVal("MS5 2: driving forward for ms = ", ms);
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(ms);
  stopDrive();
  logMsg("MS5 2: stopped");
  delay(MS5_PRINT_MS);
  float dx = Enes100.getX() - x0;
  float dy = Enes100.getY() - y0;
  logPose("MS5 2: end pose");
  logVal("MS5 2: distance m = ", sqrt(dx * dx + dy * dy));
  logVal("MS5 2: sideways drift m = ", dy);
  logMsg("MS5 2: done");
}
static void ms5Turn() {
  if (!ms5SimLineUp()) return;
  unsigned long turnMs = MS5_SIM_TURN90_MS;
  ms5StartCountdown();
  for (int i = 1; i <= 3; i++) {
    float th0 = Enes100.getTheta();
    logVal("MS5 3: turn ", i);
    setDrive(-DRIVE_PWM, DRIVE_PWM);
    delay(turnMs);
    stopDrive();
    delay(MS5_TURN_PAUSE_MS);
    logVal("MS5 3:   turned deg = ", wrapAngle(Enes100.getTheta() - th0) * 180.0 / PI);
  }
  logMsg("MS5 3: done");
}
static void ms5ActuateOnce() {
  logMsg("MS5 10: arm down");
  armDown();
  delay(MS5_ARM_HOLD_MS);
  logMsg("MS5 10: pump on");
  pumpRunMs(TEST_PUMP_CHECK_MS);
  logMsg("MS5 10: pump off then arm stow");
  armStow();
}
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
static void ms5Transmit() {
  logMsg("MS5 5: sending mission messages");
  Enes100.mission(DEPTH, SIM_DEPTH_MM);
  Enes100.mission(WATER_TYPE, SIM_POLLUTED ? FRESH_POLLUTED : FRESH_UNPOLLUTED);
  logVal("MS5 5: sent depth mm = ", SIM_DEPTH_MM);
  logMsg(SIM_POLLUTED ? "MS5 5: sent polluted" : "MS5 5: sent unpolluted");
}
int gLane = 1;
int gLaneTried = 0;
float gBlockX = -1;
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
    float x = Enes100.getX();
    logPose("MS5 7b: obstacle encountered");
    float colLeft = (x < OBST_COL1_X - OBST_HALF_W) ? OBST_COL1_X - OBST_HALF_W : OBST_COL2_X - OBST_HALF_W;
    float colRight = colLeft + 2 * OBST_HALF_W;
    if (gBlockX >= 0 && colRight > gBlockX + 0.01) gLaneTried = 0;
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
void runMs5() {
  if (gMs5Done) {
    stopDrive();
    delay(LOOP_MS);
    return;
  }
  gMs5Done = true;
  runRoute(false);
  stopDrive();
  logMsg("MS5 demo DONE - robot stopped");
}
bool gMs6Done = false;
