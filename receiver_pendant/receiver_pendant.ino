int x = 0;
int y = 0;
int z = 0;

void setup() {
  Serial.begin(115200);  // Needs to match transmitter
}

void loop() {
  static int state = 0;
  static uint8_t buffer[1];
 
  while (Serial.available() >=2) {
    uint8_t encoded = Serial.read();
    uint8_t feedByte = Serial.read();

    // Decoding byte with jog information
    bool neg = encoded & 0b10000000;
    uint8_t scaleBits = (encoded >> 5) & 0b11;
    uint8_t axis = (encoded >> 3) & 0b11;
    uint8_t magnitude = encoded & 0b00000111;

    int factor = 1;
    if (scaleBits == 0b01) factor = 10;
    else if (scaleBits == 0b10) factor = 100;

    int delta = magnitude * factor;
    if (neg) delta = -delta;

    if (axis == 0b01) y += delta;
    else if (axis == 0b10) z += delta;
    else x += delta;

    Serial.print("Encoded feed byte: ");
    Serial.println(feedByte, BIN);

    bool FWD, REV;
    int feedSpeed;

    decodeFeed(feedByte, FWD, REV, feedSpeed);

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
  }
}

void decodeFeed(uint8_t feedByte, bool &isFWD, bool &isREV, int &feedrate) {
  isREV = feedByte & 0b10000000;
  isFWD = !isREV;

  feedrate = feedByte & 0b01111111;
}