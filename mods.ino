//  (1)
// copy/paste these in the field alongside your other field #definitions and variables

#define ACTIVE_DURATION  180000UL  // 3 min in ms
#define IDLE_DURATION    120000UL  // 2 min in ms

elapsedMillis cycleTimer;
bool motorsActive = true;

//  (2)
// then copyp/paste this function above the main void loop()

/*
 * Sweep logic now in its own method for clarity. Handles logic for both sweeping/pausing.
 */
void runMotors() {
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

//  (3)
// finally, replace your existing loop() function with this one
void loop() {
  if (motorsActive) {
    if (cycleTimer >= ACTIVE_DURATION) {
      setAllServosPWM((SERVO_MIN + SERVO_MAX) / 2);
      motorsActive = false;
      cycleTimer = 0;
    } else {
      runMotors();
    }
  } else {
    if (cycleTimer >= IDLE_DURATION) {
      motorsActive = true;
      cycleTimer = 0;
      sweepTimer = 0;
    }
  }
}