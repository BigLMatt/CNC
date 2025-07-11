import serial
import time

SERIAL_PORT = 'COM3'
BAUDRATE = 115200

try:
    ser = serial.Serial(port = SERIAL_PORT,baudrate = BAUDRATE, timeout = 1, bytesize = serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)
    print(f'[INFO] Serial port opened at {SERIAL_PORT} at baudrate: {BAUDRATE} baud.')

except serial.SerialException as e:
    print(f'[ERROR] Failed to open serial port at {SERIAL_PORT}: {e}')
    exit(1)

while True:
    line = (ser.readline()).strip()