#pragma once
// ============================================================
// OTV config: ALL pin numbers and hardware limits live here.
// Other files use these names, never raw numbers.
// Source: MS2 "Arduino Pin Assignments" slide.
// ============================================================

// ---- Pins ----
const int PIN_IR_OUT      = 2;
const int PIN_M1A_LEFT    = 3;
const int PIN_COLOR_OUT   = 4;
const int PIN_M1B_LEFT    = 5;
const int PIN_M2A_RIGHT   = 6;
const int PIN_US_TRIG     = 7;
const int PIN_US_ECHO     = 8;
const int PIN_M2B_RIGHT   = 9;
const int PIN_PUMP_GATE   = 10;
const int PIN_SERVO1      = 11;
const int PIN_COLOR_S2    = 12;
const int PIN_COLOR_S3    = 13;
const int PIN_COLOR_S0    = A0;
const int PIN_COLOR_S1    = A1;
const int PIN_SERVO2      = A2;

// ---- Hardware limits (DO NOT RAISE without asking Micah) ----
// Drive motors are rated 6 V but run on a 9.6 V pack.
// MS2 caps PWM "near 60%": 0.60 * 255 = 153.
const int MAX_PWM = 153;

// ---- Serial ----
const long SERIAL_BAUD = 115200;
