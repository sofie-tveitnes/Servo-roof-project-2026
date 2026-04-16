#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver board2 = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver board3 = Adafruit_PWMServoDriver(0x42);
Adafruit_PWMServoDriver board4 = Adafruit_PWMServoDriver(0x43);
Adafruit_PWMServoDriver board5 = Adafruit_PWMServoDriver(0x44);
// called this way, it uses the default address 0x40


#define SERVOMIN  200 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  450 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
const int PAUSE_MS = 14; //duration between pulses

void setup() {
  Serial.begin(9600);
  board1.begin();
  board1.setPWMFreq(50);

  board2.begin();
  board2.setPWMFreq(50);

  board3.begin();
  board3.setPWMFreq(50);

  board4.begin();
  board4.setPWMFreq(50);

  board5.begin();
  board5.setPWMFreq(50);

}
/*
* this is a funtion that writes an @int value on servos 0 through 7.
*/
void setAllServos(uint16_t value) {
    board1.setPWM(0, 0, value);
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
