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
#define ALLCALL_ADDR 0x70

// How often to print sweep status (ms) — set to 0 to disable
#define DEBUG_STATUS_MS 1000

Adafruit_PWMServoDriver boards[NUM_BOARDS] = {
  Adafruit_PWMServoDriver(0x40),
  Adafruit_PWMServoDriver(0x41),
  Adafruit_PWMServoDriver(0x42),
  Adafruit_PWMServoDriver(0x43),
  Adafruit_PWMServoDriver(0x44)
};

elapsedMillis sweepTimer;
elapsedMillis pauseTimer;
elapsedMillis debugTimer;

uint16_t currentPulse = SERVO_MAX;
bool sweepingDown = true;
bool inPause = false;

uint32_t allCallErrorCount = 0;  // cumulative ALLCALL write failures
//uint32_t sweepStepCount = 0;     // total steps taken

const int BAUD_RATE = 115200;    // bumped for Teensy — 9600 is needlessly slow
const uint32_t I2C_CLOCK_FREQ = 400000;

// ---------------------------------------------------------------------------
// I2C helpers
// ---------------------------------------------------------------------------

// Returns a human-readable string for Wire.endTransmission() status codes
const char* i2cStatusStr(uint8_t status) {
  switch (status) {
    case 0: return "OK";
    case 1: return "ERR: buffer overflow";
    case 2: return "ERR: NACK on address";
    case 3: return "ERR: NACK on data";
    case 4: return "ERR: other";
    case 5: return "ERR: timeout";
    default: return "ERR: unknown";
  }
}

// Scan I2C bus and print all responding addresses
void i2cScan() {
  Serial.println(F("[I2C] Scanning bus..."));
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.print(F("[I2C]   Device found at 0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println(F("[I2C]   No devices found!"));
  else {
    Serial.print(F("[I2C] Total devices: "));
    Serial.println(found);
  }
}

// Read back MODE1 register from a board and print it
void debugMODE1(uint8_t boardAddr) {
  Wire.beginTransmission(boardAddr);
  Wire.write(0x00);  // MODE1 register
  uint8_t err = Wire.endTransmission(false);  // repeated start
  if (err != 0) {
    Serial.print(F("[I2C]   MODE1 read addr failed 0x"));
    if (boardAddr < 0x10) Serial.print('0');
    Serial.print(boardAddr, HEX);
    Serial.print(F(": "));
    Serial.println(i2cStatusStr(err));
    return;
  }
  Wire.requestFrom((uint8_t)boardAddr, (uint8_t)1);
  if (Wire.available()) {
    uint8_t mode1 = Wire.read();
    Serial.print(F("[I2C]   Board 0x"));
    if (boardAddr < 0x10) Serial.print('0');
    Serial.print(boardAddr, HEX);
    Serial.print(F(" MODE1=0b"));
    // Print all 8 bits
    for (int8_t b = 7; b >= 0; b--) Serial.print((mode1 >> b) & 1);
    Serial.print(F("  ALLCALL="));
    Serial.print(mode1 & 0x01);
    Serial.print(F(" SLEEP="));
    Serial.println((mode1 >> 4) & 0x01);
  } else {
    Serial.println(F("[I2C]   MODE1 read: no data returned"));
  }
}

// ---------------------------------------------------------------------------
// Board initialization
// ---------------------------------------------------------------------------
bool initBoards() {
  bool allOk = true;
  for (uint8_t i = 0; i < NUM_BOARDS; i++) {
    uint8_t addr = 0x40 + i;
    Serial.print(F("[INIT] Board "));
    Serial.print(i);
    Serial.print(F(" (0x"));
    if (addr < 0x10) Serial.print('0');
    Serial.print(addr, HEX);
    Serial.println(F(")..."));

    boards[i].begin();
    boards[i].setOscillatorFrequency(OSC_FREQ);
    boards[i].setPWMFreq(SERVO_FREQ);

    // Explicitly set MODE1: oscillator on, ALLCALL enabled, auto-increment off
    Wire.beginTransmission(addr);
    Wire.write(0x00);   // MODE1
    Wire.write(0x21);   // ALLCALL=1, SLEEP=0, AI=0
    uint8_t err = Wire.endTransmission();
    Serial.print(F("[I2C]   MODE1 write: "));
    Serial.println(i2cStatusStr(err));
    if (err != 0) allOk = false;

    // Read back and verify
    debugMODE1(addr);
  }
  return allOk;
}

// ---------------------------------------------------------------------------
// ALLCALL broadcast
// ---------------------------------------------------------------------------
void setAllServosPWM(uint16_t pulselen) {
  Wire.beginTransmission(ALLCALL_ADDR);
  Wire.write(0xFA);                     // ALL_LED_ON_L
  Wire.write(0x00);                     // ON low
  Wire.write(0x00);                     // ON high
  Wire.write(pulselen & 0xFF);          // OFF low
  Wire.write((pulselen >> 8) & 0x0F);  // OFF high
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    allCallErrorCount++;
    Serial.print(F("[I2C] ALLCALL write failed: "));
    Serial.print(i2cStatusStr(err));
    Serial.print(F("  total errors: "));
    Serial.println(allCallErrorCount);
  }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(BAUD_RATE);
  delay(200);  // Give Teensy USB serial a moment to connect
  Serial.println(F("\n=== 80-servo ALLCALL controller (Teensy 4.0) ==="));

  Wire.begin();
  Wire.setClock(I2C_CLOCK_FREQ);
  Serial.print(F("[I2C] Clock set to "));
  Serial.print(I2C_CLOCK_FREQ / 1000);
  Serial.println(F(" kHz"));

  i2cScan();

  Serial.println(F("[INIT] Initializing boards..."));
  if (!initBoards()) {
    Serial.println(F("[ERROR] One or more boards failed init. Check wiring. Halting."));
    while (1);
  }
  Serial.println(F("[INIT] All boards OK."));

  Serial.print(F("[INIT] Moving all servos to start position (pulse="));
  Serial.print(currentPulse);
  Serial.println(F(")..."));
  setAllServosPWM(currentPulse);

  sweepTimer = 0;
  debugTimer = 0;
  Serial.println(F("[INIT] Sweep starting.\n"));
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  // Periodic status report
#if DEBUG_STATUS_MS > 0
  if (debugTimer >= DEBUG_STATUS_MS) {
    debugTimer = 0;
    Serial.print(F("[STATUS] pulse="));
    Serial.print(currentPulse);
    Serial.print(F("  dir="));
    Serial.print(sweepingDown ? F("DOWN") : F("UP"));
    Serial.print(F("  paused="));
    Serial.print(inPause ? F("yes") : F("no"));
    //Serial.print(F("  steps="));
    //Serial.print(sweepStepCount);
    Serial.print(F("  i2cErrors="));
    Serial.println(allCallErrorCount);
  }
#endif

  if (inPause) {
    if (pauseTimer >= SWEEP_PAUSE_MS) {
      Serial.print(F("[SWEEP] Pause done. Resuming "));
      Serial.println(sweepingDown ? F("DOWN.") : F("UP."));
      inPause = false;
      sweepTimer = 0;
    }
    return;
  }

  if (sweepTimer >= SWEEP_STEP_MS) {
    sweepTimer = 0;
    //sweepStepCount++;

    setAllServosPWM(currentPulse);

    if (sweepingDown) {
      if (currentPulse > SERVO_MIN) {
        currentPulse--;
      } else {
        Serial.println(F("[SWEEP] Reached MIN. Pausing before reversal."));
        sweepingDown = false;
        inPause = true;
        pauseTimer = 0;
      }
    } else {
      if (currentPulse < SERVO_MAX) {
        currentPulse++;
      } else {
        Serial.println(F("[SWEEP] Reached MAX. Pausing before reversal."));
        sweepingDown = true;
        inPause = true;
        pauseTimer = 0;
      }
    }
  }
}