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
// (D2 is the front ultrasonic TRIG). PUMP_ML_PER_S is still unmeasured.
void pumpRunMs(unsigned long ms) {
  digitalWrite(PIN_PUMP_GATE, HIGH);
  delay(ms);
  digitalWrite(PIN_PUMP_GATE, LOW);
}

void pumpDispenseML(float ml) {
  pumpRunMs((unsigned long)(ml / PUMP_ML_PER_S * 1000.0));
}

// Median of the first n values (sorts v in place). Ignores outliers that
// would drag an average off.
static float medianOf(float *v, int n) {
  for (int i = 1; i < n; i++) {
    float key = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > key) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = key;
  }
  if (n % 2 == 1) return v[n / 2];
  return (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

// ---- Ultrasonic (from Adiel's ultrasonic_sensor.ino) ----
// Fixes: echo is round trip, so distance = 0.343 * t / 2; loop runs
// exactly US_SAMPLES times; skips invalid readings (pulseIn returns 0 on
// timeout); waits US_PING_GAP_MS between pings so old echoes off the bowl
// aren't read; uses the median. Returns -1 if no reading was valid.
// One HC-SR04 ping (arm or front sensor). Returns mm, or -1 if no echo.
// The caller must leave US_PING_GAP_MS between pings.
float pingMM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH, US_TIMEOUT_US);
  if (duration == 0) return -1;
  return US_MM_PER_US * duration / 2.0;
}

float readDistanceMM() {
  float readings[US_SAMPLES];
  int valid = 0;
  for (int i = 0; i < US_SAMPLES; i++) {
    float mm = pingMM(PIN_US_TRIG, PIN_US_ECHO);
    if (mm >= 0) readings[valid++] = mm;
    delay(US_PING_GAP_MS);
  }
  if (valid == 0) return -1;
  return medianOf(readings, valid);
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
// Added: COLOR_SAMPLES reads per channel, median of the valid ones.
// Returns 0 if every read timed out (sensor not responding).
static int readColorPW(int s2, int s3) {
  digitalWrite(PIN_COLOR_S2, s2);
  digitalWrite(PIN_COLOR_S3, s3);
  float pws[COLOR_SAMPLES];
  int valid = 0;
  for (int i = 0; i < COLOR_SAMPLES; i++) {
    unsigned long pw = pulseIn(PIN_COLOR_OUT, LOW, COLOR_TIMEOUT_US);
    if (pw > 0) pws[valid++] = pw;
  }
  delay(COLOR_SETTLE_MS);
  if (valid == 0) return 0;
  return (int)medianOf(pws, valid);
}

void readColorRaw(int *r, int *g, int *b) {
  *r = readColorPW(LOW, LOW);    // red filter
  *g = readColorPW(HIGH, HIGH);  // green filter
  *b = readColorPW(LOW, HIGH);   // blue filter
}

// Adiel's rule: polluted if ANY channel's pulse width is below the
// threshold (a strong non-black color). Threshold is uncalibrated.
// 0 = no reading on that channel, so it never counts as a color.
static bool pwSeesColor(int pw) { return pw > 0 && pw < COLOR_POLLUTED_THRESH; }

bool colorIsPolluted(int r, int g, int b) {
  return pwSeesColor(r) || pwSeesColor(g) || pwSeesColor(b);
}

bool isPolluted() {
  int r, g, b;
  readColorRaw(&r, &g, &b);
  if (r == 0 && g == 0 && b == 0) logMsg("Color reading FAILED (all channels), reporting unpolluted (not data)");
  else if (r == 0 || g == 0 || b == 0) logMsg("Color reading FAILED on some channels, deciding from the rest");
  return colorIsPolluted(r, g, b);
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
// The pool depth is always 20, 30, or 40 mm: snap to the nearest.
// A failed reading (negative) sends DEPTH_FALLBACK_MM.
int snapDepthMM(int rawMM) {
  if (rawMM < 0) return DEPTH_FALLBACK_MM;
  int d = constrain(rawMM, DEPTH_MIN_MM, DEPTH_MAX_MM);
  int steps = (d - DEPTH_MIN_MM + DEPTH_STEP_MM / 2) / DEPTH_STEP_MM;
  return DEPTH_MIN_MM + steps * DEPTH_STEP_MM;
}

// Each mission() call happens exactly once: graded demos hide prints,
// so results go out as single final mission() calls.
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
