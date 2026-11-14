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
int snapDepthMM(int rawMM);  // nearest of 20/30/40; fallback if the reading failed
void runMission();
#if !SIM
bool colorIsPolluted(int r, int g, int b);  // shared by isPolluted() and bench test 5
float pingMM(int trigPin, int echoPin);     // one HC-SR04 ping, -1 = no echo
void pumpRunMs(unsigned long ms);           // pump on for a fixed time (bench test 2)
#endif

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
void logPose(const char *label);
void runFullRun();
void runRoute(bool toGoal);  // false = stop just before the limbo (MS5 sub-task 6)

// tests.ino (robot only)
void runTests();

// ms5.ino (Milestone 5 demos, MS5_TASK != 0)
void runMs5();
