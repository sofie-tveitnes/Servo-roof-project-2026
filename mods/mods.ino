//imports
#include <Wire.h> //i2c com
#include <Adafruit_PWMServoDriver.h> //servo driver
#include <elapsedMillis.h> //convenient timer

//definitions
#define SERVOS_PER_BOARD 16
#define NUM_BOARDS 5
#define TOTAL_SERVOS (SERVOS_PER_BOARD * NUM_BOARDS)
#define SERVO_MIN 290
#define SERVO_MAX 450
#define SERVO_FREQ 50
#define OSC_FREQ 27000000UL
#define SWEEP_STEP_MS 8
#define SWEEP_PAUSE_MIN_MS 800   // pause duration at top (SERVO_MIN)
#define SWEEP_PAUSE_MAX_MS 1800   // pause duration at bottom (SERVO_MAX)
#define ALLCALL_ADDR 0x70

#define ACTIVE_DURATION  90000UL  // 1.5 min in ms
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
  Wire.beginTransmission(ALLCALL_ADDR);        // start I2C transaction to all boards simultaneously
  Wire.write(0xFA);                            // target ALL_LED_ON_L register (broadcasts to all 16 channels)
  Wire.write(0x00);                            // ALL_LED_ON low byte — pulse always starts at tick 0
  Wire.write(0x00);                            // ALL_LED_ON high byte — no full-on flag, normal PWM mode
  Wire.write(pulseLength & 0xFF);              // ALL_LED_OFF low byte — lower 8 bits of pulse length
  Wire.write((pulseLength >> 8) & 0x0F);       // ALL_LED_OFF high byte — upper 4 bits of pulse length (12-bit value)
  Wire.endTransmission();                      // send and release the I2C bus
}

/**
 * Stop PWM signal to all servos while idle
 */
void detachAllServos() {
  Wire.beginTransmission(ALLCALL_ADDR); // start I2C transaction to all boards simultaneously
  Wire.write(0xFA);                     // target ALL_LED_ON_L register (broadcasts to all 16 channels)
  Wire.write(0x00);                     // ALL_LED_ON low byte — no offset
  Wire.write(0x10);                     // ALL_LED_ON high byte — bit 4 set = full-on flag, forces output low
  Wire.write(0x00);                     // ALL_LED_OFF low byte — zero, ignored when full-on is set
  Wire.write(0x00);                     // ALL_LED_OFF high byte — zero, ignored when full-on is set
  Wire.endTransmission();               // send and release the I2C bus
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
        if (windingDown) {
          detachAllServos();         // cut PWM before going idle
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