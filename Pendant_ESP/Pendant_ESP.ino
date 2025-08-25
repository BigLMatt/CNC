#define ENC_A 25
#define ENC_B 26
#define RX 16
#define TX 17

#define DEBUG

volatile int readDelta = 0;

uint8_t encodeDelta(int delta){
  uint8_t encoded = 0b0100'0000;

  // Put sign bit at position 5
  if (delta < 0) {
    encoded |= 0b1 << 5;
    delta = -delta;
  }

  // Limit max delta to 31
  if (delta > 31){
    delta = 31;
    Serial.println("Delta too high");
  } 

  // Put the trimmed delta in the remaining spots
  encoded |= (delta & 0b00011111);

  return encoded;
}

void encoderReadout() {
  static int8_t encVal = 0;   // Encoder value
  static uint8_t old_AB = 3;  // Lookup table index (starts at 11)
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Shift left twice, remembering previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A (as MSB)
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B (as LSB)

  encVal += enc_states[(old_AB & 0x0f)];

  // Update counter
  if(encVal > 3){     // Could be 1 depending on construction of encoder half or full quadrature
    readDelta++;   // Update counter
    encVal = 0;
  }
  else if(encVal < -3){   // Could be -1 depending on construction of encoder half or full quadrature
    readDelta--;     // Update counter
    encVal = 0;
  }  
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderReadout, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderReadout, CHANGE);  

}

void loop() {
  noInterrupts();
  int rawDelta = readDelta;
  readDelta = 0;
  interrupts();

  if(rawDelta != 0){ 
    #ifdef DEBUG
    Serial.print("Jog delta (raw): ");
    Serial.print(rawDelta);
    #endif
    
    uint8_t encodedDelta = encodeDelta(rawDelta);   
    Serial1.write(encodedDelta);

  }
  

}
