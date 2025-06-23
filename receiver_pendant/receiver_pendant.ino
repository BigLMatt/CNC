#include <SPI.h>

volatile int jogCommandDelta = 0;
volatile uint8_t feedSpeed = 0;

int lastFeedSpeed = 0;
int jogPosition = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin(0x08);  // I²C slave adres
  Wire.onReceive(receiveData);
}

void loop() {
  // Enkel printen als er iets veranderd is
  noInterrupts();
  int delta = jogCommandDelta;
  jogCommandDelta = 0;
  interrupts();

  if (delta != 0 || feedSpeed != lastFeedSpeed) {
    Serial.print("Ontvangen jog pos: ");
    Serial.print(jogPosition);
    Serial.print(" | Feed speed: ");
    Serial.println(feedSpeed);

    lastFeedSpeed = feedSpeed;
  }
  delay(10); // CPU ontlasten
}

void receiveData(int byteCount) {
  if (byteCount >= 2) {
    int encodedDelta = Wire.read();
    feedSpeed = Wire.read();

    // Decodeer de geëncodeerde delta
    bool neg = encodedDelta & 0b10000000;
    uint8_t factorBits = (encodedDelta >> 5) & 0b11;
    uint8_t magnitude = encodedDelta & 0b00011111;

    int factor = 1;
    if (factorBits == 0b01) factor = 10;
    else if (factorBits == 0b10) factor = 100;

    int delta = magnitude * factor;
    if (neg) delta = -delta;

    jogPosition += delta;
  }
}
