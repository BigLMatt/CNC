int jogPosition = 0;
int lastFeedSpeed = -1;
uint8_t feedSpeed = 0;

void setup() {
  Serial.begin(9600); // Ontvanger snelheid moet exact gelijk zijn
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

      bool neg = encoded & 0b10000000;
      uint8_t scaleBits = (encoded >> 5) & 0b11;
      uint8_t magnitude = encoded & 0b00011111;

      int factor = 1;
      if (scaleBits == 0b01) factor = 10;
      else if (scaleBits == 0b10) factor = 100;

      int delta = magnitude * factor;
      if (neg) delta = -delta;

      jogPosition += delta;

      Serial.print("Jog pos: ");
      Serial.print(jogPosition);
      Serial.print(" | Feed: ");
      Serial.println(feedSpeed);

      lastFeedSpeed = feedSpeed;
    }
  }
}