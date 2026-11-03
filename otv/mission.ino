// Mission: arm servos, pump (water sample), ultrasonic (depth),
// color sensor (pollution).
// Robot code is ported from the teammate sketches in teammates/
// (Walter: pump, servo. Adiel: ultrasonic, color), with known bugs fixed.

#if SIM
// ---------------- Simulator: no servos, pump, or water ----------------

void missionSetup() {}

void armDown() { delay(SIM_ARM_MS); }
void armStow() { delay(SIM_ARM_MS); }

void pumpDispenseML(float ml) { delay(SIM_PUMP_MS); }

float readDistanceMM() { return -1; }

int computeDepthMM() { return SIM_DEPTH_MM; }

void readColorRaw(int *r, int *g, int *b) {
  *r = 0;
  *g = 0;
  *b = 0;
}

bool isPolluted() { return SIM_POLLUTED; }

#else
// ---------------- Robot ----------------
#include <Servo.h>

// ---- Arm: 2x SG90 moving together (from Walter's Arduino_Servo.ino) ----
// Fixes: both servos (D11 + A2, not D4); no pinMode() on servo pins
// (it killed the servo output); moves only on command, no endless sweep.
Servo armServo1;
Servo armServo2;
int armAngle = ARM_STOW_DEG;

static void writeArm(int angle) {
  armServo1.write(angle);
  armServo2.write(SERVO2_MIRRORED ? 180 - angle : angle);
}

// Sweep 1 degree at a time to limit current spikes on the shared 5 V rail.
static void moveArmTo(int target) {
  while (armAngle != target) {
    armAngle += (target > armAngle) ? 1 : -1;
    writeArm(armAngle);
    delay(SERVO_STEP_MS);
  }
}

void armDown() { moveArmTo(ARM_DOWN_DEG); }
void armStow() { moveArmTo(ARM_STOW_DEG); }

// ---- Pump (from Walter's Arduino_Pump.ino) ----
// Fixes: D10 MOSFET gate (D6 is the right motor); no fake trigger on D2
// (D2 is the IR sensor). PUMP_ML_PER_S is still unmeasured.
void pumpDispenseML(float ml) {
  unsigned long runMs = (unsigned long)(ml / PUMP_ML_PER_S * 1000.0);
  digitalWrite(PIN_PUMP_GATE, HIGH);
  delay(runMs);
  digitalWrite(PIN_PUMP_GATE, LOW);
}

// ---- Ultrasonic (from Adiel's ultrasonic_sensor.ino) ----
// Fixes: echo is round trip, so distance = 0.343 * t / 2; loop runs
// exactly US_SAMPLES times; averages only valid readings (pulseIn
// returns 0 on timeout). Returns -1 if no reading was valid.
float readDistanceMM() {
  float total = 0;
  int valid = 0;
  for (int i = 0; i < US_SAMPLES; i++) {
    digitalWrite(PIN_US_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);
    unsigned long duration = pulseIn(PIN_US_ECHO, HIGH, US_TIMEOUT_US);
    if (duration == 0) continue;
    total += US_MM_PER_US * duration / 2.0;
    valid++;
  }
  if (valid == 0) return -1;
  return total / valid;
}

// Depth logic is owned by the depth teammate.
// Team method: depth = sensor height above bowl bottom - distance to the
// water surface. SENSOR_HEIGHT_MM must be measured once the arm is built.
// Returns -1 if the ultrasonic reading failed.
int computeDepthMM() {
  float distance = readDistanceMM();
  if (distance < 0) return -1;
  return (int)round(SENSOR_HEIGHT_MM - distance);
}

// ---- Color (from Adiel's Color_sensor.ino) ----
// Same frequency scaling (S0 HIGH, S1 LOW = 20%), same filter selection,
// same pulseIn(OUT, LOW) pulse-width reads with a settle delay after each.
static int readColorPW(int s2, int s3) {
  digitalWrite(PIN_COLOR_S2, s2);
  digitalWrite(PIN_COLOR_S3, s3);
  int pw = pulseIn(PIN_COLOR_OUT, LOW);
  delay(COLOR_SETTLE_MS);
  return pw;
}

void readColorRaw(int *r, int *g, int *b) {
  *r = readColorPW(LOW, LOW);    // red filter
  *g = readColorPW(HIGH, HIGH);  // green filter
  *b = readColorPW(LOW, HIGH);   // blue filter
}

// Adiel's rule: polluted if ANY channel's pulse width is below the
// threshold (a strong non-black color). Threshold is uncalibrated.
bool isPolluted() {
  int r, g, b;
  readColorRaw(&r, &g, &b);
  return (b < COLOR_POLLUTED_THRESH || r < COLOR_POLLUTED_THRESH || g < COLOR_POLLUTED_THRESH);
}

void missionSetup() {
  pinMode(PIN_PUMP_GATE, OUTPUT);
  digitalWrite(PIN_PUMP_GATE, LOW);

  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);

  pinMode(PIN_COLOR_S0, OUTPUT);
  pinMode(PIN_COLOR_S1, OUTPUT);
  pinMode(PIN_COLOR_S2, OUTPUT);
  pinMode(PIN_COLOR_S3, OUTPUT);
  pinMode(PIN_COLOR_OUT, INPUT);
  pinMode(PIN_COLOR_LED, OUTPUT);
  digitalWrite(PIN_COLOR_LED, HIGH);  // LED on while reading
  digitalWrite(PIN_COLOR_S0, HIGH);
  digitalWrite(PIN_COLOR_S1, LOW);

  // Start position is unknown at power-up, so jump straight to stow.
  armServo1.attach(PIN_SERVO1);
  armServo2.attach(PIN_SERVO2);
  armAngle = ARM_STOW_DEG;
  writeArm(armAngle);
}

#endif

// ---------------- Both targets ----------------
// Each mission() call happens exactly once: graded demos hide prints,
// so results go out as single final mission() calls.
void runMission() {
  logMsg("Mission: arm down");
  armDown();
  delay(ARM_SETTLE_MS);

  int depthMM = computeDepthMM();
  bool polluted = isPolluted();

  logMsg("Mission: pumping sample");
  pumpDispenseML(SAMPLE_ML);
  armStow();

  Enes100.mission(DEPTH, depthMM);
  Enes100.mission(WATER_TYPE, polluted ? FRESH_POLLUTED : FRESH_UNPOLLUTED);

  logVal("Mission sent: depth mm = ", depthMM);
  logMsg(polluted ? "Mission sent: polluted" : "Mission sent: unpolluted");
}
