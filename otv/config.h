#pragma once
// ============================================================
// OTV config: EVERY pin, limit, and tunable lives here.
// Other files use these names, never raw numbers.
// Anything marked [MEASURE] is a placeholder: measure it on the
// real robot/arena and update it here.
// ============================================================

// ---- Build flags (override on the compiler command line) ----
// SIM: 1 = Tank library: ENES100 browser simulator, or the physical
//          ENES100 Tank (Mega)
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
#define TEST_FRONT_US    7
#ifndef TEST_MODE
#define TEST_MODE TEST_NONE
#endif

#if (TEST_MODE < TEST_NONE) || (TEST_MODE > TEST_FRONT_US)
#error "TEST_MODE must be 0..7"
#endif
#if SIM && (TEST_MODE != TEST_NONE)
#error "Bench tests (TEST_MODE 1-7) are robot-only. Build with -DSIM=0."
#endif

// MS5_TASK: 0 = normal build. Milestone 5 demos (tools/otv.ps1: otv2..otv10).
// Numbers match the MS5 sub-tasks; 7a/7b are 71/72.
// Simulator only: 4, 5, 6, 71, 72, 8. Robot: 2, 3, 10 (these three also build
// with SIM=1 as simulator dry runs: otv2sim, otv3sim, otv10sim).
// Robot otv8/otv9 reuse bench tests 7 and 4; MS5_TASK 8 is the sim version of 8.
#define MS5_NONE      0
#define MS5_FORWARD   2   // robot: drive straight ~3.4 m, open loop (MS5_FORWARD_MS)
#define MS5_TURN      3   // robot: three timed 90 deg turns in place
#define MS5_RECEIVE   4   // sim: print x/y/theta from vision
#define MS5_TRANSMIT  5   // sim: send the mission messages
#define MS5_NAV       6   // sim: start -> mission site -> just before the limbo
#define MS5_WALLHUG   71  // sim 7a: full route (wall hug past the obstacles)
#define MS5_DETOUR    72  // sim 7b: drive at an obstacle, detect it, detour around it
#define MS5_ACTUATE   10  // robot: arm down, pump, arm stow (on Serial keypress; sim: once)
#define MS5_SENSE     8   // sim: front distance sensor vs vision while driving at a wall
#ifndef MS5_TASK
#define MS5_TASK MS5_NONE
#endif

#if MS5_TASK != MS5_NONE
#if TEST_MODE != TEST_NONE
#error "Set MS5_TASK or TEST_MODE, not both."
#endif
#if SIM && !((MS5_TASK == MS5_RECEIVE) || (MS5_TASK == MS5_TRANSMIT) || (MS5_TASK == MS5_NAV) || (MS5_TASK == MS5_WALLHUG) || (MS5_TASK == MS5_DETOUR) || (MS5_TASK == MS5_SENSE) || (MS5_TASK == MS5_FORWARD) || (MS5_TASK == MS5_TURN) || (MS5_TASK == MS5_ACTUATE))
#error "Unknown MS5_TASK."
#endif
#if !SIM && !((MS5_TASK == MS5_FORWARD) || (MS5_TASK == MS5_TURN) || (MS5_TASK == MS5_ACTUATE))
#error "This MS5_TASK is simulator only (SIM=1)."
#endif
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
#if SIM
const int MARKER_ID         = 54;  // ENES100 physical Tank (Tank-mode builds)
#else
const int MARKER_ID         = 80;  // our OTV
#endif
const int ROOM_NUMBER       = 1120;
// WiFi module TX/RX pins: ignored on the R4 (onboard WiFi). The physical
// Tank (Mega) uses 52/50 (ENES100 Physical Tank Guide). The browser
// simulator only accepts the 6-argument Enes100.begin().
const int VISION_WIFI_TX    = 52;
const int VISION_WIFI_RX    = 50;

// ---- Serial ----
const long SERIAL_BAUD = 115200;

#if !SIM
// ---- Pins (robot only: the browser simulator has no A0..A5) ----
// Final wiring (2026-11-12). D0/D1: reserved for USB serial. A5: unused.
const int PIN_FRONT_US_TRIG = 2; // Front HC-SR04 TRIG (obstacle stop; replaced the KY-032 IR)
const int PIN_FRONT_US_ECHO = A4;// Front HC-SR04 ECHO
const int PIN_LEFT_IN1    = 3;   // Left DRV8871 IN1
const int PIN_LEFT_IN2    = 5;   // Left DRV8871 IN2
const int PIN_RIGHT_IN1   = 6;   // Right DRV8871 IN1
const int PIN_RIGHT_IN2   = 9;   // Right DRV8871 IN2
const int PIN_US_TRIG     = 7;   // Arm HC-SR04 TRIG (depth)
const int PIN_US_ECHO     = 8;   // Arm HC-SR04 ECHO (depth)
const int PIN_PUMP_GATE   = 10;  // IRLZ44N gate, HIGH = pump on
const int PIN_SERVO1      = 11;  // Arm servo 1 (SG90)
const int PIN_SERVO2      = A2;  // Arm servo 2 (SG90)
const int PIN_COLOR_S0    = A0;  // TCS3200 S0
const int PIN_COLOR_S1    = A1;  // TCS3200 S1
const int PIN_COLOR_S2    = 12;  // TCS3200 S2
const int PIN_COLOR_S3    = 13;  // TCS3200 S3
const int PIN_COLOR_OUT   = 4;   // TCS3200 OUT
const int PIN_COLOR_LED   = A3;  // TCS3200 LED/OE

// ---- Front ultrasonic obstacle stop ----
// The HC-SR04 beam is ~30 deg wide, so in the wall-hug gap it can see the
// obstacle sides and the wall next to the lane. Keep this short.
const int FRONT_OBSTACLE_MM   = 150;  // [MEASURE] obstacle if a valid echo is closer than this
const int FRONT_HITS_NEEDED   = 2;    // consecutive close pings before stopping (filters ghost echoes)
#endif

// ---- Drive (DO NOT RAISE MAX_PWM without asking Micah) ----
// 6 V motors on a 9.6 V pack: cap at 60% (0.60 * 255 = 153).
const int  MAX_PWM      = 153;
const bool LEFT_INVERT  = false;  // [MEASURE] flip if left wheel runs backward
const bool RIGHT_INVERT = false;  // [MEASURE] flip if right wheel runs backward

// ---- Navigation tuning ----
const int   DRIVE_PWM          = 140;    // straight-line drive PWM
// followLine PWM (wall hug, descent to the limbo lane, last leg, 7b lanes).
// The hug gap leaves ~2.5 cm each side of the sim Tank, so it must track tightly.
#if SIM && !defined(ARDUINO_AVR_MEGA2560)
const int   FOLLOW_PWM         = 70;     // sim Tank: v = PWM/510 m/s, so 140 = 0.27 m/s (3.5x the robot) overshoots the line; tuned with otvsimcheck
#else
const int   FOLLOW_PWM         = DRIVE_PWM;  // robot (~7.5 cm/s) and physical Tank: already slow, and tested at DRIVE_PWM
#endif
const int   MIN_TURN_PWM       = 70;     // [MEASURE] lowest PWM that still turns in place
const float TURN_GAIN          = 120.0;  // turn PWM per rad of heading error
const float HEADING_GAIN       = 150.0;  // drive correction PWM per rad of heading error
const float TURN_TOL_RAD       = 0.08;   // turnTo done inside this error
const float REALIGN_RAD        = 0.50;   // driveTo stops and turns in place above this error
// followLine (wall hug, descent to the limbo lane, last leg, 7b lanes) steers
// back onto its line at most MAX_MERGE_RAD off +x, and only stops to turn in
// place above FOLLOW_REALIGN_RAD, so it never spins next to a wall or column.
const float MAX_MERGE_RAD       = 0.60;  // tune with otvsimcheck
const float FOLLOW_REALIGN_RAD  = 1.20;
const float POS_TOL_M          = 0.05;   // waypoint reached inside this distance
// driveTo slows down inside APPROACH_SLOW_M of its target, to no less than
// APPROACH_MIN_PWM. The sim Tank at DRIVE_PWM moves ~12 cm per loop pass and
// overshot the hug staging point by up to 9 cm (then spun into the wall).
const float APPROACH_SLOW_M    = 0.30;
#if SIM && !defined(ARDUINO_AVR_MEGA2560)
const int   APPROACH_MIN_PWM   = 50;     // browser sim (tuned with otvsimcheck)
#else
const int   APPROACH_MIN_PWM   = DRIVE_PWM;  // robot / physical Tank: already slow, no slowdown
#endif
const unsigned long LOOP_MS          = 20;
#if SIM
const unsigned long VISION_LOST_MS   = 6000;   // Tank testing: tolerate marker flicker near the arena edge
#else
const unsigned long VISION_LOST_MS   = 3000;   // give up a move after this long without the marker
#endif
const unsigned long WIFI_CONNECT_MS  = 30000;  // R4 begin() returns before WiFi connects (chosen, not specified)
const unsigned long VISION_START_MS  = 10000;  // give up the run if no marker at start (after WiFi is up)
const unsigned long MOVE_TIMEOUT_MS  = 15000;  // per turnTo call; driveTo adds travel time below
const float DRIVE_SPEED_MPS          = 0.075;  // [MEASURE] MS2 operating speed 7.54 cm/s; sizes driveTo timeouts
const unsigned long BACKUP_MS        = 800;    // reverse time after the mission

// ---- Footprint: everything the OTV must keep inside the arena ----
// Half sizes of the whole body (wheels included), centered on the marker.
#if SIM
// Browser simulator Tank as it is DRAWN (sim JS): body 0.15 x 0.12 m, but
// the treads are drawn 0.18 m long and out to +/-0.10 m. The sim only keeps
// the 0.15 x 0.12 body inside the walls, so the treads can poke out 4 cm.
const float OTV_HALF_WIDTH_M  = 0.10;
const float OTV_HALF_LENGTH_M = 0.09;
const float OTV_TURN_RADIUS_M = 0.135;  // half diagonal: sqrt(0.10^2 + 0.09^2)
#else
constexpr float OTV_HALF_WIDTH_M  = 0.12;   // [MEASURE] rules: OTV < 25 cm wide
const float OTV_HALF_LENGTH_M = 0.12;   // [MEASURE]
const float OTV_TURN_RADIUS_M = 0.17;   // [MEASURE] half diagonal: sqrt(w^2 + l^2) of the two above
// The wall-hug gap (y 1.75-2.0) is 25 cm wide.
static_assert(OTV_HALF_WIDTH_M < 0.125, "OTV too wide for the 25 cm obstacle-to-wall gap");
#endif
const float WALL_TURN_MARGIN_M = 0.07;  // clearance kept when turning in place near a wall (covers a few cm of stop overshoot)

// ---- Arena (vision frame: x 0..4 m, y 0..2 m) ----
const float ARENA_W           = 4.0;
const float ARENA_H           = 2.0;
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
const float WALL_HUG_GAIN     = 5.0;   // aim-back angle (rad) per m off the line (followLine); tuned with otvsimcheck
// Staging point for the wall hug: the OTV drives up the x = HUG_STAGE_X
// line (0.3 m right of the pool centers, clear of the top mission site),
// turns to +x at HUG_STAGE_Y, then hugWall merges onto WALL_HUG_Y before
// column 1 (x 1.4-1.6). HUG_STAGE_Y is as close to the hug line as a turn
// in place allows: its turning circle stays WALL_TURN_MARGIN_M off the wall.
// (Turning in place 12.5 cm from the wall swung the physical Tank into it.)
const float HUG_STAGE_X       = 0.8;   // [MEASURE]
#if SIM && defined(ARDUINO_AVR_MEGA2560)
const float HUG_STAGE_Y       = 1.65;  // physical ENES100 Tank (Mega): verified 2026-11-06; its size is unmeasured
#else
const float HUG_STAGE_Y       = ARENA_H - OTV_TURN_RADIUS_M - WALL_TURN_MARGIN_M;  // sim 1.795, robot 1.76
#endif
// Leave the hug line once the whole body is past column 2 (right edge 2.4),
// then followLine curves down to the limbo lane (no turn in place by the wall).
const float HUG_END_X         = 2.55;  // [MEASURE] 2.4 + half length + 6 cm
const float LIMBO_ALIGN_X     = 3.0;   // [MEASURE] be on the limbo lane by here (open zone 2.8-3.4)
const float LIMBO_LANE_Y      = 1.5;   // center of the limbo opening (doc)
const float GOAL_X            = 3.7;   // center of the goal zone 3.4-4.0 (doc)
const unsigned long HUG_TIMEOUT_MS = 40000;  // [MEASURE] 1.7 m hug at ~7 cm/s on the robot
// If the limbo rod hides the marker on the last leg, drive straight for the
// remaining distance (from the last seen x and DRIVE_SPEED_MPS), capped here.
const unsigned long LIMBO_BLIND_MAX_MS = 6500;   // chosen: ~0.5 m at 7.5 cm/s, so it can't reach the east wall from the limbo

// ---- Obstacle safety stop (MS2 nav flow: front sensor sees obstacle ->
// stop, back up, re-route with vision). Off for the pool approach, since
// the pool is straight ahead there.
const int   SIM_OBSTACLE_SENSOR_ID = 1;     // Tank distance sensor (enabled by default)
const float SIM_OBSTACLE_THRESH_M  = 0.40;  // obstacle if closer than this
// The Tank sensor also sees the arena walls, so a reading only counts if
// the echo point (vision pose + heading + distance) lands on an obstacle
// column. Obstacle geometry from the Arena Details doc (see Arena above).
const float SIM_SENSOR_FRONT_M = 0.075;  // sensor ahead of the marker (sim Tank is 0.15 m long)
const float OBST_COL1_X        = 1.5;    // column centers (doc)
const float OBST_COL2_X        = 2.3;
const float OBST_HALF_W        = 0.1;    // obstacles are 0.2 m wide (doc)
const float OBST_Y_MIN         = 0.25;   // rows span y 0.25-1.75 (doc)
const float OBST_Y_MAX         = 1.75;
const float OBST_HIT_MARGIN_M  = 0.05;   // slack for vision/heading error (chosen)
const unsigned long OBSTACLE_BACKUP_MS = 500;  // reverse time per hit (chosen, not specified)
const int   MAX_OBSTACLE_HITS      = 3;     // give up the move after this many hits (chosen)

// ---- Mission ----
const float SAMPLE_ML        = 30.0;   // requirement is >= 20 mL
const float PUMP_ML_PER_S    = 2.5;    // [MEASURE] unmeasured placeholder from Walter's sketch
const int   ARM_STOW_DEG     = 20;     // [MEASURE] arm folded flat over the chassis (Walter's sketch: 20)
const int   ARM_DOWN_DEG     = 160;    // [MEASURE] arm out over the water (Walter's sketch: 160)
const bool  SERVO2_MIRRORED  = true;   // servo 2 faces servo 1 (confirmed by Micah 2026-11-13)
const unsigned long SERVO_STEP_MS = 15;  // per 1 degree of sweep (limits 5 V rail spikes)
const unsigned long ARM_SETTLE_MS = 500; // after arm down, before reading (chosen, not specified)
const int   US_SAMPLES       = 25;     // ultrasonic readings per measurement (median is used)
const unsigned long US_TIMEOUT_US = 30000;  // pulseIn timeout
const unsigned long US_PING_GAP_MS = 60;    // HC-SR04 datasheet: >= 60 ms between pings (old echoes)
const float US_MM_PER_US     = 0.343;  // speed of sound; divide by 2 for round trip
const float SENSOR_HEIGHT_MM = 0;      // [MEASURE] sensor height above bowl bottom (owned by the depth teammate)
const int   COLOR_POLLUTED_THRESH = 400;  // [MEASURE] pulse width below this = color seen (uncalibrated)
const unsigned long COLOR_SETTLE_MS = 200; // between R/G/B reads (from Adiel's sketch)
const int   COLOR_SAMPLES    = 5;      // pulse widths per channel (median is used)
const unsigned long COLOR_TIMEOUT_US = 100000;  // pulseIn timeout; 0 = no reading, ignored
// Depth is always 20, 30, or 40 mm, so the reading snaps to the nearest one
// (any reading within +/-5 mm comes out exact). A failed reading sends the middle value.
const int   DEPTH_MIN_MM      = 20;
const int   DEPTH_MAX_MM      = 40;
const int   DEPTH_STEP_MM     = 10;
const int   DEPTH_FALLBACK_MM = 30;

// ---- Simulator stand-ins (no water in the sim) ----
const int  SIM_DEPTH_MM   = 30;
// Browser simulator speed check (setup): steering only works at the
// "instant" execution speed. 10 trivial loop passes take ~150 ms at
// instant, ~1800 ms at normal, ~3500 ms at slow (measured 2026-11-14).
const int  SIM_SPEED_CHECK_LOOPS = 10;
const unsigned long SIM_SPEED_MAX_MS = 800;
const bool SIM_POLLUTED   = false;
const unsigned long SIM_ARM_MS  = 500;   // stub delay (chosen, not specified)
const unsigned long SIM_PUMP_MS = 1000;  // stub delay (chosen, not specified)

// ---- Bench tests (robot only) ----
const unsigned long TEST_STEP_MS       = 1000;  // each drive test step
const unsigned long TEST_PUMP_CHECK_MS = 2000;  // pump test: quick on/off check
const unsigned long TEST_PUMP_PAUSE_MS = 5000;  // pump test: time to empty the measuring cup
const unsigned long TEST_PUMP_CAL_MS   = 10000; // pump test: fixed-time run; PUMP_ML_PER_S = mL / 10
const unsigned long TEST_GAP_MS        = 500;   // stop between drive steps
const unsigned long TEST_ARM_HOLD_MS   = 2000;
const unsigned long TEST_PRINT_MS      = 500;   // ultrasonic + color
const unsigned long TEST_VISION_MS     = 250;

// ---- MS5 demos (MS5_TASK, see top of file) ----
// The two #defines can be overridden from the command line
// (otv2 -Seconds N, otv3 -TurnMs N).
#ifndef MS5_FORWARD_MS
#define MS5_FORWARD_MS 45000  // 45 s at DRIVE_SPEED_MPS 7.5 cm/s = ~3.4 m (the MS5 distance; catching allowed). otv2 -Seconds N to adjust
#endif
#ifndef MS5_TURN90_MS
#define MS5_TURN90_MS 1900    // [MEASURE] spin-in-place time for 90 deg at DRIVE_PWM
#endif
#if SIM
const unsigned long MS5_START_DELAY_MS = 1000;  // sim dry runs of 2/3: nobody needs to step back
#else
const unsigned long MS5_START_DELAY_MS = 5000;  // robot demos: time to step back after power-on
#endif
const unsigned long MS5_TURN_PAUSE_MS  = 2000;  // between the three turns
const unsigned long MS5_PRINT_MS       = 500;   // receive demo: between pose prints
const int           MS5_RECEIVE_PRINTS = 20;    // receive demo: number of pose prints
const unsigned long MS5_ARM_HOLD_MS    = 1000;  // actuation demo: arm down before pumping
// Detour demo (7b): three lanes, one per obstacle row (rows span y 0.25-1.75).
const float MS5_LANE_LOW_Y   = 0.5;
const float MS5_LANE_MID_Y   = 1.0;
const float MS5_LANE_HIGH_Y  = 1.5;
const float MS5_DETOUR_START_X = 0.9;   // line up on a lane here (just before column 1 at x 1.4)
const float MS5_DETOUR_END_X   = 2.8;   // past both obstacle columns
// After a block, back up / creep to the middle of the open space before that
// column (x 1.1 before column 1, 1.9 between the columns) before sidestepping,
// so the turns in place never sweep into a column.
const float MS5_SIDESTEP_GAP_M = 0.3;
const unsigned long MS5_DETOUR_TIMEOUT_MS = 90000;

// Simulator dry runs of the robot demos (otv2sim, otv3sim, otv8sim, otv10sim).
// The sim Tank is faster than the OTV: v = PWM/510 m/s, w = 2*PWM/255 rad/s
// when spinning in place at +/-PWM (CLAUDE.md, read from the sim JS).
const float SIM_DRIVE_MPS           = DRIVE_PWM / 510.0;
const unsigned long MS5_SIM_TURN90_MS = (unsigned long)(1000.0 * (PI / 2) / (2.0 * DRIVE_PWM / 255.0));  // ~1430 ms
const float MS5_SIM_FORWARD_END_X   = 3.2;   // 2 dry run: stop before the limbo (x ~3.4), treads included
const float MS5_SENSE_START_X       = 1.2;   // 8 sim: open floor between the start squares and column 1
const int   MS5_SENSE_PWM           = 50;    // 8 sim: creep at ~0.1 m/s toward the west wall
const unsigned long MS5_SENSE_TIMEOUT_MS = 20000;
