//  (1)
// copy/paste these in the field alongside your other field #definitions and variables

#define ACTIVE_DURATION  180000UL  // 3 min in ms
#define IDLE_DURATION    120000UL  // 2 min in ms

elapsedMillis cycleTimer;
bool motorsActive = true;

//  (2)
// then copyp/paste this function above the main void loop()

// Handles logic for both sweeping/pausing.
void runMotors() {
  if (inPause) {                              // If we're in a pause
    if (pauseTimer >= SWEEP_PAUSE_MS) {     // If the pause duration has elapsed
      inPause = false;                      // Exit the pause
      sweepTimer = 0;                       // Reset the sweep timer to start the next sweep immediately
    }
    return;
  }

  //original sweep logic follows

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
  if (motorsActive) {                       // If motors are active, run the sweep logic
    if (cycleTimer >= ACTIVE_DURATION) {
      setAllServosPWM((SERVO_MIN + SERVO_MAX) / 2);
      motorsActive = false;
      cycleTimer = 0;
    } else {
      runMotors();
    }
  } else {                    // If motors are idle, check if it's time to reactivate them  
    if (cycleTimer >= IDLE_DURATION) {
      motorsActive = true;
      cycleTimer = 0;
      sweepTimer = 0;
    }
  }
}