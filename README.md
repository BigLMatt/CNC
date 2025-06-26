# CNC Pendant Communication Interface

A simple rotary encoder pendant for manual CNC control using serial communication. This setup allows manual jogging of machine axes and feed rate adjustment through a handheld pendant.
## Features

- Rotary encoder for precise jogging
- Adjustable jog factor: ×1, ×10, ×100
- Feedrate for smooth feed with buttons
- Minimal wires between communicating boards
- Possibility of emergency stop through ethernet cable
  
## Hardware Used

- Arduino Uno R4 WiFi (Pendant)
- Arduino Mega R3 (Receiver)
- Rotary encoder
- Potentiometer (analog input for feed override)
- Dipswitches (x10 and x100 selector)
- Breadboard
- Jumper wires

## Wiring
Both boards share a common GND, and TX of the pendant connects to RX (Serial1) of the receiver.
The rest of the pins are configurable in the code

## Serial Communication Protocol

The pendant sends 2 bytes:
1. **Jog delta**: packed byte with sign, scale (×1/×10/×100), and magnitude
2. **Feed override**: 0–50 mapped from potentiometer

## Getting Started

1. Upload pendant_sender.ino to the Uno R4.
2. Upload receiver.ino to the Mega (or other controller).
3. Open serial monitor at **115200 baud** on the receiver.
4. Rotate encoder and see jog deltas + feedrate printed.
   
## Future Improvements

- Add optocoupler isolation for robust industrial signals
- Switch from Arduino to ESP32 with Ethernet support
- Integrate with LinuxCNC
- Add emergency stop
- Add buttons for continous feeding

## License

MIT License — free to use, modify, and distribute.
