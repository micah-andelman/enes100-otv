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
void runMission();

// nav.ino
void navSetup();
void logMsg(const char *msg);  // no String: the simulator doesn't have it
void logVal(const char *label, float value);
float wrapAngle(float a);
bool waitForVision(unsigned long timeoutMs);
bool obstacleAhead();
bool turnTo(float heading);
bool driveTo(float x, float y, float stopDistance);
bool passColumn(float columnX);
void runFullRun();

// tests.ino (robot only)
void runTests();
