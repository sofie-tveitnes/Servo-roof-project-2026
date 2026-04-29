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

#define ALLCALL_ADDR 0x70  // PCA9685 default ALLCALLADR

// Individual boards still needed for init
Adafruit_PWMServoDriver boards[NUM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),
  Adafruit_PWMServoDriver(0x41),
  Adafruit_PWMServoDriver(0x42),
  Adafruit_PWMServoDriver(0x43),
  Adafruit_PWMServoDriver(0x44)
};

// Broadcast driver — address 0x70, no begin()/init needed beyond boards init
Adafruit_PWMServoDriver allCall = Adafruit_PWMServoDriver(ALLCALL_ADDR);

elapsedMillis sweepTimer;
elapsedMillis pauseTimer;
uint16_t currentPulse = SERVO_MAX;
bool sweepingDown = true;
bool inPause = false;

const int BAUD_RATE = 9600;
const uint32_t I2C_CLOCK_FREQ = 400000;

// ---------------------------------------------------------------------------
// Board initialization
// ---------------------------------------------------------------------------
bool initBoards() {
  for (uint8_t i = 0; i < NUM_BOARDS; i++) {
    boards[i].begin();
    boards[i].setOscillatorFrequency(OSC_FREQ);
    boards[i].setPWMFreq(SERVO_FREQ);
    // MODE1: enable ALLCALL bit (bit 0) — should be on by default but set explicitly
    // The Adafruit library doesn't expose this directly, so write via Wire
    Wire.beginTransmission(0x40 + i);
    Wire.write(0x00);        // MODE1 register
    Wire.write(0x01);        // AI | ALLCALL enabled (auto-increment off, oscillator on)
    Wire.endTransmission();
  }
  return true;
}

// Broadcast one PWM value to ALL channels on ALL boards in a single I2C write
void setAllServosPWM(uint16_t pulselen) {
  // PCA9685 ALL_LED registers: 0xFA–0xFD
  Wire.beginTransmission(ALLCALL_ADDR);
  Wire.write(0xFA);                        // ALL_LED_ON_L
  Wire.write(0x00);                        // ON low byte
  Wire.write(0x00);                        // ON high byte
  Wire.write(pulselen & 0xFF);             // OFF low byte
  Wire.write((pulselen >> 8) & 0x0F);     // OFF high byte
  Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(BAUD_RATE);
  Wire.setClock(I2C_CLOCK_FREQ);
  Serial.println(F("Initializing 80-servo controller (ALLCALL mode)..."));
  if (!initBoards()) {
    Serial.println(F("ERROR: Board initialization failed."));
    while (1);
  }
  Serial.println(F("All boards initialized. Starting sweep."));
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

    // Single I2C transaction replaces 80 individual writes
    setAllServosPWM(currentPulse);

    if (sweepingDown) {
      if (currentPulse > SERVO_MIN) {
        currentPulse--;
      } else {
        Serial.println(F("Reached MIN. Reversing..."));
        sweepingDown = false;
        inPause = true;
        pauseTimer = 0;
      }
    } else {
      if (currentPulse < SERVO_MAX) {
        currentPulse++;
      } else {
        Serial.println(F("Reached MAX. Reversing..."));
        sweepingDown = true;
        inPause = true;
        pauseTimer = 0;
      }
    }
  }
}