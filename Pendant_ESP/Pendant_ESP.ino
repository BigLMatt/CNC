/*  
Project: CNC Pendant, pendant side
Description: Sends jog inputs from jog wheel and feed rates when feed buttons are pressed using serial communication.
Author: Matteo https://github.com/BigLMatt
Date last altered: 08/07/2025

Sender is ESP32 WROOM Devkit V1
Preferrably use a differential tranciever IC (e.g. SN75HVD12P) for longer distance communication of RX and TX signals
All other pins are described below, most can be exchanged except for ENC_A and ENC_B need to be pins that can handle interrupt
Also make sure to use pins that support internal pullup for the many digital signals for ease of use.

Jog and feed data are encoded with all necessary info in their respective bytes
Baudrate is 115200, lower is possible since there is not a lot of traffic.
Expandable to more axis or speed multipliers when hardware is adjusted accordingly.

Encoder readout code based on code found here https://github.com/mo-thunderz/RotaryEncoder
*/

// Allows printing out of info like raw encoder delta and feedrate
#define DEBUG

#define ENC_A 25
#define ENC_B 26
#define RX 16
#define TX 17
#define POT 34
#define FEED_FWD 13
#define FEED_REV 14
#define MULT1 23
#define MULT10 22
#define MULT100 21
#define X_AXIS 33
#define Y_AXIS 32
#define Z_AXIS 27

uint8_t prevStates {0b1111'1111};
volatile int readDelta {0};
int sumFeedSpeed {0};
int feedSpeed {0};
int speedAmount {0};

unsigned long lastFeedTime {0};
constexpr unsigned long feedInterval {50};
constexpr unsigned long setupInterval {5000};
unsigned long lastSetupTime {0};

// Encode change in axis or multiplicand jog
uint8_t encodeAxis(uint8_t states){
  uint8_t encoded {0};

  // Put factor multiplicand bits in postion 3 and 2
  if (states & 0b0000'0001) encoded |= (0b00 << 2);
  else if(states & 0b0000'0010) encoded |= (0b01 << 2);
  else if(states & 0b0000'0100) encoded |= (0b10 << 2);
  
  // Put axis direction bits at position 1 and 0
  if(states & 0b0000'1000) encoded |= 0b00;
  else if(states & 0b0001'0000) encoded |= 0b01;
  else if(states & 0b0010'0000) encoded |= 0b10;

  return encoded;
}

uint8_t encodeFeed(uint8_t states, int speedVal){
  uint8_t encoded {0b1000'0000};

  speedVal = speedVal >> 6;
  encoded |= (speedVal & 0b0011'1111);

  if(states & 0b1000'0000){
    encoded |= 0b0100'0000;
  } else if(!(states & 0b0100'0000)){
    return 0b1000'0000;
  }

  return encoded;
}

uint8_t encodeDelta(int delta){
  uint8_t encoded {0b0100'0000};

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
  encoded |= (delta & 0b0001'1111);

  return encoded;
}

void encoderReadout() {
  static int8_t encVal {0};   // Encoder value
  static uint8_t old_AB {3};  // Lookup table index (starts at 11)
  static constexpr int8_t enc_states[] {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Shift left twice, remembering previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02; // Add current state of pin A (as MSB)
  if (digitalRead(ENC_B)) old_AB |= 0x01; // Add current state of pin B (as LSB)

  encVal += enc_states[(old_AB & 0x0f)];

  // Update counter
  if(encVal > 3){     // Could be 1 depending on construction of encoder half or full quadrature
    ++readDelta;   // Update counter
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
  pinMode(MULT1, INPUT_PULLUP);
  pinMode(MULT10, INPUT_PULLUP);
  pinMode(MULT100, INPUT_PULLUP);
  pinMode(X_AXIS, INPUT_PULLUP);
  pinMode(Y_AXIS, INPUT_PULLUP);
  pinMode(Z_AXIS, INPUT_PULLUP);
  pinMode(FEED_FWD, INPUT_PULLUP);
  pinMode(FEED_REV, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderReadout, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderReadout, CHANGE);  

}

void loop() {

  static uint8_t prevStates {0};
  uint8_t states {0};
  constexpr int readPin[] {MULT1, MULT10, MULT100, X_AXIS, Y_AXIS, Z_AXIS, FEED_FWD, FEED_REV};
  for(int i {0}; i < 8; ++i){
    states |= (!digitalRead(readPin[i]) << i);
  }

  int feedSpeed = analogRead(POT);

  noInterrupts();
  int rawDelta {readDelta};
  readDelta = 0;
  interrupts();

  if(rawDelta != 0){ 
    #ifdef DEBUG
    Serial.print("Jog delta (raw): ");
    Serial.print(rawDelta);
    #endif
    
    const uint8_t encodedDelta = encodeDelta(rawDelta);   
    Serial1.write(encodedDelta);

  }
  
  // Send setup byte when something changes or after fixed time
  if(((states ^ prevStates) & 0b0011'1111) || (millis() - lastSetupTime >= setupInterval)){
    const uint8_t axis = encodeAxis(states);

    lastSetupTime = millis();

    Serial1.write(axis);

    #ifdef DEBUG
    Serial.print("Sent setup byte: ");
    Serial.println(axis);
    #endif
  } 

  // Send stop byte for feeding
  if(((states ^ prevStates) & 0b1100'0000) && !(states & 0b1100'0000)){
    Serial1.write(0b1000'0000);
  }

  // Periodically send value of potentiometer feed while button is pressed
  if((states & 0b1100'0000) && (millis() - lastFeedTime >= feedInterval)){
    lastFeedTime = millis();
    
    if(speedAmount > 1){
      feedSpeed = sumFeedSpeed/speedAmount;
    }
    sumFeedSpeed = 0;
    speedAmount = 0;

    uint8_t feedByte {encodeFeed(states, feedSpeed)};
    Serial1.write(feedByte);

    #ifdef DEBUG
    Serial.print("Feed speed: ");
    Serial.println(feedSpeed);
    #endif
  } else if(speedAmount < 100){
    ++speedAmount;
    sumFeedSpeed += feedSpeed;
  }

  prevStates = states;
}
