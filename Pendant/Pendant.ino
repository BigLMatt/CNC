const int clockPin = 2;
const int DTPin = 3;
const int buttonEncoder = 4;
const int feedSpeedPot = A0;
const int times10Pin = 7;
const int times100Pin = 8;

volatile int readDelta = 0;
int lastFeedSpeed = -1;
bool lastButtonState = HIGH;


// Encode to save space in transmission
uint8_t encodeDelta(int delta, bool times10, bool times100) {
  uint8_t encoded = 0;

  if (delta < 0) {
    encoded |= 0b10000000;
    delta = -delta;
  }

  // Limit max delta to 31
  if (delta > 31) delta = 31;

  // Put factor 10/100 bits in postion 5 and 6 respectively
  if(times10) encoded |= (0b01 << 5);
  else if(times100) encoded |= (0b10 << 5);
  else  encoded |= (0b00 << 5);
  
  encoded |= (delta & 0b00011111);

  return encoded;
}

void setup() {
  Serial.begin(115200);   // Best to match with serial1 otherwise speed impact
  Serial1.begin(115200);   // Needs to match receiver

  pinMode(clockPin, INPUT);
  pinMode(DTPin, INPUT);
  pinMode(buttonEncoder, INPUT_PULLUP);
  pinMode(times10Pin, INPUT);
  pinMode(times100Pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(clockPin), encoderISR, CHANGE);
}

void loop() {
  int rawValue = analogRead(feedSpeedPot);
  int feedSpeed = map(rawValue, 0, 1023, 0, 50);

  // Safely read out encoder position
  noInterrupts();
  int rawDelta = readDelta;
  readDelta = 0;
  interrupts();

  if(rawDelta != 0){
    uint8_t encodedDelta = encodeDelta(rawDelta, digitalRead(times10Pin), digitalRead(times100Pin));
    lastFeedSpeed = feedSpeed;

    Serial1.write(encodedDelta);
    Serial1.write(feedSpeed);

    Serial.print("Jog delta (raw): ");
    Serial.print(rawDelta);
    Serial.print(" | Feed speed: ");
    Serial.println(feedSpeed);    
  } 

  // Debounced button (not used)
  bool buttonState = digitalRead(buttonEncoder);
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed");
    delay(50);
  }
  lastButtonState = buttonState;
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