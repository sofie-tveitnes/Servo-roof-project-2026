#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/*CHANGED: I added the codes for the 4 other boards, as the code signals this later I changed the
*/
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver board2 = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver board3 = Adafruit_PWMServoDriver(0x42);
Adafruit_PWMServoDriver board4 = Adafruit_PWMServoDriver(0x43);
Adafruit_PWMServoDriver board5 = Adafruit_PWMServoDriver(0x44);
// called this way, it uses the default address 0x40


#define SERVOMIN  200 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  450 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50

const int PAUSE_MS = 14; //duration between pulses

/*CHANGED: I added t
*/
void setup() {
  Serial.begin(9600);
  Serial.println("4 servos board 1 and 1 servo on each board test");
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

  delay(10);

}
/*
* this is a funtion that writes an @int value on servos 0 through 7.
*/

/*CHANGED: Added the different boards in for testing if this makes sense

*/
void setAllServos(uint16_t value) {
    board1.setPWM(0, 0, value);
    board1.setPWM(1, 0, value);
    board1.setPWM(2, 0, value);
    board1.setPWM(3, 0, value);

    board2.setPWM(0, 0, value);
   
    board3.setPWM(0, 0, value);

    board4.setPWM(0, 0, value);

    board5.setPWM(0, 0, value);

    // add more servos here
}


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
