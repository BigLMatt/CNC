#include <SPI.h>

#define SLAVE_ADDR 0x08  // I2C adres van slave

// Pin definitie
const int clockPin = 2;
const int DTPin = 3;
const int buttonEncoder = 4;
const int feedSpeedPot = A0;
const int times10Pin = 7;
const int times100Pin = 8;

// Variabelen
volatile int readDelta = 0;
int lastFeedSpeed = -1;
bool lastButtonState = HIGH;

// Gemiddelde potentiometeruitlezing voor stabiliteit
int readStablePotentiometer(int pin, int samples = 10) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(1);  // Kleine vertraging om stabiele samples te krijgen
  }
  return sum / samples;
}

uint8_t encodeDelta(int delta, bool times10, bool times100) {
  uint8_t encoded = 0;

  // Zet bit 7: teken
  if (delta < 0) {
    encoded |= 0b10000000;
    delta = -delta;
  }

  // Beperk waarde tot max 31
  if (delta > 31) delta = 31;

  // Zet factor in bit 6-5
  if(times10) encoded |= (0b01 << 5);
  else if(times100) encoded |= (0b10 << 5);
  else  encoded |= (0b00 << 5);
  
  // Zet waarde in bit 0-4
  encoded |= (delta & 0b00011111);

  return encoded;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(clockPin, INPUT);
  pinMode(DTPin, INPUT);
  pinMode(buttonEncoder, INPUT_PULLUP);
  pinMode(times10Pin, INPUT);
  pinMode(times100Pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(clockPin), encoderISR, CHANGE);
}

void loop() {
  // Potentiometer uitlezen en mappen
  int rawValue = readStablePotentiometer(feedSpeedPot);
  int feedSpeed = map(rawValue, 0, 1023, 0, 50);

  // Encoderpositie veilig uitlezen
  int rawDelta = readDelta;
  readDelta = 0;;

  // Als er iets verandert wordt nieuwe data gestuurd
  if(rawDelta != 0 || feedSpeed != lastFeedSpeed){
    uint8_t encodedDelta = encodeDelta(rawDelta, digitalRead(times10Pin), digitalRead(times100Pin));
    lastFeedSpeed = feedSpeed;

    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write((encodedDelta));         // Delta encoder encoded
    Wire.write((uint8_t)feedSpeed);      // Feed snelheid   0-255
    int result = Wire.endTransmission(); // 0 = OK, >0 = fout

    if (result == 0) {
      Serial.print("I2C OK | ");
      Serial.print("Jog delta (raw): ");
      Serial.print(rawDelta);
      Serial.print(" | Feed speed: ");
      Serial.println(feedSpeed);
    } else {
      Serial.print("I2C ERROR: ");
      Serial.println(result);
    }
  } 

  // Debounced knopcontrole
  bool buttonState = digitalRead(buttonEncoder);
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed");
    delay(50); // debounce
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