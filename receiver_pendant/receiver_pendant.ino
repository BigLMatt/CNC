int x = 0;
int y = 0;
int z = 0;
int lastFeedSpeed = -1;
uint8_t feedSpeed = 0;

void setup() {
  Serial.begin(115200);  // Needs to match transmitter
}

void loop() {
  static int state = 0;
  static uint8_t buffer[2];
 
  if (Serial.available()) {
    buffer[state++] = Serial.read();
    if (state == 2) {
      state = 0;

      uint8_t encoded = buffer[0];
      feedSpeed = buffer[1];

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

      Serial.print("X: ");
      Serial.print(x);
      Serial.print("  Y: ");
      Serial.print(y);
      Serial.print("  Z: ");
      Serial.print(z);
      Serial.print(" | Feed: ");
      Serial.println(feedSpeed);

      lastFeedSpeed = feedSpeed;
    }
  }
}