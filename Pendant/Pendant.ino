/*  Encoder readout code based on code found here https://github.com/mo-thunderz/RotaryEncoder
*/

const int ENC_A = 2;
const int ENC_B = 3;
const int feedFWDPin = 5;
const int feedREVPin = 6;
const int times10Pin = 7;
const int times100Pin = 8;
const int YaxisEnablePin = 9;
const int ZaxisEnablePin = 10;
const int feedSpeedPot = A0;

volatile int readDelta = 0;
volatile uint8_t old_AB = 0;
int sumFeedSpeed = 0;
int feedSpeed = 0;
int speedAmount = 0;

unsigned long lastFeedTime = 0;
const unsigned long feedInterval = 100;

// Encode to save space in transmission
uint8_t encodeDelta(int delta, bool times10, bool times100, bool Yaxis, bool Zaxis) {
  uint8_t encoded = 0;

  // Put sign bit at position 7
  if (delta < 0) {
    encoded |= 0b10000000;
    delta = -delta;
  }

  // Limit max delta to 7
  if (delta > 7){
    delta = 7;
    Serial.println("Delta too high");
  } 

  // Put factor 10/100 bits in postion 5 and 6 respectively
  if(times10) encoded |= (0b01 << 5);
  else if(times100) encoded |= (0b10 << 5);
  else  encoded |= (0b00 << 5);
  
  // Put axis direction bits at position 3 and 4
  if(Yaxis) encoded |= (0b01 << 3);
  else if(Zaxis) encoded |= (0b10 << 3);
  else encoded |= (0b00 << 3);

  // Put the trimmed delta in the remaining spots
  encoded |= (delta & 0b00000111);

  return encoded;
}

uint8_t encodeFeed(bool feedFWD, bool feedREV, int speedVal){
  uint8_t result = 0;

  speedVal = map(speedVal,0,1023,0,127);
  result |= (speedVal & 0b01111111);

  if(feedREV && !feedFWD){
    result |= 0b10000000;
  } else if(!(!feedREV && feedFWD)){
    return 0;
  }

  return result;
}

void encoderReadout() {
  static int8_t encVal = 0;   // Encoder value
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Shift left twice, remembering previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A (as MSB)
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B (as LSB)

  encVal += enc_states[(old_AB & 0x0f)];

  // Update counter
  if(encVal > 1){     // Could be 3 depending on construction of encoder
    readDelta++;   // Update counter
    encVal = 0;
  }
  else if(encVal < -1){   // Could be -3 depending on construction of encoder
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
  
  old_AB = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);  // Startwaarde op basis van echte toestand
}

void loop() {
  bool feedFWD = !digitalRead(feedFWDPin);
  bool feedREV = !digitalRead(feedREVPin);
  int feedSpeed = analogRead(feedSpeedPot);

  // Safely read out encoder position
  noInterrupts();
  int rawDelta = readDelta;
  readDelta = 0;
  interrupts();

  if(rawDelta != 0 || (feedFWD ^ feedREV)){
    if(rawDelta != 0 || ((feedFWD ^ feedREV) && (millis() - lastFeedTime >= feedInterval))){ 
      lastFeedTime = millis();

      if(speedAmount > 0){
        feedSpeed = sumFeedSpeed/speedAmount;
      }
      sumFeedSpeed = 0;
      speedAmount = 0;

      uint8_t encodedDelta = encodeDelta(rawDelta, !digitalRead(times10Pin), !digitalRead(times100Pin), !digitalRead(YaxisEnablePin), !digitalRead(ZaxisEnablePin));   
      Serial1.write(encodedDelta);
      uint8_t feedByte = encodeFeed(feedFWD, feedREV, feedSpeed);
      Serial1.write(feedByte);

      Serial.print("Jog delta (raw): ");
      Serial.print(rawDelta);
      Serial.print(" | Feed speed: ");
      Serial.println(feedSpeed);
    } else if(speedAmount < 150){
      speedAmount++;
      sumFeedSpeed += feedSpeed;
    } 
  }
} 