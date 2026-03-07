/**
 * 64-Servo Controller using 4x Adafruit PCA9685 PWM Driver Boards
 * 
 * Hardware: Arduino Uno + 4x Adafruit PCA9685 (I2C addresses 0x40–0x43)
 * Each board drives 16 servos, 64 total.
 * 
 * Dependencies:
 *  - Adafruit PWM Servo Driver Library
 *  - elapsedMillis by Paul Stoffregen
 *
 * Author: Sofie Tveitnes 
 * Date: 04.03.2026
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <elapsedMillis.h>

// ---------------------------------------------------------------------------
// Hardware config
// ---------------------------------------------------------------------------

/**
* CHANGED: Your original code had four separate #defines for each board's
* servo count (BOARD1_SERVOS, BOARD2_SERVOS, etc.), all set to 16.
* Since they're all identical, we only need one constant (SERVOS_PER_BOARD).
* TOTAL_SERVOS is then derived automatically. If board counts ever differ,
* you can revisit this, but always prefer DRY (Don't Repeat Yourself) code.
*/
#define SERVOS_PER_BOARD 16
#define NUM_BOARDS_MAX 4
#define TOTAL_SERVOS (SERVOS_PER_BOARD * NUM_BOARDS)

/**
* These pulse limits are fine as definitions (#define) since they are true compile-time constants used to configure hardware, never used by the system after initial setup.
* Adjust these values if your servos don't reach their full range of motion, or if they buzz/strain when reaching limits
*/
#define SERVO_MIN 200
#define SERVO_MAX 450
#define SERVO_FREQ 50  // in Hz

// The PCA9685 has an internal oscillator that drifts slightly between boards and changes in temperature, which will in turn affect the servo frequency randomly.
// Setting this to a measured value (27MHz here) improves PWM accuracy.
// UL means unsigned long
#define OSC_FREQ 27000000UL

// Timing constants for the sweep loop
#define SWEEP_STEP_MS 15    // milliseconds between each pulse step
#define SWEEP_PAUSE_MS 500  // milliseconds to pause between sweep directions

// ---------------------------------------------------------------------------
// PWM driver instances
// ---------------------------------------------------------------------------

// CHANGED: Your original code declared four separate named variables:
//   Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); etc.
//
// Grouping them into an array means we can initialize and address them
// with a loop or index instead of repeating the same code four times.
// The I2C addresses (0x40–0x43) are set physically on each board via
// solder jumpers on the A0–A5 pads — make sure they match your hardware.
Adafruit_PWMServoDriver boards[NUM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),
  Adafruit_PWMServoDriver(0x41),
  Adafruit_PWMServoDriver(0x42),
  Adafruit_PWMServoDriver(0x43)
};

// ---------------------------------------------------------------------------
// Sweep state
// ---------------------------------------------------------------------------

// CHANGED: You used delay() in your original loop to pace the sweep and
// pause between directions. delay() is problematic in embedded code because
// it completely halts the CPU, meaning nothing else can happen during a delay,
// including reading sensors, handling serial input, or any future feature.
//
// elapsedMillis is a cleaner alternative: it tracks time passively using
// the hardware timer underneath millis(), and you just check "has enough
// time passed?" without ever blocking. Think of it like a stopwatch you
// can reset, rather than a sleep command.
//
// These three variables together represent the full state of the sweep:
// where we are, which direction we're going, and whether we're pausing.
elapsedMillis sweepTimer;
elapsedMillis pauseTimer;

uint16_t currentPulse = SERVO_MAX;  // Start at max position
bool sweepingDown = true;           // true = MAX→MIN, false = MIN→MAX
bool inPause = false;               // true while waiting between sweeps
const int BAUD_RATE = 9600;
const uint32_t I2C_CLOCK_FREQ = 400000;

// ---------------------------------------------------------------------------
// Function declarations (prototypes)
// ---------------------------------------------------------------------------

// Declaring functions here at the top lets you define them in any order below, an make it easy to see at the top which methods you implement
bool initBoards();
void setServoPWM(uint8_t servoIndex, uint16_t pulselen);

// ---------------------------------------------------------------------------
// Board initialization
// ---------------------------------------------------------------------------

/**
 * Initializes all four PWM driver boards over I2C.
* @return true if initialization completed
 *
 * CHANGED: This method is implemented to make the void setup() more readble and modular (= future proofing).
 * Your original setup() called begin(), setOscillatorFrequency(), and setPWMFreq() four times with copy-pasted code, once per board.
 * Extracting this into a dedicated function with a loop eliminates the repetition and makes it easy to add or remove boards in one place. We like to code lazy :)
 *
 * The bool return type is intentional, meaning it returns true/false to the setup method, so you can implement failure detection at this step in the future.
 * The Adafruit library doessn't return any error detection, so here we return true by default.
 */
bool initBoards() {
  for (uint8_t i = 0; i < NUM_BOARDS; i++) {
    boards[i].begin();
    boards[i].setOscillatorFrequency(OSC_FREQ);
    boards[i].setPWMFreq(SERVO_FREQ);
  }
  return true;
}

// ---------------------------------------------------------------------------
// Servo dispatch
// ---------------------------------------------------------------------------

/**
 * Moves a single servo to a given pulse width.
 *
 * CHANGED: This function existed in your original code but was empty, which meant none of your sweep loops actually did anything when called.
 *
 * We have 64 servos numbered 0–63 across 4 boards, but each board only knows about its own 16 channels (0–15).
 * This function automatically figures out which board/channel a given servo belongs to.
 * You just pass a number from 0 to 63 (= a servo) and it handles the rest.
 *
 * If you scale up (f.eks. 5 boards, 80 servos), just update variable NUM_BOARDS and add an extra board to the boards[] array, this function will adapt automatically.
 *
 * Example: servo 37
 *   37 / 16 = 2  → boards[2] (the third board, address 0x42)
 *   37 % 16 = 5  → channel 5 on that board
 *
 * @param servoIndex  Which servo to move, 0 to 63
 * @param pulselen    Pulse width, between SERVO_MIN and SERVO_MAX
 */
void setServoPWM(uint8_t servoIndex, uint16_t pulselen) {
  // Bounds guard: silently ignore out of range indices rather than sending garbage to a nonexistent board channel
  if (servoIndex >= TOTAL_SERVOS) return;

  uint8_t boardIndex = servoIndex / SERVOS_PER_BOARD;
  uint8_t channel = servoIndex % SERVOS_PER_BOARD;

  boards[boardIndex].setPWM(channel, 0, pulselen);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(BAUD_RATE);
  Wire.setClock(I2C_CLOCK_FREQ);

  // NOTE: F() wraps string literals so they are stored in flash (program memory) instead of being copied into RAM at startup (the UNo doesnt have a lot of RAM..)
  Serial.println(F("Initializing 64-servo controller..."));

  if (!initBoards()) {
    Serial.println(F("ERROR: Board initialization failed. Check wiring and I2C addresses."));
    while (1)
      ;  // Halt — no point continuing if drivers aren't responding
  }

  Serial.println(F("All boards initialized. Starting sweep."));

  // Move all servos to the starting position before the loop begins, so they don't jump unpredictably
  for (uint8_t s = 0; s < TOTAL_SERVOS; s++) {
    setServoPWM(s, currentPulse);
  }

  //reset timer
  sweepTimer = 0;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

/**
 * Sweeps all 64 servos back and forth between SERVO_MIN and SERVO_MAX, one pulse-width tick per SWEEP_STEP_MS interval, with a pause of SWEEP_PAUSE_MS between direction changes.
 *
 * CHANGED: Your original loop() used delay() three times, once per sweep step and once between sweeps.
 * Since delay() blocks the CPU, we use elapsedMillis() (external library) to achieve the same timing without blocking the system.
 *
 * The logic is structured with three states:
 *   Pausing: waiting between directions (inPause == true)
 *   Sweeping down: decrementing pulse toward SERVO_MIN
 *   Sweeping up: incrementing pulse toward SERVO_MAX
 *
 * Because loop() returns immediately if there's nothing to do, other code
 * (sensor reads, serial commands, etc.) can be added here later without
 * disrupting the sweep timing.
 */
void loop() {

  // Pausing
  if (inPause) {
    if (pauseTimer >= SWEEP_PAUSE_MS) {
      inPause = false;
      sweepTimer = 0;  // Reset sweep timer so next step starts cleanly
    }
    return;  // Nothing else to do while pausing
  }

  // Sweeping down/up
  if (sweepTimer >= SWEEP_STEP_MS) {
    sweepTimer = 0;

    // Apply the current PWM to every servo
    for (uint8_t s = 0; s < TOTAL_SERVOS; s++) {
      setServoPWM(s, currentPulse);
    }

    if (sweepingDown) {
      // Moving MAX to MIN
      if (currentPulse > SERVO_MIN) {
        currentPulse--;
      } else {
        // Hit the bottom, flip direction and pause
        Serial.println(F("Reached MIN. Reversing..."));
        sweepingDown = false;
        inPause = true;
        pauseTimer = 0;
      }
    } else {
      // Moving MIN to MAX
      if (currentPulse < SERVO_MAX) {
        currentPulse++;
      } else {
        // Hit the top, flip direction and pause
        Serial.println(F("Reached MAX. Reversing..."));
        sweepingDown = true;
        inPause = true;
        pauseTimer = 0;
      }
    }
  }
}