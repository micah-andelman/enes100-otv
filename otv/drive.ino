// Drive layer: the ONLY code that touches motor pins or Tank.
// Navigation calls setDrive()/stopDrive() and nothing else, so this
// file is the single switch point between simulator and robot.
// PWM is -255..255 per side (+ = forward), clamped to +/-MAX_PWM.

#if SIM
// ---------------- Simulator: Tank library ----------------

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

#else
// ---------------- Robot: one DRV8871 per motor ----------------
// Forward = PWM on IN1, 0 on IN2. Reverse = the opposite.
// Stop = both 0 (coast).
//
// analogWrite is used on BOTH pins in every state, including writing 0.
// On the UNO R4, mixing digitalWrite and analogWrite on the same pin can
// leave it stuck in PWM mode, so these pins never see digitalWrite.
static void writeMotor(int in1, int in2, int pwm, bool invert) {
  if (invert) pwm = -pwm;
  if (pwm > 0) {
    analogWrite(in1, pwm);
    analogWrite(in2, 0);
  } else if (pwm < 0) {
    analogWrite(in1, 0);
    analogWrite(in2, -pwm);
  } else {
    analogWrite(in1, 0);
    analogWrite(in2, 0);
  }
}

void driveSetup() {
  pinMode(PIN_LEFT_IN1, OUTPUT);
  pinMode(PIN_LEFT_IN2, OUTPUT);
  pinMode(PIN_RIGHT_IN1, OUTPUT);
  pinMode(PIN_RIGHT_IN2, OUTPUT);
  stopDrive();
}

void setDrive(int left, int right) {
  writeMotor(PIN_LEFT_IN1, PIN_LEFT_IN2, constrain(left, -MAX_PWM, MAX_PWM), LEFT_INVERT);
  writeMotor(PIN_RIGHT_IN1, PIN_RIGHT_IN2, constrain(right, -MAX_PWM, MAX_PWM), RIGHT_INVERT);
}

void stopDrive() {
  setDrive(0, 0);
}

#endif
