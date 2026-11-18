// Milestone 6 (Systems Integration) demos. Build with -DMS6_TASK=n
// (tools/otv.ps1: otvsite, otvmission, otvobst). See config.h.
// MS6 objectives can be shown one at a time with unlimited attempts, so each
// demo runs only part of the full mission (the same route legs as
// runFullRun(), nav.ino) and stops. runMs6() is called every loop(); it runs
// once, then idles.

bool gMs6Done = false;  // global: the simulator resets static locals

#if MS6_TASK != MS6_NONE

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
#endif

  stopDrive();
  logMsg("MS6: done");
}

#endif  // MS6_TASK != MS6_NONE
