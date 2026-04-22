/**
 * Reviced code for 80-Servo Controller using 5x Adafruit PCA9685 PWM Driver Boards
 *Based on servo example from Servo Driver Library
 * 
 * Hardware: Arduino Uno + 5x Adafruit PCA9685 (I2C addresses 0x40–0x43)
 * Each board drives 16 servos, 80 total.
 * 
 * Dependencies:
 *  - Adafruit PWM Servo Driver Library
 *  - Wire.h
 *  -Ellapsed Millis
 
 * Author: Sofie Tveitnes
 * Date: 16.04.2026
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <elapsedMillis.h>
/*CHANGED: I added the adresses for the 4 other boards
*/
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver board2 = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver board3 = Adafruit_PWMServoDriver(0x42);
Adafruit_PWMServoDriver board4 = Adafruit_PWMServoDriver(0x43);
Adafruit_PWMServoDriver board5 = Adafruit_PWMServoDriver(0x44);
// called this way, it uses the default address 0x40


#define SERVOMIN  200 // This is the 'minimum' pulse length count - specific for this project (out of 4096)
#define SERVOMAX  450 // This is the 'maximum' pulse length count - specific for this project(out of 4096)
#define SERVO_FREQ 50

elapsedMillis pwmTimer;                 // our timer

const unsigned long PAUSE_VALUE = 10;   // pause value in ms, we can tune this one

int pwm = 0;          // current PWM value sent to all servos
int direction = 1;    // +1 going up, -1 going down


/*CHANGED: Added each board begin,
//CHANGED: added wire begin
*/
void setup() {
  Wire.begin();
  Wire.setClock(400000L); 
  
  Serial.begin(9600);
  Serial.println("16 servo test");


  board1.begin();
  board1.setOscillatorFrequency(27000000);
  board1.setPWMFreq(50);

  board2.begin();
  board2.setOscillatorFrequency(27000000);
  board2.setPWMFreq(50);

  board3.begin();
  board3.setOscillatorFrequency(27000000);
  board3.setPWMFreq(50);

  board4.begin();
  board4.setOscillatorFrequency(27000000);
  board4.setPWMFreq(50);

  board5.begin();
  board5.setOscillatorFrequency(27000000);
  board5.setPWMFreq(50);

  delay(5);

}
/*
* this is a funtion that writes an @int value on servos 0 - 80
*/

/*CHANGED: Added the different boards in for testing if this makes sense
//This works, now Ill try to fill one servo driver

*/

void writePWM(uint16_t value) {
    board1.setPWM(0, 0, value);
}
/* REST OF SERVOS:
    board1.setPWM(1, 0, value);
    board1.setPWM(2, 0, value);
    board1.setPWM(3, 0, value);
    board1.setPWM(4, 0, value);
    board1.setPWM(5, 0, value);
    board1.setPWM(6, 0, value);
    board1.setPWM(7, 0, value);
    board1.setPWM(8, 0, value);
    board1.setPWM(9, 0, value);
    board1.setPWM(10, 0, value);
    board1.setPWM(11, 0, value);
    board1.setPWM(12, 0, value);
    board1.setPWM(13, 0, value);
    board1.setPWM(14, 0, value);
    board1.setPWM(15, 0, value);

    board2.setPWM(0, 0, value);
    board2.setPWM(1, 0, value);
    board2.setPWM(2, 0, value);
    board2.setPWM(3, 0, value);
    board2.setPWM(4, 0, value);
    board2.setPWM(5, 0, value);
    board2.setPWM(6, 0, value);
    board2.setPWM(7, 0, value);
    board2.setPWM(8, 0, value);
    board2.setPWM(9, 0, value);
    board2.setPWM(10, 0, value);
    board2.setPWM(11, 0, value);
    board2.setPWM(12, 0, value);
    board2.setPWM(13, 0, value);
    board2.setPWM(14, 0, value);
    board2.setPWM(15, 0, value);

    board3.setPWM(0, 0, value);
    board3.setPWM(1, 0, value);
    board3.setPWM(2, 0, value);
    board3.setPWM(3, 0, value);
    board3.setPWM(4, 0, value);
    board3.setPWM(5, 0, value);
    board3.setPWM(6, 0, value);
    board3.setPWM(7, 0, value);
    board3.setPWM(8, 0, value);
    board3.setPWM(9, 0, value);
    board3.setPWM(10, 0, value);
    board3.setPWM(11, 0, value);
    board3.setPWM(12, 0, value);
    board3.setPWM(13, 0, value);
    board3.setPWM(14, 0, value);
    board3.setPWM(15, 0, value);

    board4.setPWM(0, 0, value);
    board4.setPWM(1, 0, value);
    board4.setPWM(2, 0, value);
    board4.setPWM(3, 0, value);
    board4.setPWM(4, 0, value);
    board4.setPWM(5, 0, value);
    board4.setPWM(6, 0, value);
    board4.setPWM(7, 0, value);
    board4.setPWM(8, 0, value);
    board4.setPWM(9, 0, value);
    board4.setPWM(10, 0, value);
    board4.setPWM(11, 0, value);
    board4.setPWM(12, 0, value);
    board4.setPWM(13, 0, value);
    board4.setPWM(14, 0, value);
    board4.setPWM(15, 0, value);

    board5.setPWM(0, 0, value);
    board5.setPWM(1, 0, value);
    board5.setPWM(2, 0, value);
    board5.setPWM(3, 0, value);
    board5.setPWM(4, 0, value);
    board5.setPWM(5, 0, value);
    board5.setPWM(6, 0, value);
    board5.setPWM(7, 0, value);
    board5.setPWM(8, 0, value);
    board5.setPWM(9, 0, value);
    board5.setPWM(10, 0, value);
    board5.setPWM(11, 0, value);
    board5.setPWM(12, 0, value);
    board5.setPWM(13, 0, value);
    board5.setPWM(14, 0, value);
    board5.setPWM(15, 0, value);
    // add more servos here

*/

/*
void loop() {
    for (uint16_t pulseLength = SERVOMAX; pulseLength > SERVOMIN; pulseLength--) {
    setAllServos(pulseLength);
    //Serial.print("current pulselength: ");
    //Serial.println(pulseLength);
    delay(PAUSE_MS);       
  }

  delay(500);

  for (uint16_t pulseLength = SERVOMIN; pulseLength < SERVOMAX; pulseLength++) {
    setAllServos(pulseLength);
    //Serial.print("current pulselength: ");
    //Serial.println(pulseLength);
    delay(PAUSE_MS);   
 
  }
  delay(500);
  }
  */


void loop() {
  if (pwmTimer >= PAUSE_VALUE) {                    // has enough time passed since last step?
    pwmTimer = 0;                                   // reset the timer for the next step
    writePWM(pwm);                                  // send current value to all 80 servos
    pwm += direction;                               // move one step up or down
    if (pwm >= SERVOMAX) { pwm = SERVOMAX; direction = -1; }  // hit the top, now go down
    if (pwm <= SERVOMIN)   { pwm = SERVOMIN;   direction = 1;  }  // hit the bottom, now go up
  }
}



