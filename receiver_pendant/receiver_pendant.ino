/*
Project: CNC Pendant, rudimentary receiver side
Description: Receives jog and feed bytes and decodes them and adds the values to axes, only for jog, feedrate shown seperately
Author: Matteo https://github.com/BigLMatt
Date last altered: 06/07/2025

Receiver is arduino mega
Hook up RX of receiver to TX of sender and also connect both GND to eachother
Baudrate is 115200 but higher is better to avoid skipping jog data because delta is capped at 7 
Expandable to 1 extra speed factor and 1 extra axis
*/

static int x = 0;
static int y = 0;
static int z = 0;
bool enableY = false;
bool enableZ = false;
int factor = 1;
int feedSpeed = 0;


// Allows printing out of axes and their states
#define DEBUG

void setup() {
  Serial.begin(115200);  // Needs to match transmitter
}

void loop() {
  static int state = 0;
  static uint8_t buffer[1];

  static bool FWD = false;
  static bool REV = false;
 
  while (Serial.available()) {
    uint8_t encodedByte = Serial.read();    

    // Check which byte was sent
    if ((encodedByte & 0b10000000) == 0b10000000) {
      decodeFeed(encodedByte, FWD, REV, feedSpeed);
    } else if ((encodedByte & 0b11000000) == 0b01000000) {
      decodeJog(encodedByte, enableY, enableZ, factor, x, y, z);
    } else if ((encodedByte & 0b11000000) == 0b00000000) {
      decodeAxis(encodedByte, factor, enableY, enableZ);
    }   

    #ifdef DEBUG
    Serial.print(encodedByte);
    Serial.print("   X: ");
    Serial.print(x);
    Serial.print("  Y: ");
    Serial.print(y);
    Serial.print("  Z: ");
    Serial.print(z);
    Serial.print(" | Direction: ");
    if (FWD) Serial.print("FWD");
    else if (REV) Serial.print("REV");
    else Serial.print("NONE");
    Serial.print(" | Feedrate: ");
    Serial.println(feedSpeed);
    #endif
  }
}

void decodeFeed(uint8_t feedByte, bool &isFWD, bool &isREV, int &feedSpeed) {
  if ((feedByte & 0b01111111) == 0) {
    isFWD = false;
    isREV = false;
    feedSpeed = 0;
    return;
  }

  isREV = (feedByte & 0b01000000) != 0;
  isFWD = !isREV;
  feedSpeed = feedByte & 0b00111111;
}

void decodeJog(uint8_t jogByte, bool &enableY, bool &enableZ, int &factor, int &x, int &y, int &z){
  bool sign = jogByte & 0b00100000;
  uint8_t magnitude = jogByte & 0b00011111;

  int delta = magnitude * factor;
  if (sign) delta = -delta;

  if (enableY) y += delta;
  else if (enableZ) z += delta;
  else x += delta;
}

void decodeAxis(uint8_t axisByte, int &factor, bool &enableY, bool &enableZ){
  uint8_t scaleBits = (axisByte & 0b00110000) >> 4 ;
  uint8_t axisBits = (axisByte & 0b00001100) >> 2;

  if (scaleBits == 0b01) factor = 10;
  else if (scaleBits == 0b10) factor = 100;
  else factor = 1;

  enableY = (axisBits == 0b01);
  enableZ = (axisBits == 0b10);
}