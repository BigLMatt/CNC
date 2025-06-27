const int clockPin = 2;
const int DTPin = 3;
const int feedFWDPin = 5;
const int feedREVPin = 6;
const int times10Pin = 7;
const int times100Pin = 8;
const int YaxisEnablePin = 9;
const int ZaxisEnablePin = 10;
const int feedSpeedPot = A0;

volatile int readDelta = 0;
int lastFeedSpeed = -1;
bool lastButtonState = HIGH;


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

// Encoder interrupt handler
void encoderISR() {
  int stateA = digitalRead(clockPin);
  int stateB = digitalRead(DTPin);

  if (stateB != stateA) {
    readDelta += 1;
  } else {
    readDelta -= 1;
  }
}

void setup() {
  Serial.begin(115200);   // Best to match with serial1 otherwise speed impact
  Serial1.begin(115200);   // Needs to match receiver

  pinMode(clockPin, INPUT);
  pinMode(DTPin, INPUT);
  pinMode(feedFWDPin, INPUT_PULLUP);
  pinMode(feedREVPin, INPUT_PULLUP);
  pinMode(times10Pin, INPUT_PULLUP);
  pinMode(times100Pin, INPUT_PULLUP);
  pinMode(YaxisEnablePin, INPUT_PULLUP);
  pinMode(ZaxisEnablePin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(clockPin), encoderISR, CHANGE);
}

void loop() {
  int rawValue = analogRead(feedSpeedPot);
  int feedSpeed = map(rawValue, 0, 1023, 0, 50);
  bool feedFWD = !digitalRead(feedFWDPin);
  bool feedREV = !digitalRead(feedREVPin);

  // Safely read out encoder position
  noInterrupts();
  int rawDelta = readDelta;
  readDelta = 0;
  interrupts();

  if(rawDelta != 0 || (feedFWD ^ feedREV)){
    uint8_t encodedDelta = encodeDelta(rawDelta, !digitalRead(times10Pin), !digitalRead(times100Pin), !digitalRead(YaxisEnablePin), !digitalRead(ZaxisEnablePin));
    lastFeedSpeed = feedSpeed;

    Serial1.write(encodedDelta);
    Serial1.write(feedSpeed);

    Serial.print("Jog delta (raw): ");
    Serial.print(rawDelta);
    Serial.print(" | Feed speed: ");
    Serial.println(feedSpeed);    
  }
} 