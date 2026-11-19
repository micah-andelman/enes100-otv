// Milestone 6 (Systems Integration) demos. Build with -DMS6_TASK=n
// (tools/otv.ps1: otvsite, otvmission, otvobst). See config.h.
// MS6 objectives can be shown one at a time with unlimited attempts, so each
// demo runs only part of the full mission (the same route legs as
// runFullRun(), nav.ino) and stops. runMs6() is called every loop(); it runs
// once, then idles.

bool gMs6Done = false;  // global: the simulator resets static locals

#if MS6_TASK != MS6_NONE

// Pose after the OTV has stopped and vision has caught up.
float gChkX = 0;
float gChkY = 0;
float gChkT = 0;
void checkRead(const char *label) {
  stopDrive();
  delay(CHECK_SETTLE_MS);
  waitForVision(VISION_START_MS);
  gChkX = Enes100.getX();
  gChkY = Enes100.getY();
  gChkT = Enes100.getTheta();
  logPose(label);
}

// Tells apart two wiring/marker problems that the simulator can't have:
// 1. Turning reversed: a left (counter-clockwise) pulse must INCREASE theta.
// 2. Marker rotated on the OTV: driving forward must move the marker in the
//    direction theta points (travel angle - theta ~ 0).
void runHeadingCheck() {
  logMsg("CHECK: needs ~30 cm clear all round. Starting in 3 s");
  if (!routeStart()) return;
  delay(3000);
  checkRead("CHECK start");
  float t0 = gChkT;

  logMsg("CHECK 1: left turn pulse (theta should go UP)");
  setDrive(-CHECK_TURN_PWM, CHECK_TURN_PWM);
  delay(CHECK_TURN_MS);
  checkRead("CHECK after left turn");
  float dTurn = wrapAngle(gChkT - t0);
  logVal("CHECK 1 theta change (rad, + = correct) = ", dTurn);
  if (dTurn > 0.05) logMsg("CHECK 1 RESULT: turning OK");
  else if (dTurn < -0.05) logMsg("CHECK 1 RESULT: TURNING REVERSED (left and right motors swapped)");
  else logMsg("CHECK 1 RESULT: did not turn (more power or time needed)");

  logMsg("CHECK 2: forward pulse (should move the way theta points)");
  float x0 = gChkX;
  float y0 = gChkY;
  float th = gChkT;
  setDrive(DRIVE_PWM, DRIVE_PWM);
  delay(CHECK_DRIVE_MS);
  checkRead("CHECK after forward");
  float dx = gChkX - x0;
  float dy = gChkY - y0;
  float moved = sqrt(dx * dx + dy * dy);
  logVal("CHECK 2 moved (m) = ", moved);
  if (moved < 0.03) {
    logMsg("CHECK 2 RESULT: barely moved, can't tell");
  } else {
    float off = wrapAngle(atan2(dy, dx) - th);
    logVal("CHECK 2 travel minus theta (deg, 0 = correct) = ", off * 57.2958);
    if (fabs(off) < 0.35) logMsg("CHECK 2 RESULT: marker lines up with driving direction");
    else logMsg("CHECK 2 RESULT: MARKER ROTATED on the OTV by the angle above");
  }

  logMsg("CHECK 3: back to the start");
  setDrive(-DRIVE_PWM, -DRIVE_PWM);
  delay(CHECK_DRIVE_MS);
  checkRead("CHECK end");
}

void runMs6() {
  if (gMs6Done) {
    stopDrive();
    delay(LOOP_MS);
    return;
  }
  gMs6Done = true;

#if MS6_TASK == MS6_SITE
  // Nav 1 (within 150 mm of the mission site) + Mission I-III.
  logMsg("MS6: start -> mission site -> mission");
  if (routeStart()) {
    legMissionSite();
    float dx = gSiteX - Enes100.getX();
    float dy = gSiteY - Enes100.getY();
    logVal("MS6: marker to pool center (m) = ", sqrt(dx * dx + dy * dy));
    runMission();
  }
#elif MS6_TASK == MS6_MISSION
  // Mission I-III only: the OTV was placed at the pool by hand. No vision
  // pose needed; the results still go to the vision system.
  logMsg("MS6: mission only (OTV placed at the pool)");
  runMission();
#elif MS6_TASK == MS6_OBSTACLES
  // Nav 2 (past the obstacles) + Nav 3 (into the destination zone).
  // Start anywhere in the landing zone (x < 0.8), any heading.
  logMsg("MS6: landing zone -> past the obstacles -> goal");
  if (routeStart()) {
    legObstacles();
    legLimboAlign();
    legGoal();
  }
#elif MS6_TASK == MS6_CHECK
  runHeadingCheck();
#endif

  stopDrive();
  logMsg("MS6: done");
}

#endif  // MS6_TASK != MS6_NONE
