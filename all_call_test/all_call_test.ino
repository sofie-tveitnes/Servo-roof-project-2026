#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <elapsedMillis.h>

#define SERVOS_PER_BOARD 16
#define NUM_BOARDS 5
#define TOTAL_SERVOS (SERVOS_PER_BOARD * NUM_BOARDS)
#define SERVO_MIN 200
#define SERVO_MAX 450
#define SERVO_FREQ 50
#define OSC_FREQ 27000000UL
#define SWEEP_STEP_MS 10
#define SWEEP_PAUSE_MS 500
#define ALLCALL_ADDR 0x70

Adafruit_PWMServoDriver boards[NUM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),
  Adafruit_PWMServoDriver(0x41),
  Adafruit_PWMServoDriver(0x42),
  Adafruit_PWMServoDriver(0x43),
  Adafruit_PWMServoDriver(0x44)
};

elapsedMillis sweepTimer;
elapsedMillis pauseTimer;

uint16_t currentPulse = SERVO_MAX;
bool sweepingDown = true;
bool inPause = false;

const uint32_t I2C_CLOCK_FREQ = 400000;

// ---------------------------------------------------------------------------
// Board initialization
// ---------------------------------------------------------------------------
void initBoards() {
  for (uint8_t i = 0; i < NUM_BOARDS; i++) {
    boards[i].begin();
    boards[i].setOscillatorFrequency(OSC_FREQ);
    boards[i].setPWMFreq(SERVO_FREQ);
    Wire.beginTransmission(0x40 + i);
    Wire.write(0x00);
    Wire.write(0x21);  // AI=1, SLEEP=0, ALLCALL=1
    Wire.endTransmission();
  }
}

// ---------------------------------------------------------------------------
// ALLCALL broadcast
// ---------------------------------------------------------------------------
void setAllServosPWM(uint16_t pulselen) {
  Wire.beginTransmission(ALLCALL_ADDR);
  Wire.write(0xFA);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(pulselen & 0xFF);
  Wire.write((pulselen >> 8) & 0x0F);
  Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Wire.begin();
  Wire.setClock(I2C_CLOCK_FREQ);
  initBoards();
  setAllServosPWM(currentPulse);
  sweepTimer = 0;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  if (inPause) {
    if (pauseTimer >= SWEEP_PAUSE_MS) {
      inPause = false;
      sweepTimer = 0;
    }
    return;
  }

  if (sweepTimer >= SWEEP_STEP_MS) {
    sweepTimer = 0;
    setAllServosPWM(currentPulse);

    if (sweepingDown) {
      if (currentPulse > SERVO_MIN) {
        currentPulse--;
      } else {
        sweepingDown = false;
        inPause = true;
        pauseTimer = 0;
      }
    } else {
      if (currentPulse < SERVO_MAX) {
        currentPulse++;
      } else {
        sweepingDown = true;
        inPause = true;
        pauseTimer = 0;
      }
    }
  }
}