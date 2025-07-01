/*
Project: CNC Pendant, rudamentary receiver side
Description: Receives jog and feed bytes and decodes them and adds the values to axes, only for jog, feedrate shown seperately
Author: Matteo https://github.com/BigLMatt
Date last altered: 01/07/2025

Receiver is arduino mega
Hook up RX of receiver to TX of sender and also connect both GND to eachother
Baudrate is 115200 but higher is better to avoid skipping jog data because delta is capped at 7 
Expandable to 1 extra speed factor and 1 extra axis
*/

int x = 0;
int y = 0;
int z = 0;

// Allows printing out of axes and their
#define DEBUG

void setup() {
  Serial.begin(115200);  // Needs to match transmitter
}

void loop() {
  static int state = 0;
  static uint8_t buffer[1];
 
  while (Serial.available() >=2) {
    uint8_t jogByte = Serial.read();
    uint8_t feedByte = Serial.read();

    // Decoding byte with jog information
    decodeJog(jogByte, x, y, z);

    bool FWD, REV;
    int feedSpeed;

    decodeFeed(feedByte, FWD, REV, feedSpeed);

    #ifdef DEBUG
    Serial.print("X: ");
    Serial.print(x);
    Serial.print("  Y: ");
    Serial.print(y);
    Serial.print("  Z: ");
    Serial.print(z);
    Serial.print(" | Direction: ");
    Serial.print(FWD ? "FWD" : "REV");
    Serial.print(" | Feedrate: ");
    Serial.println(feedSpeed);
    #endif
  }
}

void decodeFeed(uint8_t feedByte, bool &isFWD, bool &isREV, int &feedrate) {
  isREV = feedByte & 0b10000000;
  isFWD = !isREV;

  feedrate = feedByte & 0b01111111;
}

void decodeJog(uint8_t jogByte, int &x, int &y, int &z){
  bool sign = jogByte & 0b10000000;
  uint8_t scaleBits = (jogByte >> 5) & 0b11;
  uint8_t axis = (jogByte >> 3) & 0b11;
  uint8_t magnitude = jogByte & 0b00000111;

  int factor = 1;
  if (scaleBits == 0b01) factor = 10;
  else if (scaleBits == 0b10) factor = 100;

  int delta = magnitude * factor;
  if (sign) delta = -delta;

  if (axis == 0b01) y += delta;
  else if (axis == 0b10) z += delta;
  else x += delta;
}