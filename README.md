# CNC Pendant Communication Interface

A simple rotary encoder pendant for manual CNC control using serial communication. This setup allows manual jogging of machine axes and feed rate adjustment through a handheld pendant.
## Features

- Rotary encoder for precise jogging
- Adjustable jog factor: ×1, ×10, ×100
- Feedrate for smooth feed with buttons
- Minimal wires between communicating boards
- Possibility of emergency stop through ethernet cable
  
## Hardware Used

- ESP32 Devkit V1 (Pendant)
- Computer/ Raspberry Pi (Receiver)
- Rotary encoder (jog wheel)
- Potentiometer (analog input for feed)
- 2x Selector switch + knob
- Protoboard
- SN75HVD12P tranciever IC 3.3V half duplex
- USB to Serial dongle
- 2x Ethernet breakout board
- Ethernet cable
- Buck converter (optional)
- E-stop (optional)

## Wiring

Hook up a buck converter to the VIN pin on the ESP so you can power the pendant with higher voltages.
This allows lower current and less voltage drop over the ethernet cable.
The input of the buck converter is hooked up to the ethernet breakout board.

Buttons and selectors are connected to a pulled up pin so that they when they are pressed they give a LOW signal.

The jogwheel/encoder is also connected to regular GPIO pins and fed from the ESP32 on 3.3V.
Most models of metallic jogwheels with inverted outputs seem to state that they need 5V but from testing they seem to work fine on 3.3V.

The TX signal is fed into the tranciever chip which is always in send mode for now but is able to be put in recieve mode via a GPIO pin.

Connect A and B differential outputs of the tranciever to the ethernet breakout board, make sure they are part of the same twisted pair.

If an estop is used, then the four terminals are just connected straight to the breakout board (DO NOT CONNECT THROUGH MICROCONTROLLER).
This makes it so that all the ethernet pairs are populated, 2 for communication 2 for data and 4 for dual channel estop.

## Serial Communication Protocol

The pendant sends 3 types of bytes:
1. **Jog delta**: byte with jog sign and magnitude
2. **Feed**: 0–63 mapped from potentiometer and direction data, sent periodically while either feed button is pressed.
3. **Setup byte/Axis byte**: A byte that packs in the scale and selected axis. Sent when something changes and on an interval as a sort of failsafe/heartbeat.

## Getting Started

1. Upload Pendant_ESP.ino to the ESP32.
2. Download PendantHAL.py to the reciever device (Pi/PC).
3. Power the pendant externally via buck converter.
4. Hook up the estop to extenal safety circuits.
5. Hook up the A and B differential signals to the USB dongle which is plugged into the reciever
6. Run the PendantHAL.py script in any IDE or text editor
7. The values of x y and z should get printed and updated when the jogwheel is used.
   
## Future Improvements

- Properly integrate the feedspeed to actually change the x, y and z values.
- Get info back from receiver, like end of travel reached so a motor in pendant could vibrate.
- Integrate with LinuxCNC.

## Previous version

Previously an arduino Uno R4 wifi was used for the pendant microcontroller and an arduino mega as the receiver.
The TX of the pendant was hooked up straight to the RX of the receiver and GND was also connected between both.
The code of this prototype version is still available in the Arduino version (old) folder.
The receiver does not need to be an arduino mega, any device with an RX pin wil suffice.

## Notes

All bytes start with 1 or two bits that identify the type of byte, this is why the value of the potentiometer only has 6 bits.
It is possible to change to multiple bytes per package but this was not robust and the complexity was not needed.

The code in the ESP version includes a yellow indicator led that is built into the ethernet breakout board used.
This is highly recommended for debugging of input elements.

If a wireless version is desired, an estop cannot safely be used.

## License

MIT License — free to use, modify, and distribute.
