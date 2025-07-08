/*  
Project: CNC Pendant, pendant side
Description: Sends jog inputs from jog wheel and feed rates when feed buttons are pressed using serial communication.
Author: Matteo https://github.com/BigLMatt
Date last altered: 08/07/2025

Sender is arduino uno R4 wifi
Hook up TX of sender to RX of receiver and also connect both GND to eachother
All other pins are described below, most can be exchanged except for ENC_A and ENC_B need to be pins that can handle interrupt
Jog and feed data are encoded with all necessary info in their respective bytes
Baudrate is 115200 but higher is better to avoid skipping jog data because delta is capped at 7 
Expandable to 1 extra speed factor and 1 extra axis
Encoder readout code based on code found here https://github.com/mo-thunderz/RotaryEncoder
*/

// Allows printing out of info like raw encoder delta and feedrate
//#define DEBUG

#define ENC_A 2
#define ENC_B 3
#define feedFWDPin 5
#define feedREVPin 6
#define times10Pin 7
#define times100Pin 8
#define YaxisEnablePin 9
#define ZaxisEnablePin 10
#define feedSpeedPot A0

volatile int readDelta = 0;
int sumFeedSpeed = 0;
int feedSpeed = 0;
int speedAmount = 0;
bool prevTimes10 = false;
bool prevTimes100 = false;
bool prevEnableY = false;
bool prevEnableZ = false;
bool prevPressed = false;

unsigned long lastFeedTime = 0;
const unsigned long feedInterval = 50;
const unsigned long setupInterval = 5000;
unsigned long lastSetupTime = 0;

// Encode change in axis or multiplicand jog
uint8_t encodeAxis(bool times10, bool times100, bool enableY, bool enableZ){
  uint8_t encoded = 0;

  // Put factor 10/100 bits in postion 4 and 5 respectively
  if(times10) encoded |= (0b01 << 4);
  else if(times100) encoded |= (0b10 << 4);
  
  // Put axis direction bits at position 2 and 3
  if(enableY) encoded |= (0b01 << 2);
  else if(enableZ) encoded |= (0b10 << 2);
  else encoded |= (0b00 << 3);

  return encoded;
}

// Encode jog delta and direction
uint8_t encodeDelta(int delta) {
  uint8_t encoded = 0b01000000;

  // Put sign bit at position 5
  if (delta < 0) {
    encoded |= 0b1 << 5;
    delta = -delta;
  }

  // Limit max delta to 31
  if (delta > 31){
    delta = 31;
    Serial.println("Delta too high");
  } 

  // Put the trimmed delta in the remaining spots
  encoded |= (delta & 0b00011111);

  return encoded;
}

uint8_t encodeFeed(bool feedFWD, bool feedREV, int speedVal){
  uint8_t encoded = 0b10000000;

  speedVal = speedVal >> 4;
  encoded |= (speedVal & 0b00111111);

  if(feedREV){
    encoded |= 0b01000000;
  } else if(feedFWD){
    encoded |= 0b00000000;
  }

  return encoded;
}

void encoderReadout() {
  static int8_t encVal = 0;   // Encoder value
  static uint8_t old_AB = 3;  // Lookup table index (starts at 11)
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Shift left twice, remembering previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A (as MSB)
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B (as LSB)

  encVal += enc_states[(old_AB & 0x0f)];

  // Update counter
  if(encVal > 3){     // Could be 1 depending on construction of encoder half or full quadrature
    readDelta++;   // Update counter
    encVal = 0;
  }
  else if(encVal < -3){   // Could be -1 depending on construction of encoder half or full quadrature
    readDelta--;     // Update counter
    encVal = 0;
  }  
}

void setup() {
  Serial.begin(115200);   // Best to match with serial1 otherwise speed impact
  Serial1.begin(115200);   // Needs to match receiver

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(feedFWDPin, INPUT_PULLUP);
  pinMode(feedREVPin, INPUT_PULLUP);
  pinMode(times10Pin, INPUT_PULLUP);
  pinMode(times100Pin, INPUT_PULLUP);
  pinMode(YaxisEnablePin, INPUT_PULLUP);
  pinMode(ZaxisEnablePin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderReadout, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderReadout, CHANGE);  
}

void loop() {
  bool times10 = !digitalRead(times10Pin);
  bool times100 = !digitalRead(times100Pin);
  bool enableY =!digitalRead(YaxisEnablePin);
  bool enableZ = !digitalRead(ZaxisEnablePin);
  bool feedFWD = !digitalRead(feedFWDPin);
  bool feedREV = !digitalRead(feedREVPin);
  int feedSpeed = analogRead(feedSpeedPot);

  // Safely read out encoder position
  noInterrupts();
  int rawDelta = readDelta;
  readDelta = 0;
  interrupts();

  if(rawDelta != 0){ 
    #ifdef DEBUG
    Serial.print("Jog delta (raw): ");
    Serial.println(rawDelta);
    #endif
    
    uint8_t encodedDelta = encodeDelta(rawDelta);   
    Serial1.write(encodedDelta);

  }

  if(prevTimes10 != times10 || prevTimes100 != times100 || prevEnableY != enableY || prevEnableZ != enableZ || (millis() - lastSetupTime >= setupInterval)){
    uint8_t axis = encodeAxis(times10, times100, enableY, enableZ);

    prevTimes10 = times10;
    prevTimes100 = times100;
    prevEnableY = enableY;
    prevEnableZ = enableZ;
    lastSetupTime = millis();

    Serial1.write(axis);

    #ifdef DEBUG
    Serial.print("Sent setup byte: ");
    Serial.println(axis);
    #endif
  } 

  if(prevPressed && !(feedFWD || feedREV)){
    prevPressed = false;
    Serial1.write(0b10000000);
  }

  if((feedFWD ^ feedREV) && (millis() - lastFeedTime >= feedInterval)){
    prevPressed = true;
    lastFeedTime = millis();
    
    if(speedAmount > 0){
      feedSpeed = sumFeedSpeed/speedAmount;
    }
    sumFeedSpeed = 0;
    speedAmount = 0;

    uint8_t feedByte = encodeFeed(feedFWD, feedREV, feedSpeed);
    Serial1.write(feedByte);

    #ifdef DEBUG
    Serial.print("Feed speed: ");
    Serial.println(feedSpeed);
    #endif
  } else if(speedAmount < 100){
    speedAmount++;
    sumFeedSpeed += feedSpeed;
  } 
} 