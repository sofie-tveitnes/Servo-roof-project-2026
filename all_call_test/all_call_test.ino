//imports
#include <Wire.h> //i2c com
#include <Adafruit_PWMServoDriver.h> //servo driver
#include <elapsedMillis.h> //convenient timer

//definitions
#define SERVOS_PER_BOARD 16
#define NUM_BOARDS 5
#define TOTAL_SERVOS (SERVOS_PER_BOARD * NUM_BOARDS)
#define SERVO_MIN 200
#define SERVO_MAX 450
#define SERVO_FREQ 50
#define OSC_FREQ 27000000UL
#define SWEEP_STEP_MS 15
#define SWEEP_PAUSE_MS 500
#define ALLCALL_ADDR 0x70

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

uint16_t currentPulse = SERVO_MAX;
bool sweepingDown = true;
bool inPause = false;

const uint32_t I2C_CLOCK_FREQ = 400000;

// ---------------------------------------------------------------------------
// Board initialization
// ---------------------------------------------------------------------------
void initBoards() {
  for (uint8_t i = 0; i < NUM_BOARDS; i++) { //for each board, perform a set of init functions
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
// writes at once to all boards, specifies cndition to set a given PWM signal
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
  Wire.begin(); //start i2c bus
  Wire.setClock(I2C_CLOCK_FREQ); //set i2c bus freq
  initBoards(); //init all boards
  setAllServosPWM(currentPulse); //set all servos to max
  sweepTimer = 0; //reset timer
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  //if machine is in pause
  if (inPause) {
    if (pauseTimer >= SWEEP_PAUSE_MS) { //if pause timer is smaller than max pause time, stop pause
      inPause = false; //not in pause anyore
      sweepTimer = 0; //reset sweep timer
    }
    return;
  }

  // if sweep timer bigger than desired "delay" duration
  if (sweepTimer >= SWEEP_STEP_MS) {
    sweepTimer = 0; //reset timer
    setAllServosPWM(currentPulse); //write to servos current value

    //conditions to sweep down
    if (sweepingDown) { 
      if (currentPulse > SERVO_MIN) { //if current pulse is bigger than minimum, go down
        currentPulse--;
      } else {  //if not bigger than minimum, changes "machine state"
        sweepingDown = false; //dont weep down anymore
        inPause = true; //start pause
        pauseTimer = 0; //reset pause timer
      }
    } else { //conditions to sweep up
      if (currentPulse < SERVO_MAX) {  //if current pulse is smaller than maximum, go up
        currentPulse++;
      } else { //if not smaller than maximum, changes "machine state"
        sweepingDown = true;
        inPause = true;
        pauseTimer = 0;
      }
    }
  }
}