//imports
#include <Wire.h> //i2c com
#include <Adafruit_PWMServoDriver.h> //servo driver
#include <elapsedMillis.h> //convenient timer

//definitions
#define SERVOS_PER_BOARD 16
#define NUM_BOARDS 5
#define TOTAL_SERVOS (SERVOS_PER_BOARD * NUM_BOARDS)
#define SERVO_MIN 290
#define SERVO_MAX 460
#define SERVO_FREQ 50
#define OSC_FREQ 27000000UL
#define SWEEP_STEP_MS 8
#define SWEEP_PAUSE_MIN_MS 500   // pause duration at bottom (SERVO_MIN)
#define SWEEP_PAUSE_MAX_MS 1600   // pause duration at top (SERVO_MAX)
#define ALLCALL_ADDR 0x70

#define ACTIVE_DURATION  120000UL  // 2 min in ms
#define IDLE_DURATION    180000UL  // 3 min in ms

//servo drivers array with 5 boards
Adafruit_PWMServoDriver boards[NUM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),
  Adafruit_PWMServoDriver(0x41),
  Adafruit_PWMServoDriver(0x42),
  Adafruit_PWMServoDriver(0x43),
  Adafruit_PWMServoDriver(0x44)
};

//variables definitions
elapsedMillis sweepTimer;
elapsedMillis pauseTimer;
elapsedMillis cycleTimer;

uint16_t currentPulse = SERVO_MAX;
bool sweepingDown = true;
bool inPause = false;
bool motorsActive = true;
bool windingDown = false;
bool pauseAtMin = false;         // tracks which end triggered the pause

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
void setAllServosPWM(uint16_t pulseLength) {
  Wire.beginTransmission(ALLCALL_ADDR);
  Wire.write(0xFA);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(pulseLength & 0xFF);
  Wire.write((pulseLength >> 8) & 0x0F);
  Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  delay(2000);                       // 2 second startup delay
  Wire.begin();
  Wire.setClock(I2C_CLOCK_FREQ);
  initBoards();
  setAllServosPWM(currentPulse);
  sweepTimer = 0;
}

// ---------------------------------------------------------------------------
// Run motors - handles sweep and pause logic
// ---------------------------------------------------------------------------
void runMotors() {
  if (inPause) {
    uint16_t pauseDuration = pauseAtMin ? SWEEP_PAUSE_MIN_MS : SWEEP_PAUSE_MAX_MS;
    if (pauseTimer >= pauseDuration) {
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
        sweepingDown = false;        // reached bottom
        inPause = true;
        pauseAtMin = true;           // use SWEEP_PAUSE_MIN_MS
        pauseTimer = 0;
      }
    } else {
      if (currentPulse < SERVO_MAX) {
        currentPulse++;
      } else {
        sweepingDown = true;
        if (windingDown) {           // stops here at SERVO_MAX
          motorsActive = false;
          windingDown = false;
          cycleTimer = 0;
          return;
        }
        inPause = true;
        pauseAtMin = false;          // use SWEEP_PAUSE_MAX_MS
        pauseTimer = 0;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  if (motorsActive) {
    if (cycleTimer >= ACTIVE_DURATION && !windingDown) {
      windingDown = true;            // request stop, let current sweep finish
    }
    runMotors();
  } else {
    if (cycleTimer >= IDLE_DURATION) {
      motorsActive = true;
      windingDown = false;
      inPause = false;
      pauseAtMin = false;            // reset pause tracking
      sweepingDown = true;           // reset sweep direction
      currentPulse = SERVO_MAX;      // reset position to top
      setAllServosPWM(currentPulse); // physically move servo to start position
      cycleTimer = 0;
      sweepTimer = 0;
      pauseTimer = 0;
    }
  }
}
}