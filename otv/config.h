#pragma once
// ============================================================
// OTV config: EVERY pin, limit, and tunable lives here.
// Other files use these names, never raw numbers.
// Anything marked [MEASURE] is a placeholder: measure it on the
// real robot/arena and update it here.
// ============================================================

// ---- Build flags (override on the compiler command line) ----
// SIM: 1 = ENES100 browser simulator (Tank library)
//      0 = real robot (DRV8871 drivers)
#ifndef SIM
#define SIM 1
#endif

// TEST_MODE: 0 = full mission. Robot-only bench tests:
#define TEST_NONE        0
#define TEST_DRIVE       1
#define TEST_PUMP        2
#define TEST_ARM         3
#define TEST_ULTRASONIC  4
#define TEST_COLOR       5
#define TEST_VISION      6
#define TEST_IR          7
#ifndef TEST_MODE
#define TEST_MODE TEST_NONE
#endif

#if (TEST_MODE < TEST_NONE) || (TEST_MODE > TEST_IR)
#error "TEST_MODE must be 0..7"
#endif
#if SIM && (TEST_MODE != TEST_NONE)
#error "Bench tests (TEST_MODE 1-7) are robot-only. Build with -DSIM=0."
#endif

#if SIM
#pragma message "OTV build target: SIMULATOR"
#else
#pragma message "OTV build target: ROBOT"
#endif

// ---- Vision system (ENES100 library v6.2.4) ----
// WATER comes from Enes100.h, which must be included before this file.
const char* const TEAM_NAME = "H2Woah!";
const byte TEAM_TYPE        = WATER;
const int MARKER_ID         = 80;
const int ROOM_NUMBER       = 1120;
// WiFi module TX/RX pins: ignored on the R4 (onboard WiFi), but the
// browser simulator only accepts the 6-argument Enes100.begin().
const int VISION_WIFI_TX    = 8;
const int VISION_WIFI_RX    = 9;

// ---- Serial ----
const long SERIAL_BAUD = 115200;

#if !SIM
// ---- Pins (robot only: the browser simulator has no A0..A5) ----
// D0/D1: reserved for USB serial. D4: unused (old MS2 map had color
// OUT here). A4: free.
const int PIN_IR          = 2;   // KY-032 IR obstacle sensor (digital)
const int PIN_LEFT_IN1    = 3;   // Left DRV8871 IN1
const int PIN_LEFT_IN2    = 5;   // Left DRV8871 IN2
const int PIN_RIGHT_IN1   = 6;   // Right DRV8871 IN1
const int PIN_RIGHT_IN2   = 9;   // Right DRV8871 IN2
const int PIN_US_TRIG     = 7;   // HC-SR04 TRIG
const int PIN_US_ECHO     = 8;   // HC-SR04 ECHO
const int PIN_PUMP_GATE   = 10;  // IRLZ44N gate, HIGH = pump on
const int PIN_SERVO1      = 11;  // Arm servo 1 (SG90)
const int PIN_SERVO2      = A2;  // Arm servo 2 (SG90)
const int PIN_COLOR_S0    = A0;  // TCS3200 S0
const int PIN_COLOR_S1    = A1;  // TCS3200 S1
const int PIN_COLOR_S2    = 12;  // TCS3200 S2
const int PIN_COLOR_S3    = 13;  // TCS3200 S3
const int PIN_COLOR_OUT   = A3;  // TCS3200 OUT. Follows Adiel's sketch, not MS2 (D4): confirm wiring
const int PIN_COLOR_LED   = A5;  // TCS3200 LED

// ---- IR obstacle sensor ----
const int IR_OBSTACLE_LEVEL = LOW;  // [MEASURE] assumed LOW = obstacle; needs bench verify
#endif

// ---- Drive (DO NOT RAISE MAX_PWM without asking Micah) ----
// 6 V motors on a 9.6 V pack: cap at 60% (0.60 * 255 = 153).
const int  MAX_PWM      = 153;
const bool LEFT_INVERT  = false;  // [MEASURE] flip if left wheel runs backward
const bool RIGHT_INVERT = false;  // [MEASURE] flip if right wheel runs backward

// ---- Navigation tuning ----
const int   DRIVE_PWM          = 140;    // straight-line drive PWM
const int   MIN_TURN_PWM       = 70;     // [MEASURE] lowest PWM that still turns in place
const float TURN_GAIN          = 120.0;  // turn PWM per rad of heading error
const float HEADING_GAIN       = 150.0;  // drive correction PWM per rad of heading error
const float TURN_TOL_RAD       = 0.08;   // turnTo done inside this error
const float REALIGN_RAD        = 0.50;   // driveTo stops and turns in place above this error
const float POS_TOL_M          = 0.05;   // waypoint reached inside this distance
const unsigned long LOOP_MS          = 20;
const unsigned long VISION_LOST_MS   = 3000;   // give up a move after this long without the marker
const unsigned long WIFI_CONNECT_MS  = 30000;  // R4 begin() returns before WiFi connects (chosen, not specified)
const unsigned long VISION_START_MS  = 10000;  // give up the run if no marker at start (after WiFi is up)
const unsigned long MOVE_TIMEOUT_MS  = 15000;  // per turnTo call; driveTo adds travel time below
const float DRIVE_SPEED_MPS          = 0.075;  // [MEASURE] MS2 operating speed 7.54 cm/s; sizes driveTo timeouts
const unsigned long BACKUP_MS        = 800;    // reverse time after the mission

// ---- Arena (vision frame: x 0..4 m, y 0..2 m) ----
// Source: official "Arena Details" doc (ENES100 course drive) diagram.
// Zones along x: landing/mission 0-0.8, obstacles 0.8-2.8, open 2.8-3.4,
// limbo/log at ~3.4, goal 3.4-4.0.
const float MISSION_TOP_X     = 0.5;   // square A center (doc)
const float MISSION_TOP_Y     = 1.5;   // square A center (doc)
const float MISSION_BOT_X     = 0.5;   // square B center (doc)
const float MISSION_BOT_Y     = 0.5;   // square B center (doc)
const float ARENA_MID_Y       = 1.0;   // A and B are 1.0 m apart, split at y = 1.0 (doc)
const float MISSION_STOP_DIST = 0.30;  // [MEASURE] tune so the OTV front ends within 150 mm of the pool edge
// Obstacles: 0.2 m wide x 0.5 m tall, columns centered at x = 1.5 and 2.3,
// rows spanning y = 0.25..1.75. That leaves a 0.25 m gap to each wall
// (MS2: OTV < 25 cm wide for the obstacle-to-wall gap).
// Plan: avoid all obstacles by hugging the top wall through that gap, then
// drop to the limbo lane. Limbo spans the top half (y 1.0-2.0) with 22 cm
// walls under each end, so the opening is y 1.22-1.78.
const float WALL_HUG_Y        = 1.875; // center of the top gap (y 1.75-2.0, doc)
const float WALL_HUG_GAIN     = 3.0;   // aim-back angle (rad) per m off the hug line (tune)
const float HUG_START_X       = 1.0;   // [MEASURE] join the hug line before column 1 (x 1.4-1.6)
const float HUG_END_X         = 2.7;   // [MEASURE] leave the hug line after column 2 (x 2.2-2.4)
const float LIMBO_ALIGN_X     = 3.0;   // [MEASURE] be on the limbo lane by here (open zone 2.8-3.4)
const float LIMBO_LANE_Y      = 1.5;   // center of the limbo opening (doc)
const float GOAL_X            = 3.7;   // center of the goal zone 3.4-4.0 (doc)
const unsigned long HUG_TIMEOUT_MS = 40000;  // [MEASURE] 1.7 m hug at ~7 cm/s on the robot

// ---- Obstacle safety stop (MS2 nav flow: front IR sees obstacle ->
// stop, back up, re-route with vision). Off for the pool approach, since
// the pool is straight ahead there.
const int   SIM_OBSTACLE_SENSOR_ID = 1;     // Tank distance sensor (enabled by default)
const float SIM_OBSTACLE_THRESH_M  = 0.40;  // obstacle if closer than this
const unsigned long OBSTACLE_BACKUP_MS = 500;  // reverse time per hit (chosen, not specified)
const int   MAX_OBSTACLE_HITS      = 3;     // give up the move after this many hits (chosen)

// ---- Mission ----
const float SAMPLE_ML        = 30.0;   // requirement is >= 20 mL
const float PUMP_ML_PER_S    = 2.5;    // [MEASURE] unmeasured placeholder from Walter's sketch
const int   ARM_STOW_DEG     = 20;     // [MEASURE] arm folded flat over the chassis (Walter's sketch: 20)
const int   ARM_DOWN_DEG     = 160;    // [MEASURE] arm out over the water (Walter's sketch: 160)
const bool  SERVO2_MIRRORED  = false;  // [MEASURE] true if servo 2 is mounted facing servo 1
const unsigned long SERVO_STEP_MS = 15;  // per 1 degree of sweep (limits 5 V rail spikes)
const unsigned long ARM_SETTLE_MS = 500; // after arm down, before reading (chosen, not specified)
const int   US_SAMPLES       = 25;     // ultrasonic readings averaged per measurement
const unsigned long US_TIMEOUT_US = 30000;  // pulseIn timeout
const float US_MM_PER_US     = 0.343;  // speed of sound; divide by 2 for round trip
const float SENSOR_HEIGHT_MM = 0;      // [MEASURE] sensor height above bowl bottom (owned by the depth teammate)
const int   COLOR_POLLUTED_THRESH = 400;  // [MEASURE] pulse width below this = color seen (uncalibrated)
const unsigned long COLOR_SETTLE_MS = 200; // between R/G/B reads (from Adiel's sketch)

// ---- Simulator stand-ins (no water in the sim) ----
const int  SIM_DEPTH_MM   = 30;
const bool SIM_POLLUTED   = false;
const unsigned long SIM_ARM_MS  = 500;   // stub delay (chosen, not specified)
const unsigned long SIM_PUMP_MS = 1000;  // stub delay (chosen, not specified)

// ---- Bench tests (robot only) ----
const unsigned long TEST_STEP_MS       = 1000;  // each drive test step
const unsigned long TEST_GAP_MS        = 500;   // stop between drive steps
const unsigned long TEST_ARM_HOLD_MS   = 2000;
const unsigned long TEST_PRINT_MS      = 500;   // ultrasonic + color
const unsigned long TEST_VISION_MS     = 250;
