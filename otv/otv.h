#pragma once
// Prototypes for every function shared across tabs.
// The simulator gets one concatenated file and may not auto-generate
// prototypes, so every cross-tab function is declared here.
// Simulator (JSCPP) limits: no String, no references, no Serial,
// no #if blocks (make_sim.ps1 preprocesses them), static locals reset
// on every call (use globals).
#include <Arduino.h>

// Set in setup() once Enes100.begin() returns.
extern bool gVisionStarted;

// drive.ino: the ONLY code that touches motors
void driveSetup();
void setDrive(int left, int right);  // -255..255, clamped to +/-MAX_PWM
void stopDrive();

// mission.ino
void missionSetup();
void armDown();
void armStow();
void pumpDispenseML(float ml);
float readDistanceMM();
int computeDepthMM();
void readColorRaw(int *r, int *g, int *b);  // pointers: the simulator can't parse references
bool isPolluted();
bool frontIsHose(float mm);  // front ultrasonic reading is the pump hose (ignored)
int snapDepthMM(int rawMM);  // nearest of 20/30/40; fallback if the reading failed
void runMission();
#if !SIM
bool colorIsPolluted(int r, int g, int b);  // shared by isPolluted() and bench test 5
float pingMM(int trigPin, int echoPin);     // one HC-SR04 ping, -1 = no echo
float readDistanceSamplesMM(int n);         // median of n arm pings (bench test 8)
void armMoveTo(int deg);                    // sweep arm to 0..180 (bench test 8)
int armGetAngle();
#endif
void pumpRunMs(unsigned long ms);  // pump on for a fixed time (bench test 2, MS5 10; sim: stub delay)

// nav.ino
void navSetup();
void logMsg(const char *msg);  // no String: the simulator doesn't have it
void logVal(const char *label, float value);
float wrapAngle(float a);
bool waitForConnection(unsigned long timeoutMs);
bool waitForVision(unsigned long timeoutMs);
bool simSpeedOk();  // SIM only: false if the simulator speed is not "instant"
bool obstacleAhead();
bool turnTo(float heading);
bool driveTo(float x, float y, float stopDistance);
bool hugWall(float xEnd);
// followLine() results
const int FOLLOW_DONE    = 0;  // reached xEnd
const int FOLLOW_BLOCKED = 1;  // front sensor saw an obstacle (stopped)
const int FOLLOW_FAILED  = 2;  // vision lost or timeout (stopped)
int followLine(float lineY, float xEnd, int pwm, bool watchObstacles, unsigned long timeoutMs);
bool moveToX(float xTarget);
unsigned long legTimeoutMs(float distM);
void logPose(const char *label);
void runFullRun();
void runRoute(bool toGoal);  // false = stop just before the limbo (MS5 sub-task 6)
// Route legs (runRoute runs them in this order; MS6 demos run some of them)
extern float gSiteX;  // mission site center, set by legMissionSite()
extern float gSiteY;
bool routeStart();     // wait for the marker; false = abort
void legMissionSite(); // start square -> mission site
void legBackup();      // reverse away from the pool
void legObstacles();   // landing zone -> past both obstacle columns (wall hug)
void legLimboAlign();  // down onto the limbo lane, just before the limbo
void legGoal();        // under the limbo to the goal

// tests.ino (robot only)
void runTests();

// ms5.ino (Milestone 5 demos, MS5_TASK != 0)
void runMs5();

// ms6.ino (Milestone 6 demos, MS6_TASK != 0)
void runMs6();
